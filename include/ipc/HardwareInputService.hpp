#ifndef IPC_HARDWARE_INPUT_SERVICE_HPP
#define IPC_HARDWARE_INPUT_SERVICE_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <ios>
#include <string>
#include <thread>

namespace IPC {

/// Bootstrap input bridge for headless development.
///
/// The service watches a command file and forwards mapped actions to the
/// existing GUI->Audio shared-memory endpoint.
class HardwareInputService {
public:
    explicit HardwareInputService(std::string commandFilePath = "/tmp/tinyfw_hwio.cmd");
    ~HardwareInputService();

    void start();
    void stop();

private:
    enum class EncoderMode {
        TogglePoint,
        AddRemovePoint,
        Rotate
    };

    struct GpioInput {
        int gpio = -1;
        int fd = -1;
        int lastValue = 1;
        bool valid = false;
    };

    struct PotInput {
        bool enabled = false;
        std::string spiDevice = "/dev/spidev0.0";
        int fd = -1;
        int channel = 0;
        int lastRaw = -1;
        int lastMapped = -1;
    };

    struct EncoderInput {
        GpioInput clk;
        GpioInput dt;
        int lastClk = 1;
        bool enabled = false;
    };

    struct Mcp23017Input {
        bool enabled = false;
        std::string i2cDevice = "/dev/i2c-1";
        int fd = -1;
        int address = 0x20;
        int debounceMs = 60;
        uint16_t lastState = 0xFFFF;
        std::array<std::chrono::steady_clock::time_point, 16> lastEvent{};
    };

    void workerLoop();
    void processCommandFile();
    void pollHardware();
    void processCommandLine(const std::string& line);
    bool sendToAudio(const std::string& message);

    bool initHardwareFromEnv();
    bool initGpioInput(GpioInput& input, int gpio);
    int readGpioValue(GpioInput& input);
    void closeGpioInput(GpioInput& input);

    bool initPotInputFromEnv();
    int readMcp3008Channel(int channel);

    bool initMcp23017FromEnv();
    bool writeMcp23017Register(uint8_t reg, uint8_t value);
    bool readMcp23017Register(uint8_t reg, uint8_t& value);

    void handleButtonInput();
    void handleEncoderInput();
    void handleEncoderButtonInput();
    void handlePotInput();
    void handleMcp23017Buttons();
    static int parseIntEnv(const char* name, int fallback);
    bool setEncoderMode(EncoderMode mode);
    bool sendModeToAudio();

    std::string _commandFilePath;
    std::thread _worker;
    std::atomic<bool> _running{false};
    std::streamoff _readOffset{0};

    bool _hardwareInitialized{false};
    int _ringIndex{0};
    int _pointIndex{0};
    bool _playingState{false};
    EncoderMode _encoderMode{EncoderMode::TogglePoint};

    int _buttonDebounceMs{120};
    int _encoderDebounceMs{2};
    int _encoderButtonDebounceMs{120};
    int _potDeadband{8};
    int _encoderStep{1};
    bool _encoderInvert{false};
    int _potMinSubdiv{2};
    int _potMaxSubdiv{16};

    int _playPauseMcpPin{0};
    int _navLeftPin{1};
    int _navRightPin{2};
    int _modeTogglePin{1};
    int _modePointsPin{2};
    int _modeRotatePin{3};
    int _encoderSwitchMcpPin{4};

    GpioInput _button;
    EncoderInput _encoder;
    GpioInput _encoderButton;
    PotInput _pot;
    Mcp23017Input _mcp;

    std::chrono::steady_clock::time_point _lastButtonEvent;
    std::chrono::steady_clock::time_point _lastEncoderEvent;
    std::chrono::steady_clock::time_point _lastEncoderButtonEvent;
};

} // namespace IPC

#endif // IPC_HARDWARE_INPUT_SERVICE_HPP
