#include "engine/AudioEngine.hpp"
#include "engine/module/OscModule.hpp"
#include "engine/module/FilterModule.hpp"
#include "engine/module/AudioOutModule.hpp"
#include "engine/module/ModuleFactory.hpp"
#include "engine/module/Instrument.hpp"
#include "engine/module/EventEmitter.hpp"
#include "engine/module/ArpegiatorModule.hpp"
#include "engine/module/EnvelopeModule.hpp"
#include "engine/module/StepSequencerModule.hpp"
#include "engine/module/SamplerModule.hpp"
#include "engine/module/PolyRhythmModule.hpp"
#include <thread>
#include <iostream>
#include <string>
#include <chrono>
#include <algorithm>
#include <cctype>

#include "engine/config/InstrumentParser.hpp"

#include "engine/config/InstrumentBuilder.hpp"
#include "ipc/SharedMemory/SharedMemory.hpp"

namespace Engine {

    AudioEngine::AudioEngine(Common::SPSCQueue<std::unique_ptr<Common::ICommand>, 1024>& commandQueue,
                             Common::SPSCQueue<std::unique_ptr<Common::IEvent>, 1024>& eventQueue)
        : Common::Consumer<std::unique_ptr<Common::ICommand>>(commandQueue),
          Common::Producer<std::unique_ptr<Common::IEvent>>(eventQueue),
          _isPlaying(false) {
    }

    AudioEngine::~AudioEngine() = default;

    bool AudioEngine::initialize() {
        // ═══════════════════════════════════════════════════════════════════════
        //  Create one PolyRhythmModule per instrument — each with its own shapes
        // ═══════════════════════════════════════════════════════════════════════

        struct InstrumentDef {
            std::string name;
            int         noteNumber;
            int         velocity;
            uint32_t    color;
            int         radius;
            uint32_t    defaultSubdiv;
            std::string shapeName;
        };

        std::vector<InstrumentDef> defs = {
            {"Kick1",  48, 100, 0xff8c00, 140, 4, "Quarter"},
            {"Snare1", 50, 100, 0x0080ff, 110, 3, "Triplets"},
            {"HiHat1", 52,  80, 0x00ff00,  80, 4, "Quarter"},
            {"Kick1bis",  54, 100, 0xff00ff, 140, 4, "Quarter"},
            {"Kick2",  56, 100, 0xff8c00, 140, 5, "Quintuplets"},
            {"Snare2", 58, 100, 0x0080ff, 110, 5, "Quintuplets"},
            {"HiHat2", 60,  80, 0x00ff00,  80, 5, "Quintuplets"},
            {"Kick2bis",  62, 100, 0xff00ff, 140, 5, "Quintuplets"},
            {"Kick3",  64, 100, 0xff8c00, 140, 2, "Quintuplets"},
            {"Snare3", 66, 100, 0x0080ff, 110, 2, "Quintuplets"},
            {"HiHat3", 68,  80, 0x00ff00,  80, 2, "Quintuplets"},
            {"Kick3bis",  70, 100, 0xff00ff, 140, 2, "Quintuplets"},
            {"Kick4",  72, 100, 0xff8c00, 140, 3, "Quintuplets"},
            {"Snare4", 74, 100, 0x0080ff, 110, 3, "Quintuplets"},
            {"HiHat4", 76,  80, 0x00ff00,  80, 3, "Quintuplets"},
            {"Kick4bis",  78, 100, 0xff00ff, 140, 3, "Quintuplets"},
        };

        auto drumSampler    = Module::ModuleFactory::instance().create("sampler", this);
        auto audioOutModule = Module::ModuleFactory::instance().create("audio_out", this);

        _audioGraph.addModule(drumSampler);
        _audioGraph.addModule(audioOutModule);

        // Load samples
        Engine::Util::WavData kick1Sample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData snare1Sample = Engine::Util::WavLoader::load("../samples/snare.wav");
        Engine::Util::WavData hihat1Sample = Engine::Util::WavLoader::load("../samples/hihat.wav");
        Engine::Util::WavData kick1bisSample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData kick2Sample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData snare2Sample = Engine::Util::WavLoader::load("../samples/snare.wav");
        Engine::Util::WavData hihat2Sample = Engine::Util::WavLoader::load("../samples/hihat.wav");
        Engine::Util::WavData kick2bisSample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData kick3Sample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData snare3Sample = Engine::Util::WavLoader::load("../samples/snare.wav");
        Engine::Util::WavData hihat3Sample = Engine::Util::WavLoader::load("../samples/hihat.wav");
        Engine::Util::WavData kick3bisSample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData kick4Sample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        Engine::Util::WavData snare4Sample = Engine::Util::WavLoader::load("../samples/snare.wav");
        Engine::Util::WavData hihat4Sample = Engine::Util::WavLoader::load("../samples/hihat.wav");
        Engine::Util::WavData kick4bisSample  = Engine::Util::WavLoader::load("../samples/kick.wav");
        std::map<int, Engine::Util::WavData> sampleMap{
            {48, kick1Sample}, {50, snare1Sample}, {52, hihat1Sample}, {54, kick1bisSample}, {56, kick2Sample}, {58, snare2Sample}, {60, hihat2Sample}, {62, kick2bisSample}, {64, kick3Sample}, {66, snare3Sample}, {68, hihat3Sample}, {70, kick3bisSample}, {72, kick4Sample}, {74, snare4Sample}, {76, hihat4Sample}, {78, kick4bisSample}
        };

        for (auto& def : defs) {
            auto mod = Module::ModuleFactory::instance().create("poly_rhythm", this);
            auto poly = std::dynamic_pointer_cast<Module::PolyRhythmModule>(mod);

            // Each instrument gets its own initial shape
            poly->addShape({def.defaultSubdiv, 0.0, def.shapeName});

            // Single track per module
            Module::PolyRhythmTrack track;
            track.noteNumber = def.noteNumber;
            track.velocity   = def.velocity;
            track.name       = def.name;
            track.activePoints.resize(poly->getTimelineSize(), false);
            // Activate first point by default
            if (!track.activePoints.empty()) track.activePoints[0] = true;
            for (int i = 1; i < track.activePoints.size(); ++i) {
                track.activePoints[i] = true;
            }
            poly->setTracks({track});

            _audioGraph.addModule(mod);
            // Connect poly → sampler
            _audioGraph.connect(mod->getOutputPorts()[0], drumSampler->getInputPorts()[0]);

            PolyInstrument pi;
            pi.module = poly;
            pi.name   = def.name;
            pi.color  = def.color;
            pi.radius = def.radius;
            pi.shapeCounter = 2;  // Already created 1 shape (shapeName), next will be "Shape 2"
            _polyInstruments.push_back(std::move(pi));

            std::cout << "[AudioEngine] Created PolyRhythm for " << def.name
                      << " (note=" << def.noteNumber << ", n=" << def.defaultSubdiv << ")\n";
        }

        _selectedRing = 0;
        _selectedPoints.assign(_polyInstruments.size(), 0);

        // Wire sampler → audioOut
        _audioGraph.connect(drumSampler->getOutputPorts()[0], audioOutModule->getInputPorts()[0]);

        this->setParameter<std::map<int, Engine::Util::WavData>>(
            drumSampler->getId(), "Samples", sampleMap);


        // Config::InstrumentBuilder builder(Module::ModuleFactory::instance());
        // auto config_303 = Config::InstrumentParser::parseFromFile("../presets/tb_303.json");
        // auto config_808 = Config::InstrumentParser::parseFromFile("../presets/808_bass.json");
        // auto mix = Module::ModuleFactory::instance().create("mix", this);
        // try {
        //     auto instrumentModule = builder.build(config_303);
        //     auto instrument808Module = builder.build(config_808);
        //     auto audioOutModule = Module::ModuleFactory::instance().create("audio_out", this);

        //     auto stepSequencerModule303 = Module::ModuleFactory::instance().create("step_sequencer", this);
        //     Module::SequencerTrack highHatTrack;
        //     highHatTrack.noteNumber = 48;
        //     highHatTrack.velocity = 100;
        //     // highHatTrack.pattern = {true, true, true, false,
        //     //                         true, true, false, true,
        //     //                         false, true, false, true,
        //     //                         true, true, true, true};
        //     highHatTrack.pattern = {true, false, false, false,
        //                             true, false, false, false,
        //                             true, false, false, false,
        //                             true, false, false, false};
            
        //     auto stepSequencerModule808 = Module::ModuleFactory::instance().create("step_sequencer", this);
        //     Module::SequencerTrack bassTrack2;
        //     bassTrack2.noteNumber = 40; // E1
        //     bassTrack2.velocity = 100;
        //     // bassTrack2.pattern = {true, false, false, false,
        //     //                      true, false, true, false,
        //     //                     true, false, false, false,
        //     //                     true, false, true, false};
        //     bassTrack2.pattern = {false, false, false, true,
        //                          false, false, false, true,
        //                         false, false, false, true,
        //                         false, false, false, true};
        //     std::dynamic_pointer_cast<Module::StepSequencerModule>(stepSequencerModule303)->setTracks({ highHatTrack });
        //     std::dynamic_pointer_cast<Module::StepSequencerModule>(stepSequencerModule808)->setTracks({ bassTrack2 });

        //     _audioGraph.addModule(instrumentModule);
        //     _audioGraph.addModule(instrument808Module);
        //     _audioGraph.addModule(audioOutModule);
        //     _audioGraph.addModule(stepSequencerModule303);
        //     _audioGraph.addModule(stepSequencerModule808);
        //     _audioGraph.addModule(mix);

        //     _audioGraph.connect(stepSequencerModule303->getOutputPorts()[0], instrumentModule->getInputPorts()[0]);
        //     _audioGraph.connect(stepSequencerModule808->getOutputPorts()[0], instrument808Module->getInputPorts()[0]);
        //     _audioGraph.connect(instrumentModule->getOutputPorts()[0], mix->getInputPorts()[0]);
        //     _audioGraph.connect(instrument808Module->getOutputPorts()[0], mix->getInputPorts()[1]);
        //     _audioGraph.connect(mix->getOutputPorts()[0], audioOutModule->getInputPorts()[0]);

        // } catch (const std::exception& e) {
        //     std::cerr << "Error building instrument: " << e.what() << std::endl;
        //     return false;
        // }

        return true;
    }

    // ─── Helper: send full state for one ring to GUI ─────────────────────────
    static void sendRingData(IPC::SharedMemory& ipc, size_t ringIdx,
                             const AudioEngine::PolyInstrument& pi) {
        auto poly = pi.module;

        // POLY_RING:idx|name|color_hex|radius
        char colorBuf[8];
        snprintf(colorBuf, sizeof(colorBuf), "%06x", pi.color);
        std::string ringMsg = "POLY_RING:" + std::to_string(ringIdx) + "|" +
            pi.name + "|" + std::string(colorBuf) + "|" + std::to_string(pi.radius);
        ipc.send(ringMsg);

        // POLY_RING_SHAPE for each shape
        const auto& shapes = poly->getShapes();
        for (const auto& s : shapes) {
            std::string msg = "POLY_RING_SHAPE:" + std::to_string(ringIdx) + "|" +
                s.name + "|" + std::to_string(s.subdivision) + "|" +
                std::to_string(s.offset) + "|" + std::to_string(s.euclideanHits);
            ipc.send(msg);
        }

        // POLY_RING_TIMELINE
        const auto& tl = poly->getTimeline();
        std::string tlMsg = "POLY_RING_TIMELINE:" + std::to_string(ringIdx) + "|";
        for (size_t i = 0; i < tl.size(); ++i) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.6f", tl[i].phase);
            tlMsg += buf;
            if (i < tl.size() - 1) tlMsg += ",";
        }
        ipc.send(tlMsg);

        // POLY_RING_ACTIVE  (track 0 — there's exactly 1 track per module)
        const auto& tracks = poly->getTracks();
        if (!tracks.empty()) {
            std::string actMsg = "POLY_RING_ACTIVE:" + std::to_string(ringIdx) + "|";
            for (size_t i = 0; i < tracks[0].activePoints.size(); ++i) {
                actMsg += (tracks[0].activePoints[i] ? "1" : "0");
                if (i < tracks[0].activePoints.size() - 1) actMsg += ",";
            }
            ipc.send(actMsg);
        }
    }

    void AudioEngine::start() {
        // Start audio processing
        _isRunning = true;
        
        IPC::SharedMemory ipc_to_gui("TinyFirmwareSharedMemory_AudioToGUI", "audio");
        IPC::SharedMemory ipc_from_gui("TinyFirmwareSharedMemory_GUIToAudio", "audio");
        
        ipc_from_gui.startReaderThread();

        auto startsWith = [](const std::string& text, const std::string& prefix) {
            return text.rfind(prefix, 0) == 0;
        };

        auto modeToString = [](AudioEngine::HardwareMode mode) -> const char* {
            switch (mode) {
                case AudioEngine::HardwareMode::TogglePoint:   return "toggle";
                case AudioEngine::HardwareMode::AddRemovePoint:return "points";
                case AudioEngine::HardwareMode::Rotate:        return "rotate";
            }
            return "rotate";
        };

        auto safeRingIndex = [this](size_t ringIdx) -> size_t {
            if (_polyInstruments.empty()) {
                return 0;
            }
            return std::min(ringIdx, _polyInstruments.size() - 1);
        };

        auto clampSelectedPoint = [this](size_t ringIdx) {
            if (ringIdx >= _polyInstruments.size()) {
                return;
            }

            if (_selectedPoints.size() < _polyInstruments.size()) {
                _selectedPoints.resize(_polyInstruments.size(), 0);
            }

            const size_t pointCount = _polyInstruments[ringIdx].module->getTimelineSize();
            if (pointCount == 0) {
                _selectedPoints[ringIdx] = 0;
                return;
            }

            if (_selectedPoints[ringIdx] >= pointCount) {
                _selectedPoints[ringIdx] = pointCount - 1;
            }
        };

        auto sendSelectedPoint = [&](size_t ringIdx) {
            if (_polyInstruments.empty() || ringIdx >= _polyInstruments.size()) {
                return;
            }
            clampSelectedPoint(ringIdx);
            ipc_to_gui.send("POLY_SELECTED_POINT:" + std::to_string(ringIdx) + "|" +
                            std::to_string(_selectedPoints[ringIdx]));
        };

        auto sendHardwareMode = [&]() {
            ipc_to_gui.send("POLY_HW_MODE:" + std::string(modeToString(_hardwareMode)));
        };

        auto sendFullRebuild = [&](size_t ringToSelect) {
            ipc_to_gui.send("POLY_REBUILD");
            for (size_t i = 0; i < _polyInstruments.size(); ++i) {
                sendRingData(ipc_to_gui, i, _polyInstruments[i]);
            }

            if (!_polyInstruments.empty()) {
                _selectedRing = safeRingIndex(ringToSelect);
                clampSelectedPoint(_selectedRing);
                ipc_to_gui.send("POLY_SELECT:" + std::to_string(_selectedRing));
                sendSelectedPoint(_selectedRing);
            }
            sendHardwareMode();
            ipc_to_gui.send("POLY_READY");
        };
        
        std::cout << "[AudioEngine] Started and ready (" << _polyInstruments.size()
                  << " poly instruments)" << std::endl;
        
        while (_isRunning) {
            // Process messages from the GUI
            auto& queue = ipc_from_gui.getMessageQueue();
            while (auto msg = queue.tryPop()) {
                std::string message = msg.value();
                std::cout << "[AudioEngine] Received: " << message << std::endl;
                                
                if (message == "GET_INITIAL_SETUP") {
                    // Make setup idempotent on GUI side: always clear before replaying ring data.
                    ipc_to_gui.send("POLY_REBUILD");
                    // Send per-ring data
                    for (size_t i = 0; i < _polyInstruments.size(); ++i) {
                        sendRingData(ipc_to_gui, i, _polyInstruments[i]);
                    }
                    // BPM (take from first instrument)
                    if (!_polyInstruments.empty()) {
                        int bpm = static_cast<int>(_polyInstruments[0].module->getBPM());
                        ipc_to_gui.send("BPM:" + std::to_string(bpm));
                        _selectedRing = safeRingIndex(_selectedRing);
                        ipc_to_gui.send("POLY_SELECT:" + std::to_string(_selectedRing));
                        sendSelectedPoint(_selectedRing);
                    }
                    ipc_to_gui.send(std::string("PLAY_STATE:") + (_isPlaying ? "1" : "0"));
                    sendHardwareMode();
                    ipc_to_gui.send("POLY_READY");

                } else if (message == "PLAY") {
                    _isPlaying = true;
                    ipc_to_gui.send("PLAY_STATE:1");
                    std::cout << "[AudioEngine] Playing" << std::endl;
                } else if (message == "PAUSE") {
                    _isPlaying = false;
                    ipc_to_gui.send("PLAY_STATE:0");
                    std::cout << "[AudioEngine] Paused" << std::endl;

                } else if (message == "POLY_NAV_LEFT") {
                    ipc_to_gui.send("POLY_NAV_LEFT");
                    std::cout << "[AudioEngine] Forwarded POLY_NAV_LEFT" << std::endl;

                } else if (message == "POLY_NAV_RIGHT") {
                    ipc_to_gui.send("POLY_NAV_RIGHT");
                    std::cout << "[AudioEngine] Forwarded POLY_NAV_RIGHT" << std::endl;

                } else if (startsWith(message, "POLY_SET_SELECTED_RING:")) {
                    // POLY_SET_SELECTED_RING:ringIdx
                    if (!_polyInstruments.empty()) {
                        size_t ringIdx = std::stoull(message.substr(23));
                        _selectedRing = safeRingIndex(ringIdx);
                        clampSelectedPoint(_selectedRing);
                        ipc_to_gui.send("POLY_SELECT:" + std::to_string(_selectedRing));
                        sendSelectedPoint(_selectedRing);
                    }

                } else if (startsWith(message, "POLY_HW_MODE:")) {
                    // POLY_HW_MODE:toggle|points|rotate
                    std::string mode = message.substr(13);
                    std::transform(mode.begin(), mode.end(), mode.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    if (mode == "toggle" || mode == "toggle_point" || mode == "activation") {
                        _hardwareMode = HardwareMode::TogglePoint;
                    } else if (mode == "points" || mode == "add_remove" || mode == "addremove") {
                        _hardwareMode = HardwareMode::AddRemovePoint;
                    } else if (mode == "rotate") {
                        _hardwareMode = HardwareMode::Rotate;
                    }
                    sendHardwareMode();

                } else if (startsWith(message, "POLY_SELECT_POINT_DELTA:")) {
                    // POLY_SELECT_POINT_DELTA:amount
                    if (!_polyInstruments.empty()) {
                        int amount = std::stoi(message.substr(24));
                        _selectedRing = safeRingIndex(_selectedRing);
                        const size_t pointCount = _polyInstruments[_selectedRing].module->getTimelineSize();
                        if (pointCount > 0) {
                            long long next = static_cast<long long>(_selectedPoints[_selectedRing] % pointCount);
                            next += amount;
                            next %= static_cast<long long>(pointCount);
                            if (next < 0) {
                                next += static_cast<long long>(pointCount);
                            }
                            _selectedPoints[_selectedRing] = static_cast<size_t>(next);
                            sendSelectedPoint(_selectedRing);
                        }
                    }

                } else if (startsWith(message, "POLY_ENCODER_DELTA:")) {
                    // POLY_ENCODER_DELTA:amount
                    if (!_polyInstruments.empty()) {
                        int amount = std::stoi(message.substr(19));
                        if (amount != 0) {
                            if (_hardwareMode == HardwareMode::TogglePoint) {
                                _selectedRing = safeRingIndex(_selectedRing);
                                const size_t pointCount = _polyInstruments[_selectedRing].module->getTimelineSize();
                                if (pointCount > 0) {
                                    long long next = static_cast<long long>(_selectedPoints[_selectedRing] % pointCount);
                                    next += amount;
                                    next %= static_cast<long long>(pointCount);
                                    if (next < 0) {
                                        next += static_cast<long long>(pointCount);
                                    }
                                    _selectedPoints[_selectedRing] = static_cast<size_t>(next);
                                    sendSelectedPoint(_selectedRing);
                                }
                            } else if (_hardwareMode == HardwareMode::AddRemovePoint) {
                                _selectedRing = safeRingIndex(_selectedRing);
                                auto& poly = _polyInstruments[_selectedRing].module;
                                const auto& shapes = poly->getShapes();
                                if (!shapes.empty()) {
                                    Module::RhythmShape updated = shapes[0];
                                    int newSubdiv = static_cast<int>(updated.subdivision) + amount;
                                    if (newSubdiv < 1) newSubdiv = 1;
                                    if (newSubdiv > 32) newSubdiv = 32;
                                    if (newSubdiv != static_cast<int>(updated.subdivision)) {
                                        updated.subdivision = static_cast<uint32_t>(newSubdiv);
                                        poly->updateShape(0, updated);
                                        clampSelectedPoint(_selectedRing);
                                        sendFullRebuild(_selectedRing);
                                        std::cout << "[AudioEngine] Encoder(points) ring=" << _selectedRing
                                                  << " newSubdiv=" << newSubdiv << std::endl;
                                    }
                                }
                            } else {
                                _selectedRing = safeRingIndex(_selectedRing);
                                _polyInstruments[_selectedRing].module->rotateTrack(0, amount);
                                sendFullRebuild(_selectedRing);
                                std::cout << "[AudioEngine] Encoder(rotate) ring=" << _selectedRing
                                          << " by " << amount << std::endl;
                            }
                        }
                    }

                } else if (message == "POLY_ENCODER_CLICK") {
                    if (_hardwareMode == HardwareMode::TogglePoint && !_polyInstruments.empty()) {
                        _selectedRing = safeRingIndex(_selectedRing);
                        clampSelectedPoint(_selectedRing);
                        const size_t pointIdx = _selectedPoints[_selectedRing];
                        _polyInstruments[_selectedRing].module->togglePoint(0, pointIdx);
                        ipc_to_gui.send("POLY_SELECT:" + std::to_string(_selectedRing));
                        ipc_to_gui.send("POLY_TOGGLE:" + std::to_string(_selectedRing) + "|" + std::to_string(pointIdx));
                        sendSelectedPoint(_selectedRing);
                        std::cout << "[AudioEngine] Encoder click toggle ring=" << _selectedRing
                                  << " point=" << pointIdx << std::endl;
                    }

                } else if (message == "POLY_TOGGLE_SELECTED_POINT") {
                    if (!_polyInstruments.empty()) {
                        _selectedRing = safeRingIndex(_selectedRing);
                        clampSelectedPoint(_selectedRing);
                        const size_t pointIdx = _selectedPoints[_selectedRing];
                        _polyInstruments[_selectedRing].module->togglePoint(0, pointIdx);
                        ipc_to_gui.send("POLY_SELECT:" + std::to_string(_selectedRing));
                        ipc_to_gui.send("POLY_TOGGLE:" + std::to_string(_selectedRing) + "|" + std::to_string(pointIdx));
                        sendSelectedPoint(_selectedRing);
                        std::cout << "[AudioEngine] Toggle selected point ring=" << _selectedRing
                                  << " point=" << pointIdx << std::endl;
                    }

                } else if (startsWith(message, "POLY_ADJUST_POINTS:")) {
                    // POLY_ADJUST_POINTS:amount
                    if (!_polyInstruments.empty()) {
                        int amount = std::stoi(message.substr(19));
                        if (amount != 0) {
                            _selectedRing = safeRingIndex(_selectedRing);
                            auto& poly = _polyInstruments[_selectedRing].module;
                            const auto& shapes = poly->getShapes();
                            if (!shapes.empty()) {
                                Module::RhythmShape updated = shapes[0];
                                int newSubdiv = static_cast<int>(updated.subdivision) + amount;
                                if (newSubdiv < 1) newSubdiv = 1;
                                if (newSubdiv > 32) newSubdiv = 32;
                                if (newSubdiv != static_cast<int>(updated.subdivision)) {
                                    updated.subdivision = static_cast<uint32_t>(newSubdiv);
                                    poly->updateShape(0, updated);
                                    clampSelectedPoint(_selectedRing);
                                    sendFullRebuild(_selectedRing);
                                    std::cout << "[AudioEngine] Adjust points ring=" << _selectedRing
                                              << " newSubdiv=" << newSubdiv << std::endl;
                                }
                            }
                        }
                    }

                } else if (startsWith(message, "POLY_ROTATE_SELECTED:")) {
                    // POLY_ROTATE_SELECTED:amount
                    if (!_polyInstruments.empty()) {
                        int amount = std::stoi(message.substr(21));
                        if (amount != 0) {
                            _selectedRing = safeRingIndex(_selectedRing);
                            _polyInstruments[_selectedRing].module->rotateTrack(0, amount);
                            sendFullRebuild(_selectedRing);
                            std::cout << "[AudioEngine] Rotated selected ring " << _selectedRing
                                      << " by " << amount << std::endl;
                        }
                    }

                } else if (message.substr(0, 12) == "POLY_TOGGLE:") {
                    // POLY_TOGGLE:ringIdx|pointIdx
                    std::string data = message.substr(12);
                    size_t sep = data.find('|');
                    if (sep != std::string::npos) {
                        size_t ringIdx  = std::stoull(data.substr(0, sep));
                        size_t pointIdx = std::stoull(data.substr(sep + 1));
                        if (ringIdx < _polyInstruments.size()) {
                            _polyInstruments[ringIdx].module->togglePoint(0, pointIdx);
                            _selectedRing = ringIdx;
                            if (_selectedPoints.size() < _polyInstruments.size()) {
                                _selectedPoints.resize(_polyInstruments.size(), 0);
                            }
                            _selectedPoints[ringIdx] = pointIdx;
                            clampSelectedPoint(_selectedRing);
                            sendSelectedPoint(_selectedRing);
                            std::cout << "[AudioEngine] Toggle ring=" << ringIdx
                                      << " point=" << pointIdx << std::endl;
                        }
                    }

                } else if (message.substr(0, 15) == "POLY_ADD_SHAPE:") {
                    // POLY_ADD_SHAPE:ringIdx|subdivision|offset
                    std::string data = message.substr(15);
                    size_t p1 = data.find('|');
                    size_t p2 = data.find('|', p1 + 1);
                    if (p1 != std::string::npos && p2 != std::string::npos) {
                        size_t ringIdx  = std::stoull(data.substr(0, p1));
                        uint32_t subdiv = static_cast<uint32_t>(std::stoul(data.substr(p1 + 1, p2 - p1 - 1)));
                        double offset   = std::stod(data.substr(p2 + 1));
                        if (ringIdx < _polyInstruments.size()) {
                            Module::RhythmShape shape;
                            shape.subdivision = subdiv;
                            shape.offset = offset;
                            shape.name = "Shape " + std::to_string(
                                _polyInstruments[ringIdx].shapeCounter++);
                            _polyInstruments[ringIdx].module->addShape(shape);

                            sendFullRebuild(ringIdx);
                            std::cout << "[AudioEngine] Shape added to ring " << ringIdx << std::endl;
                        }
                    }

                } else if (message.substr(0, 18) == "POLY_REMOVE_SHAPE:") {
                    // POLY_REMOVE_SHAPE:ringIdx|shapeIdx
                    std::string data = message.substr(18);
                    size_t sep = data.find('|');
                    if (sep != std::string::npos) {
                        size_t ringIdx  = std::stoull(data.substr(0, sep));
                        size_t shapeIdx = std::stoull(data.substr(sep + 1));
                        if (ringIdx < _polyInstruments.size()) {
                            _polyInstruments[ringIdx].module->removeShape(shapeIdx);
                            sendFullRebuild(ringIdx);
                            std::cout << "[AudioEngine] Shape " << shapeIdx
                                      << " removed from ring " << ringIdx << std::endl;
                        }
                    }

                } else if (message.substr(0, 15) == "POLY_MOD_SHAPE:") {
                    // POLY_MOD_SHAPE:ringIdx|shapeIdx|newSubdiv|newOffset
                    std::string data = message.substr(15);
                    size_t p1 = data.find('|');
                    size_t p2 = data.find('|', p1 + 1);
                    size_t p3 = data.find('|', p2 + 1);
                    if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos) {
                        size_t ringIdx  = std::stoull(data.substr(0, p1));
                        size_t shapeIdx = std::stoull(data.substr(p1 + 1, p2 - p1 - 1));
                        uint32_t newSub = static_cast<uint32_t>(std::stoul(data.substr(p2 + 1, p3 - p2 - 1)));
                        double newOff   = std::stod(data.substr(p3 + 1));
                        if (ringIdx < _polyInstruments.size()) {
                            auto& poly = _polyInstruments[ringIdx].module;
                            if (shapeIdx < poly->getShapes().size()) {
                                Module::RhythmShape updated = poly->getShapes()[shapeIdx];
                                updated.subdivision = newSub;
                                updated.offset = newOff;
                                poly->updateShape(shapeIdx, updated);

                                sendFullRebuild(ringIdx);
                                std::cout << "[AudioEngine] Shape " << shapeIdx
                                          << " modified on ring " << ringIdx
                                          << " (n=" << newSub << ")" << std::endl;
                            }
                        }
                    }

                } else if (message.substr(0, 12) == "POLY_ROTATE:") {
                    // POLY_ROTATE:ringIdx|amount
                    std::string data = message.substr(12);
                    size_t sep = data.find('|');
                    if (sep != std::string::npos) {
                        size_t ringIdx = std::stoull(data.substr(0, sep));
                        int amount     = std::stoi(data.substr(sep + 1));
                        if (ringIdx < _polyInstruments.size()) {
                            _polyInstruments[ringIdx].module->rotateTrack(0, amount);
                            sendFullRebuild(ringIdx);
                            std::cout << "[AudioEngine] Rotated ring " << ringIdx
                                      << " by " << amount << std::endl;
                        }
                    }
                }
            }
            
            // Process commands from internal queue
            this->processAll();
            
            // Process audio graph only if playing
            if (this->_isPlaying) {
                this->process();
                
                // Send playhead — all modules share the same BPM so use first one's phase
                if (!_polyInstruments.empty()) {
                    int bpm = static_cast<int>(_polyInstruments[0].module->getBPM());
                    double phase = _polyInstruments[0].module->getCurrentPhase();
                    // Shift the phase backward according to the BPM (e.g., 20 ms compensation)
                    // 20 ms = 0.02 s, fraction of the cycle = 0.02 / (60.0 / bpm)
                    double compensationMs = 20.0;
                    double compensation = (compensationMs / 1000.0) / (60.0 / bpm);
                    phase -= compensation;
                    if (phase < 0.0) phase += 1.0; // wrap-around

                    char phaseBuf[32];
                    snprintf(phaseBuf, sizeof(phaseBuf), "%.6f", phase);
                    std::string msg = "POLY_PLAYHEAD:" + std::string(phaseBuf) + "|" +
                                     std::to_string(bpm);
                    ipc_to_gui.send(msg);
                }
            }
        }
        
        ipc_from_gui.stopReaderThread();
        std::cout << "[AudioEngine] Stopped" << std::endl;
    }

    void AudioEngine::stop() {
        // Stop audio processing
        _isRunning = false;
    }

    void AudioEngine::process() {
        // Process audio data through the audio graph
        _audioGraph.process();
    }

    AudioGraph& AudioEngine::getScopedAudioGraph(unsigned int scope) {
        if (scope == 0) {
            return _audioGraph;
        }
        auto modules = _audioGraph.getModules();
        for (const auto& module : modules) {
            if (module->getName().find("instrument_" + std::to_string(scope)) != std::string::npos) {
                auto instrumentModule = std::dynamic_pointer_cast<Module::Instrument>(module);
                if (instrumentModule) {
                    return instrumentModule->getAudioGraph();
                }
            }
        }
        return _audioGraph;
    }

    void AudioEngine::handle(std::unique_ptr<Common::ICommand>& cmd) {
        if (cmd) cmd->apply(*this);
    }


    void AudioEngine::createConnection(const std::string& srcModuleId, const std::string& srcPort,
                                       const std::string& dstModuleId, const std::string& dstPort, unsigned int scope) {
        auto modules = getScopedAudioGraph(scope).getModules();
        std::shared_ptr<Port::OutputPort> outputPort = nullptr;
        std::shared_ptr<Port::InputPort> inputPort = nullptr;

        for (const auto& module : modules) {
            if (module->getName().find(srcModuleId) != std::string::npos) {
                outputPort = module->getOutputPortByName(srcPort);
                if (outputPort) {
                    std::cerr << "Found source port: " << srcPort << std::endl;
                }
            }
            if (module->getName().find(dstModuleId) != std::string::npos) {
                inputPort = module->getInputPortByName(dstPort);
                if (inputPort) {
                    std::cerr << "Found destination port: " << dstPort << std::endl;
                }
            }
        }

        if (outputPort && inputPort) {
            getScopedAudioGraph(scope).connect(outputPort, inputPort);
        } else {
            std::cerr << "Error creating connection: Invalid module ID or port name." << std::endl;
        }
    }

    void AudioEngine::removeConnection(const std::string& srcModuleId, const std::string& srcPort,
                                       const std::string& dstModuleId, const std::string& dstPort, unsigned int scope) {
        auto modules = getScopedAudioGraph(scope).getModules();
        std::shared_ptr<Port::OutputPort> outputPort = nullptr;
        std::shared_ptr<Port::InputPort> inputPort = nullptr;

        for (const auto& module : modules) {
            if (module->getName().find(srcModuleId) != std::string::npos) {
                outputPort = module->getOutputPortByName(srcPort);
                if (outputPort) {
                    std::cerr << "Found source port: " << srcPort << std::endl;
                }
            }
            if (module->getName().find(dstModuleId) != std::string::npos) {
                inputPort = module->getInputPortByName(dstPort);
                if (inputPort) {
                    std::cerr << "Found destination port: " << dstPort << std::endl;
                }
            }
        }

        if (outputPort && inputPort) {
            getScopedAudioGraph(scope).disconnect(outputPort, inputPort);
        } else {
            std::cerr << "Error removing connection: Invalid module ID or port name." << std::endl;
        }
    }


} // namespace Engine