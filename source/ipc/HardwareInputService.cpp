#include "ipc/HardwareInputService.hpp"

#include "ipc/SharedMemory/SharedMemory.hpp"

#include <boost/interprocess/exceptions.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace IPC {

namespace {
std::string trim(std::string s) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool writeTextFile(const std::string& path, const std::string& value) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << value;
    return out.good();
}

int clampInt(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

bool parseBoolEnv(const char* name, bool fallback) {
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') {
        return fallback;
    }

    std::string v = toLower(value);
    if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
    if (v == "0" || v == "false" || v == "no" || v == "off") return false;
    return fallback;
}

constexpr uint8_t kMcp23017IodirA = 0x00;
constexpr uint8_t kMcp23017IodirB = 0x01;
constexpr uint8_t kMcp23017GppuA = 0x0C;
constexpr uint8_t kMcp23017GppuB = 0x0D;
constexpr uint8_t kMcp23017GpioA = 0x12;
constexpr uint8_t kMcp23017GpioB = 0x13;

std::string mcpPinName(int pin) {
    const char port = (pin < 8) ? 'A' : 'B';
    return std::string("GP") + port + std::to_string(pin % 8);
}
} // namespace

HardwareInputService::HardwareInputService(std::string commandFilePath)
    : _commandFilePath(std::move(commandFilePath)) {}

HardwareInputService::~HardwareInputService() {
    stop();
}

int HardwareInputService::parseIntEnv(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') {
        return fallback;
    }

    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(value, &end, 0);
    if (errno != 0 || end == value || (end && *end != '\0')) {
        std::cerr << "[HWIO] Invalid integer for " << name << ": '" << value
                  << "', fallback=" << fallback << std::endl;
        return fallback;
    }

    return static_cast<int>(parsed);
}

void HardwareInputService::start() {
    if (_running.exchange(true)) {
        return;
    }

    std::ofstream ensureFile(_commandFilePath, std::ios::app);
    ensureFile.close();

    try {
        _readOffset = static_cast<std::streamoff>(std::filesystem::file_size(_commandFilePath));
    } catch (...) {
        _readOffset = 0;
    }

    std::cout << "[HWIO] Service started. Command file: " << _commandFilePath << std::endl;
    std::cout << "[HWIO] Example: echo 'play' >> " << _commandFilePath << std::endl;

    _hardwareInitialized = initHardwareFromEnv();
    _lastButtonEvent = std::chrono::steady_clock::now();
    _lastEncoderEvent = std::chrono::steady_clock::now();
    _lastEncoderButtonEvent = std::chrono::steady_clock::now();

    if (!sendModeToAudio()) {
        std::cout << "[HWIO] Initial mode sync pending (audio endpoint unavailable)" << std::endl;
    }

    if (_hardwareInitialized) {
        std::cout << "[HWIO] Direct hardware polling enabled (GPIO/SPI)" << std::endl;
    } else {
        std::cout << "[HWIO] Direct hardware polling disabled (file commands only)" << std::endl;
    }

    _worker = std::thread(&HardwareInputService::workerLoop, this);
}

void HardwareInputService::stop() {
    if (!_running.exchange(false)) {
        return;
    }

    if (_worker.joinable()) {
        _worker.join();
    }

    closeGpioInput(_button);
    closeGpioInput(_encoder.clk);
    closeGpioInput(_encoder.dt);
    closeGpioInput(_encoderButton);

#ifdef __linux__
    if (_pot.fd >= 0) {
        close(_pot.fd);
    }
    if (_mcp.fd >= 0) {
        close(_mcp.fd);
    }
#endif
    _pot.fd = -1;
    _pot.enabled = false;
    _mcp.fd = -1;
    _mcp.enabled = false;

    std::cout << "[HWIO] Service stopped" << std::endl;
}

void HardwareInputService::workerLoop() {
    while (_running.load()) {
        try {
            processCommandFile();
            if (_hardwareInitialized) {
                pollHardware();
            }
        } catch (const std::exception& e) {
            std::cerr << "[HWIO] Loop error: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HardwareInputService::processCommandFile() {
    if (!std::filesystem::exists(_commandFilePath)) {
        return;
    }

    const auto fileSize = static_cast<std::streamoff>(std::filesystem::file_size(_commandFilePath));
    if (fileSize < _readOffset) {
        _readOffset = 0;
    }

    std::ifstream in(_commandFilePath);
    if (!in.is_open()) {
        return;
    }

    in.seekg(_readOffset, std::ios::beg);
    std::string line;
    while (std::getline(in, line)) {
        processCommandLine(line);
    }

    if (in.good() || in.eof()) {
        _readOffset = in.tellg();
        if (_readOffset < 0) {
            _readOffset = fileSize;
        }
    }
}

void HardwareInputService::pollHardware() {
    handleEncoderInput();
    handleEncoderButtonInput();
    handlePotInput();
    handleMcp23017Buttons();
}

bool HardwareInputService::initHardwareFromEnv() {
    _ringIndex = std::max(0, parseIntEnv("TINYFW_HW_RING_INDEX", 0));
    _pointIndex = std::max(0, parseIntEnv("TINYFW_HW_POINT_INDEX", 0));

    _buttonDebounceMs = std::max(10, parseIntEnv("TINYFW_HW_BUTTON_DEBOUNCE_MS", 120));
    _encoderDebounceMs = std::max(0, parseIntEnv("TINYFW_HW_ENCODER_DEBOUNCE_MS", 2));
    _encoderButtonDebounceMs = std::max(10, parseIntEnv("TINYFW_HW_ENCODER_BUTTON_DEBOUNCE_MS", 120));
    _potDeadband = std::max(1, parseIntEnv("TINYFW_HW_POT_DEADBAND", 8));

    _encoderStep = std::max(1, parseIntEnv("TINYFW_HW_ENCODER_STEP", 1));
    _encoderInvert = parseBoolEnv("TINYFW_HW_ENCODER_INVERT", false);

    _potMinSubdiv = std::max(1, parseIntEnv("TINYFW_HW_POT_MIN_SUBDIV", 2));
    _potMaxSubdiv = std::max(_potMinSubdiv, parseIntEnv("TINYFW_HW_POT_MAX_SUBDIV", 16));

    _playPauseMcpPin = clampInt(parseIntEnv("TINYFW_HW_PLAY_PAUSE_MCP_PIN", 0), -1, 15);
    _navLeftPin = clampInt(parseIntEnv("TINYFW_HW_NAV_LEFT_PIN", 1), -1, 15);
    _navRightPin = clampInt(parseIntEnv("TINYFW_HW_NAV_RIGHT_PIN", 2), -1, 15);
    _modeTogglePin = clampInt(parseIntEnv("TINYFW_HW_MODE_TOGGLE_PIN", -1), -1, 15);
    _modePointsPin = clampInt(parseIntEnv("TINYFW_HW_MODE_POINTS_PIN", -1), -1, 15);
    _modeRotatePin = clampInt(parseIntEnv("TINYFW_HW_MODE_ROTATE_PIN", 3), 0, 15);
    _encoderSwitchMcpPin = clampInt(parseIntEnv("TINYFW_HW_ENCODER_SW_MCP_PIN", 4), -1, 15);

    const char* modeEnv = std::getenv("TINYFW_HW_ENCODER_MODE");
    if (modeEnv && modeEnv[0] != '\0') {
        std::string mode = toLower(modeEnv);
        if (mode == "toggle" || mode == "toggle_point" || mode == "activation") {
            _encoderMode = EncoderMode::TogglePoint;
        } else if (mode == "points" || mode == "add_remove" || mode == "addremove") {
            _encoderMode = EncoderMode::AddRemovePoint;
        } else if (mode == "rotate") {
            _encoderMode = EncoderMode::Rotate;
        }
    }

    bool anyHardware = false;

    const int buttonGpio = parseIntEnv("TINYFW_HW_BUTTON_GPIO", -1);
    if (buttonGpio >= 0) {
        std::cout << "[HWIO] Ignoring BUTTON(GPIO" << buttonGpio
                  << "): play/pause now routed via MCP23017"
                  << std::endl;
    }

    const int encClk = parseIntEnv("TINYFW_HW_ENCODER_CLK_GPIO", 17);
    const int encDt = parseIntEnv("TINYFW_HW_ENCODER_DT_GPIO", 27);
    if (encClk >= 0 && encDt >= 0) {
        _encoder.clk.valid = initGpioInput(_encoder.clk, encClk);
        _encoder.dt.valid = initGpioInput(_encoder.dt, encDt);
        _encoder.enabled = _encoder.clk.valid && _encoder.dt.valid;
        anyHardware = anyHardware || _encoder.enabled;
        if (_encoder.enabled) {
            _encoder.lastClk = _encoder.clk.lastValue;
            std::cout << "[HWIO] Encoder GPIO CLK=" << encClk
                      << " DT=" << encDt
                      << " step=" << _encoderStep
                      << (_encoderInvert ? " (inverted)" : "")
                      << std::endl;
        } else {
            std::cout << "[HWIO] Encoder disabled (CLK GPIO" << encClk
                      << ", DT GPIO" << encDt << " not accessible)"
                      << std::endl;
        }
    }

    // Keep compatibility with legacy wiring:
    // - If BUTTON_GPIO is set, reuse it as encoder click input.
    // - Otherwise default encoder click to GPIO22 if ENCODER_SW_GPIO is unset.
    const int encoderSwDefault = (buttonGpio >= 0) ? buttonGpio : 22;
    const int encoderSwGpio = parseIntEnv("TINYFW_HW_ENCODER_SW_GPIO", encoderSwDefault);
    if (encoderSwGpio >= 0) {
        _encoderButton.valid = initGpioInput(_encoderButton, encoderSwGpio);
        anyHardware = anyHardware || _encoderButton.valid;
        if (_encoderButton.valid) {
            std::cout << "[HWIO] Encoder switch GPIO=" << encoderSwGpio
                      << " (override with TINYFW_HW_ENCODER_SW_GPIO=-1 to disable)"
                      << std::endl;
        } else {
            std::cout << "[HWIO] Encoder switch GPIO" << encoderSwGpio
                      << " disabled (not accessible)" << std::endl;
        }
    }

    if (initPotInputFromEnv()) {
        anyHardware = true;
        std::cout << "[HWIO] MCP3008 SPI=" << _pot.spiDevice
                  << " CH=" << _pot.channel
                  << " -> POLY_MOD_SHAPE ring=" << _ringIndex
                  << " shape=0 subdivisions=[" << _potMinSubdiv
                  << "," << _potMaxSubdiv << "]"
                  << std::endl;
    }

    if (initMcp23017FromEnv()) {
        anyHardware = true;
        char addrBuf[16];
        std::snprintf(addrBuf, sizeof(addrBuf), "0x%02X", _mcp.address);
        std::cout << "[HWIO] MCP23017 I2C=" << _mcp.i2cDevice
                  << " addr=" << addrBuf
                  << " (buttons GPA0..GPB7, active-low)"
                  << std::endl;
        std::cout << "[HWIO] Play/Pause via MCP23017: "
              << (_playPauseMcpPin >= 0 ? mcpPinName(_playPauseMcpPin) : std::string("disabled"))
              << std::endl;
        std::cout << "[HWIO] Carousel nav via MCP23017: left="
                  << (_navLeftPin >= 0 ? mcpPinName(_navLeftPin) : std::string("disabled"))
                  << " right="
                  << (_navRightPin >= 0 ? mcpPinName(_navRightPin) : std::string("disabled"))
                  << std::endl;
        std::cout << "[HWIO] Mode buttons via MCP23017: toggle="
                  << (_modeTogglePin >= 0 ? mcpPinName(_modeTogglePin) : std::string("disabled"))
                  << " points="
                  << (_modePointsPin >= 0 ? mcpPinName(_modePointsPin) : std::string("disabled"))
                  << " rotate="
                  << (_modeRotatePin >= 0 ? mcpPinName(_modeRotatePin) : std::string("disabled"));
        if (_encoderSwitchMcpPin >= 0) {
            std::cout << " encoder_click=" << mcpPinName(_encoderSwitchMcpPin);
        }
        std::cout << std::endl;
    }

    return anyHardware;
}

bool HardwareInputService::initGpioInput(GpioInput& input, int gpio) {
#ifdef __linux__
    input.gpio = gpio;
    auto openSysfsGpio = [&](int sysfsGpio) {
        const std::string base = "/sys/class/gpio/gpio" + std::to_string(sysfsGpio);

        if (!std::filesystem::exists(base)) {
            std::ofstream exportFile("/sys/class/gpio/export");
            if (exportFile.is_open()) {
                exportFile << sysfsGpio;
            }
        }

        if (!std::filesystem::exists(base)) {
            return false;
        }

        writeTextFile(base + "/direction", "in");
        writeTextFile(base + "/edge", "both");

        const int fd = open((base + "/value").c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            return false;
        }

        input.fd = fd;
        input.lastValue = readGpioValue(input);
        input.valid = true;
        return true;
    };

    // First try the value as-is (legacy systems where sysfs index matches BCM GPIO).
    if (openSysfsGpio(gpio)) {
        return true;
    }

    // Fallback for newer kernels where sysfs numbering is offset by gpiochip base.
    if (gpio >= 0 && gpio < 64) {
        std::vector<int> chipBases;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/gpio")) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::string name = entry.path().filename().string();
            if (name.rfind("gpiochip", 0) != 0) {
                continue;
            }

            std::ifstream baseFile(entry.path() / "base");
            int baseValue = -1;
            if (baseFile >> baseValue) {
                chipBases.push_back(baseValue);
            }
        }

        std::sort(chipBases.begin(), chipBases.end());
        chipBases.erase(std::unique(chipBases.begin(), chipBases.end()), chipBases.end());

        for (int baseValue : chipBases) {
            const int mapped = baseValue + gpio;
            if (mapped == gpio) {
                continue;
            }

            if (openSysfsGpio(mapped)) {
                std::cout << "[HWIO] GPIO" << gpio
                          << " mapped to sysfs GPIO" << mapped << std::endl;
                return true;
            }
        }
    }

    std::cerr << "[HWIO] GPIO" << gpio << " unavailable (export failed?)" << std::endl;
    return false;
#else
    (void)input;
    (void)gpio;
    return false;
#endif
}

int HardwareInputService::readGpioValue(GpioInput& input) {
#ifdef __linux__
    if (input.fd < 0) return -1;

    char c = '1';
    if (lseek(input.fd, 0, SEEK_SET) < 0) {
        return -1;
    }
    if (read(input.fd, &c, 1) != 1) {
        return -1;
    }
    return (c == '0') ? 0 : 1;
#else
    (void)input;
    return -1;
#endif
}

void HardwareInputService::closeGpioInput(GpioInput& input) {
#ifdef __linux__
    if (input.fd >= 0) {
        close(input.fd);
    }
#endif
    input.fd = -1;
    input.valid = false;
}

bool HardwareInputService::initPotInputFromEnv() {
    const char* spiEnv = std::getenv("TINYFW_HW_POT_SPI_DEV");
    if (spiEnv && std::string(spiEnv) == "off") {
        _pot.enabled = false;
        return false;
    }

    if (spiEnv && spiEnv[0] != '\0') {
        _pot.spiDevice = spiEnv;
    }

    _pot.channel = clampInt(parseIntEnv("TINYFW_HW_POT_CHANNEL", 0), 0, 7);

#ifdef __linux__
    _pot.fd = open(_pot.spiDevice.c_str(), O_RDWR | O_CLOEXEC);
    if (_pot.fd < 0) {
        std::cerr << "[HWIO] SPI open failed for " << _pot.spiDevice
                  << ": " << std::strerror(errno) << std::endl;
        _pot.enabled = false;
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = static_cast<uint32_t>(std::max(1000, parseIntEnv("TINYFW_HW_POT_SPI_HZ", 1000000)));

    if (ioctl(_pot.fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(_pot.fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(_pot.fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "[HWIO] SPI setup failed for " << _pot.spiDevice
                  << ": " << std::strerror(errno) << std::endl;
        close(_pot.fd);
        _pot.fd = -1;
        _pot.enabled = false;
        return false;
    }

    _pot.enabled = true;
    return true;
#else
    _pot.enabled = false;
    return false;
#endif
}

int HardwareInputService::readMcp3008Channel(int channel) {
#ifdef __linux__
    if (_pot.fd < 0) return -1;

    uint8_t tx[3] = {
        0x01,
        static_cast<uint8_t>(0x80 | ((channel & 0x07) << 4)),
        0x00
    };
    uint8_t rx[3] = {0, 0, 0};

    spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(tx);
    tr.rx_buf = reinterpret_cast<unsigned long>(rx);
    tr.len = 3;
    tr.bits_per_word = 8;

    if (ioctl(_pot.fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        return -1;
    }

    return ((rx[1] & 0x03) << 8) | rx[2];
#else
    (void)channel;
    return -1;
#endif
}

bool HardwareInputService::initMcp23017FromEnv() {
    _mcp.enabled = false;
    _mcp.address = clampInt(parseIntEnv("TINYFW_HW_MCP23017_ADDR", 0x20), 0x03, 0x77);
    _mcp.debounceMs = std::max(1, parseIntEnv("TINYFW_HW_MCP23017_DEBOUNCE_MS", 60));

    const bool useMcp = parseBoolEnv("TINYFW_HW_MCP23017_ENABLE", true);
    if (!useMcp) {
        return false;
    }

    const char* i2cDev = std::getenv("TINYFW_HW_MCP23017_I2C_DEV");
    if (i2cDev && i2cDev[0] != '\0') {
        _mcp.i2cDevice = i2cDev;
    }

#ifdef __linux__
    _mcp.fd = open(_mcp.i2cDevice.c_str(), O_RDWR | O_CLOEXEC);
    if (_mcp.fd < 0) {
        std::cerr << "[HWIO] MCP23017 open failed for " << _mcp.i2cDevice
                  << ": " << std::strerror(errno) << std::endl;
        return false;
    }

    if (ioctl(_mcp.fd, I2C_SLAVE, _mcp.address) < 0) {
        std::cerr << "[HWIO] MCP23017 ioctl(I2C_SLAVE) failed addr="
                  << _mcp.address << ": " << std::strerror(errno) << std::endl;
        close(_mcp.fd);
        _mcp.fd = -1;
        return false;
    }

    // Configure both banks as inputs with pull-ups enabled.
    if (!writeMcp23017Register(kMcp23017IodirA, 0xFF) ||
        !writeMcp23017Register(kMcp23017IodirB, 0xFF) ||
        !writeMcp23017Register(kMcp23017GppuA, 0xFF) ||
        !writeMcp23017Register(kMcp23017GppuB, 0xFF)) {
        std::cerr << "[HWIO] MCP23017 register configuration failed" << std::endl;
        close(_mcp.fd);
        _mcp.fd = -1;
        return false;
    }

    uint8_t gpioA = 0xFF;
    uint8_t gpioB = 0xFF;
    if (!readMcp23017Register(kMcp23017GpioA, gpioA) ||
        !readMcp23017Register(kMcp23017GpioB, gpioB)) {
        std::cerr << "[HWIO] MCP23017 initial read failed" << std::endl;
        close(_mcp.fd);
        _mcp.fd = -1;
        return false;
    }

    _mcp.lastState = static_cast<uint16_t>(gpioA) |
        (static_cast<uint16_t>(gpioB) << 8);

    const auto now = std::chrono::steady_clock::now();
    for (auto& ts : _mcp.lastEvent) {
        ts = now;
    }

    _mcp.enabled = true;
    return true;
#else
    return false;
#endif
}

bool HardwareInputService::writeMcp23017Register(uint8_t reg, uint8_t value) {
#ifdef __linux__
    if (_mcp.fd < 0) {
        return false;
    }

    uint8_t payload[2] = {reg, value};
    return write(_mcp.fd, payload, sizeof(payload)) == static_cast<ssize_t>(sizeof(payload));
#else
    (void)reg;
    (void)value;
    return false;
#endif
}

bool HardwareInputService::readMcp23017Register(uint8_t reg, uint8_t& value) {
#ifdef __linux__
    if (_mcp.fd < 0) {
        return false;
    }

    if (write(_mcp.fd, &reg, 1) != 1) {
        return false;
    }

    return read(_mcp.fd, &value, 1) == 1;
#else
    (void)reg;
    (void)value;
    return false;
#endif
}

void HardwareInputService::handleMcp23017Buttons() {
    if (!_mcp.enabled) {
        return;
    }

    uint8_t gpioA = 0xFF;
    uint8_t gpioB = 0xFF;
    if (!readMcp23017Register(kMcp23017GpioA, gpioA) ||
        !readMcp23017Register(kMcp23017GpioB, gpioB)) {
        return;
    }

    const uint16_t currentState = static_cast<uint16_t>(gpioA) |
        (static_cast<uint16_t>(gpioB) << 8);
    const uint16_t changedMask = currentState ^ _mcp.lastState;
    if (changedMask == 0) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    for (int pin = 0; pin < 16; ++pin) {
        if ((changedMask & (static_cast<uint16_t>(1) << pin)) == 0) {
            continue;
        }

        const int prev = ((_mcp.lastState >> pin) & 0x1) ? 1 : 0;
        const int curr = ((currentState >> pin) & 0x1) ? 1 : 0;

        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - _mcp.lastEvent[pin]).count();
        if (elapsedMs < _mcp.debounceMs) {
            continue;
        }

        const std::string pinLabel = mcpPinName(pin);
        std::cout << "[HWIO] MCP23017(" << pinLabel << ") edge "
                  << prev << "->" << curr << std::endl;

        // With pull-up enabled, press is 1->0 and release is 0->1.
        if (prev == 1 && curr == 0) {
            std::cout << "[HWIO] MCP23017(" << pinLabel << ") click detected" << std::endl;

            if (_playPauseMcpPin >= 0 && pin == _playPauseMcpPin) {
                const std::string message = _playingState ? "PAUSE" : "PLAY";
                if (sendToAudio(message)) {
                    _playingState = !_playingState;
                    std::cout << "[HWIO] MCP23017(" << pinLabel << ") -> "
                              << message << std::endl;
                } else {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") detected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (_navLeftPin >= 0 && pin == _navLeftPin) {
                if (sendToAudio("POLY_NAV_LEFT")) {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") -> POLY_NAV_LEFT" << std::endl;
                } else {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") detected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (_navRightPin >= 0 && pin == _navRightPin) {
                if (sendToAudio("POLY_NAV_RIGHT")) {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") -> POLY_NAV_RIGHT" << std::endl;
                } else {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") detected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (pin == _modeTogglePin) {
                if (!setEncoderMode(EncoderMode::TogglePoint)) {
                    std::cout << "[HWIO] Mode toggle selected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (pin == _modePointsPin) {
                if (!setEncoderMode(EncoderMode::AddRemovePoint)) {
                    std::cout << "[HWIO] Mode points selected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (pin == _modeRotatePin) {
                if (!setEncoderMode(EncoderMode::Rotate)) {
                    std::cout << "[HWIO] Mode rotate selected but IPC endpoint unavailable"
                              << std::endl;
                }
            } else if (_encoderSwitchMcpPin >= 0 && pin == _encoderSwitchMcpPin) {
                if (sendToAudio("POLY_ENCODER_CLICK")) {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") -> POLY_ENCODER_CLICK" << std::endl;
                } else {
                    std::cout << "[HWIO] MCP23017(" << pinLabel
                              << ") detected but IPC endpoint unavailable"
                              << std::endl;
                }
            }
        }

        _mcp.lastEvent[pin] = now;
    }

    _mcp.lastState = currentState;
}

void HardwareInputService::handleButtonInput() {
    if (!_button.valid) {
        return;
    }

    const int value = readGpioValue(_button);
    if (value < 0) {
        return;
    }

    if (value != _button.lastValue) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - _lastButtonEvent).count();

        std::cout << "[HWIO] BUTTON(GPIO" << _button.gpio << ") edge "
                  << _button.lastValue << "->" << value << std::endl;

        // Active-low button, trigger on press edge.
        if (_button.lastValue == 1 && value == 0 && elapsedMs >= _buttonDebounceMs) {
            const std::string message = _playingState ? "PAUSE" : "PLAY";
            std::cout << "[HWIO] BUTTON(GPIO" << _button.gpio
                      << ") click detected" << std::endl;

            if (sendToAudio(message)) {
                _playingState = !_playingState;
                std::cout << "[HWIO] BUTTON(GPIO" << _button.gpio << ") -> "
                          << message << std::endl;
            } else {
                std::cout << "[HWIO] BUTTON(GPIO" << _button.gpio
                          << ") detected but IPC endpoint unavailable"
                          << std::endl;
            }

            _lastButtonEvent = now;
        }

        _button.lastValue = value;
    }
}

void HardwareInputService::handleEncoderInput() {
    if (!_encoder.enabled) {
        return;
    }

    const int clk = readGpioValue(_encoder.clk);
    const int dt = readGpioValue(_encoder.dt);
    if (clk < 0 || dt < 0) {
        return;
    }

    if (clk != _encoder.lastClk) {
        // Use falling edge for simpler quadrature handling.
        if (clk == 0) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - _lastEncoderEvent).count();

            if (elapsedMs >= _encoderDebounceMs) {
                int direction = (dt != clk) ? 1 : -1;
                if (_encoderInvert) {
                    direction = -direction;
                }
                direction *= _encoderStep;

                std::cout << "[HWIO] ENCODER(GPIO" << _encoder.clk.gpio
                          << "/GPIO" << _encoder.dt.gpio << ") step="
                          << direction << " (clk=" << clk
                          << ", dt=" << dt << ")" << std::endl;

                // Mode is resolved in AudioEngine from POLY_HW_MODE.
                const std::string message = "POLY_ENCODER_DELTA:" + std::to_string(direction);

                if (sendToAudio(message)) {
                    std::cout << "[HWIO] ENCODER(GPIO" << _encoder.clk.gpio
                              << "/GPIO" << _encoder.dt.gpio << ") -> "
                              << message << std::endl;
                } else {
                    std::cout << "[HWIO] ENCODER(GPIO" << _encoder.clk.gpio
                              << "/GPIO" << _encoder.dt.gpio
                              << ") detected but IPC endpoint unavailable"
                              << std::endl;
                }

                _lastEncoderEvent = now;
            }
        }

        _encoder.lastClk = clk;
    }
}

void HardwareInputService::handleEncoderButtonInput() {
    if (!_encoderButton.valid) {
        return;
    }

    const int value = readGpioValue(_encoderButton);
    if (value < 0) {
        return;
    }

    if (value != _encoderButton.lastValue) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - _lastEncoderButtonEvent).count();

        std::cout << "[HWIO] ENCODER_SW(GPIO" << _encoderButton.gpio << ") edge "
                  << _encoderButton.lastValue << "->" << value << std::endl;

        // Active-low button, trigger on press edge.
        if (_encoderButton.lastValue == 1 && value == 0 && elapsedMs >= _encoderButtonDebounceMs) {
            if (sendToAudio("POLY_ENCODER_CLICK")) {
                std::cout << "[HWIO] ENCODER_SW(GPIO" << _encoderButton.gpio
                          << ") -> POLY_ENCODER_CLICK" << std::endl;
            } else {
                std::cout << "[HWIO] ENCODER_SW(GPIO" << _encoderButton.gpio
                          << ") detected but IPC endpoint unavailable"
                          << std::endl;
            }

            _lastEncoderButtonEvent = now;
        }

        _encoderButton.lastValue = value;
    }
}

void HardwareInputService::handlePotInput() {
    if (!_pot.enabled) {
        return;
    }

    const int raw = readMcp3008Channel(_pot.channel);
    if (raw < 0) {
        return;
    }

    if (_pot.lastRaw < 0) {
        _pot.lastRaw = raw;
    }

    if (std::abs(raw - _pot.lastRaw) < _potDeadband) {
        return;
    }
    _pot.lastRaw = raw;

    const int span = std::max(1, _potMaxSubdiv - _potMinSubdiv);
    int mapped = _potMinSubdiv + static_cast<int>(
        (static_cast<long long>(raw) * span + 511) / 1023);
    mapped = clampInt(mapped, _potMinSubdiv, _potMaxSubdiv);

    if (mapped == _pot.lastMapped) {
        return;
    }
    _pot.lastMapped = mapped;

    char msg[128];
    std::snprintf(msg, sizeof(msg), "POLY_MOD_SHAPE:%d|0|%d|0.000000", _ringIndex, mapped);

    std::cout << "[HWIO] POT(" << _pot.spiDevice << ",CH" << _pot.channel
              << ") raw=" << raw << " mapped=" << mapped << std::endl;

    if (sendToAudio(msg)) {
        std::cout << "[HWIO] POT(" << _pot.spiDevice << ",CH" << _pot.channel
                  << ",raw=" << raw << ") -> " << msg << std::endl;
    } else {
        std::cout << "[HWIO] POT(" << _pot.spiDevice << ",CH" << _pot.channel
                  << ") detected but IPC endpoint unavailable" << std::endl;
    }
}

bool HardwareInputService::setEncoderMode(EncoderMode mode) {
    if (_encoderMode == mode) {
        return sendModeToAudio();
    }

    _encoderMode = mode;

    const char* modeName = "rotate";
    switch (_encoderMode) {
        case EncoderMode::TogglePoint: modeName = "toggle"; break;
        case EncoderMode::AddRemovePoint: modeName = "points"; break;
        case EncoderMode::Rotate: modeName = "rotate"; break;
    }

    std::cout << "[HWIO] Encoder mode -> " << modeName << std::endl;
    return sendModeToAudio();
}

bool HardwareInputService::sendModeToAudio() {
    std::string modeName = "rotate";
    switch (_encoderMode) {
        case EncoderMode::TogglePoint: modeName = "toggle"; break;
        case EncoderMode::AddRemovePoint: modeName = "points"; break;
        case EncoderMode::Rotate: modeName = "rotate"; break;
    }

    return sendToAudio("POLY_HW_MODE:" + modeName);
}

void HardwareInputService::processCommandLine(const std::string& rawLine) {
    const std::string line = trim(rawLine);
    if (line.empty() || line[0] == '#') {
        return;
    }

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    const std::string lc = toLower(cmd);

    std::string out;

    if (lc == "play") {
        out = "PLAY";
    } else if (lc == "pause") {
        out = "PAUSE";
    } else if (lc == "setup") {
        out = "GET_INITIAL_SETUP";
    } else if (lc == "mode") {
        std::string mode;
        if (iss >> mode) {
            mode = toLower(mode);
            if (mode == "toggle" || mode == "toggle_point" || mode == "activation") {
                setEncoderMode(EncoderMode::TogglePoint);
                return;
            } else if (mode == "points" || mode == "add_remove" || mode == "addremove") {
                setEncoderMode(EncoderMode::AddRemovePoint);
                return;
            } else if (mode == "rotate") {
                setEncoderMode(EncoderMode::Rotate);
                return;
            }
        }
    } else if (lc == "select_ring") {
        size_t ring = 0;
        if (iss >> ring) {
            out = "POLY_SET_SELECTED_RING:" + std::to_string(ring);
        }
    } else if (lc == "select_delta") {
        int amount = 0;
        if (iss >> amount) {
            out = "POLY_SELECT_POINT_DELTA:" + std::to_string(amount);
        }
    } else if (lc == "toggle_selected") {
        out = "POLY_TOGGLE_SELECTED_POINT";
    } else if (lc == "adjust_points") {
        int amount = 0;
        if (iss >> amount) {
            out = "POLY_ADJUST_POINTS:" + std::to_string(amount);
        }
    } else if (lc == "rotate_selected") {
        int amount = 0;
        if (iss >> amount) {
            out = "POLY_ROTATE_SELECTED:" + std::to_string(amount);
        }
    } else if (lc == "rotate") {
        size_t ring = 0;
        int amount = 0;
        if (iss >> ring >> amount) {
            out = "POLY_ROTATE:" + std::to_string(ring) + "|" + std::to_string(amount);
        }
    } else if (lc == "toggle") {
        size_t ring = 0;
        size_t point = 0;
        if (iss >> ring >> point) {
            out = "POLY_TOGGLE:" + std::to_string(ring) + "|" + std::to_string(point);
        }
    } else if (lc == "add_shape") {
        size_t ring = 0;
        uint32_t subdiv = 4;
        double offset = 0.0;
        if (iss >> ring >> subdiv >> offset) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "POLY_ADD_SHAPE:%zu|%u|%.6f", ring, subdiv, offset);
            out = buf;
        }
    } else if (lc == "mod_shape") {
        size_t ring = 0;
        size_t shape = 0;
        uint32_t subdiv = 4;
        double offset = 0.0;
        if (iss >> ring >> shape >> subdiv >> offset) {
            char buf[160];
            std::snprintf(buf, sizeof(buf), "POLY_MOD_SHAPE:%zu|%zu|%u|%.6f", ring, shape, subdiv, offset);
            out = buf;
        }
    } else if (lc == "remove_shape") {
        size_t ring = 0;
        size_t shape = 0;
        if (iss >> ring >> shape) {
            out = "POLY_REMOVE_SHAPE:" + std::to_string(ring) + "|" + std::to_string(shape);
        }
    } else if (lc == "raw") {
        std::string rest;
        std::getline(iss, rest);
        out = trim(rest);
    }

    if (out.empty()) {
        std::cerr << "[HWIO] Ignored command line: " << line << std::endl;
        return;
    }

    if (sendToAudio(out)) {
        std::cout << "[HWIO] -> " << out << std::endl;
    }
}

bool HardwareInputService::sendToAudio(const std::string& message) {
    try {
        SharedMemory ipc("TinyFirmwareSharedMemory_GUIToAudio", "gui");
        return ipc.send(message);
    } catch (const boost::interprocess::interprocess_exception&) {
        // Audio endpoint may not be available yet.
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[HWIO] IPC send failed: " << e.what() << std::endl;
        return false;
    }
}

} // namespace IPC
