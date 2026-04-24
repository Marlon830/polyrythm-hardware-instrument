#include "engine/module/ArpegiatorModule.hpp"
#include "engine/module/ModuleFactory.hpp"

#include <iostream>

using namespace std::chrono;

namespace Engine {
    namespace Module {

        ArpegiatorModule::ArpegiatorModule() : _timeInterval(std::make_shared<Param<float>>("TInt", 500.0f)),
                                               _noteSequence(std::make_shared<Param<std::vector<int>>>("NoteSeq", std::vector<int>{60, 64, 67})) {
            _name = "arpegiator";
            auto out = std::make_shared<Port::OutputPort>("Arpeggiator Output", Signal::SignalType::EVENT);
            _outputPorts.push_back(out);
            _parameters.push_back(_timeInterval);
            _parameters.push_back(_noteSequence);
        }

        ArpegiatorModule::ArpegiatorModule(std::string& name) : _timeInterval(std::make_shared<Param<float>>("TInt", 500.0f)),
                                               _noteSequence(std::make_shared<Param<std::vector<int>>>("NoteSeq", std::vector<int>{60, 64, 67})) {
            _name = std::move(name);
            auto out = std::make_shared<Port::OutputPort>("Arpeggiator Output", Signal::SignalType::EVENT);
            _outputPorts.push_back(out);
            _parameters.push_back(_timeInterval);
            _parameters.push_back(_noteSequence);
        }

        ArpegiatorModule::~ArpegiatorModule() {
        }

        IModule* ArpegiatorModule::clone() const {
            return new ArpegiatorModule(*this);
        }

        void ArpegiatorModule::process(Core::AudioContext& context) {
            using namespace std::chrono;
            auto now = steady_clock::now();

            // TimeInterval now exprimé en millisecondes (float)
            float intervalMs = _timeInterval->get();
            duration<float, std::milli> intervalDuration(intervalMs);

            // Si premier appel, initialise le timer et quitte (évite un grand saut)
            if (_lastEventTime == steady_clock::time_point{}) {
                _lastEventTime = now;
                return;
            }

            if (now - _lastEventTime >= intervalDuration) {
                const auto& sequence = _noteSequence->get();
                if (!sequence.empty()) {
                    if (_lastNote != -1) {
                        auto eventOff = std::make_shared<Signal::EventSignal>(
                            Signal::EventType::NOTE_OFF,
                            0, _lastNote, 100
                        );
                        _outputPorts[0]->send(eventOff);
                    }

                    int note = sequence[_currentNoteIndex % sequence.size()];
                    //std::cout << "ArpegiatorModule: Emitting NOTE_ON for note " << note << std::endl;
                    auto eventOn = std::make_shared<Signal::EventSignal>(
                        Signal::EventType::NOTE_ON,
                        0, note, 100
                    );
                    _outputPorts[0]->send(eventOn);
                    _currentNoteIndex++;
                    _lastNote = note;
                }
                _lastEventTime = now;
            }
        }

        static AutoRegister arpegiatorModuleReg{
            "arpegiator",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<ArpegiatorModule>(name);
            }
        };
    } // namespace Module
} // namespace Engine