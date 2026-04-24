#include "engine/module/EventEmitter.hpp"
#include "engine/module/ModuleFactory.hpp"

#include <iostream>

using namespace std::chrono;

namespace Engine {
    namespace Module {

        EventEmitter::EventEmitter() : _time(std::make_shared<Param<int>>("Time", 1)),
                                     _note(std::make_shared<Param<int>>("Note", 60)) {
            
            _name = "EventEmitterModule";
            auto out = std::make_shared<Port::OutputPort>("Event Emitter Output", Signal::SignalType::EVENT);
            _outputPorts.push_back(out);
            _parameters.push_back(_time);
            _parameters.push_back(_note);
        }

        EventEmitter::EventEmitter(std::string& name) : _time(std::make_shared<Param<int>>("Time", 1)),
                                     _note(std::make_shared<Param<int>>("Note", 60)) {
            
            _name = std::move(name);
            auto out = std::make_shared<Port::OutputPort>("Event Emitter Output", Signal::SignalType::EVENT);
            _outputPorts.push_back(out);
            _parameters.push_back(_time);
            _parameters.push_back(_note);
        }

        EventEmitter::~EventEmitter() {
        }

        IModule* EventEmitter::clone() const {
            return new EventEmitter(*this);
        }

        void EventEmitter::process(Core::AudioContext& context) {
            using clock = std::chrono::steady_clock;
            auto now = clock::now();
            const auto Duration = std::chrono::seconds(_time->get());

            if (_lastToggle == clock::time_point{}) {
                _lastToggle = now - Duration;
            }

            if (now - _lastToggle >= Duration) {
                //std::cout << "EventEmitter: Emitting "<< (_isOn ? "NOTE_OFF" : "NOTE_ON") << " for note " << _note->get() << std::endl;
                auto eventSignal = std::make_shared<Signal::EventSignal>(
                    _isOn ? Signal::EventType::NOTE_OFF : Signal::EventType::NOTE_ON,
                    0, _note->get(), 100
                );
                _outputPorts[0]->send(eventSignal);
                _lastToggle = now;
                _isOn = !_isOn;
            }
        }

        static AutoRegister eventEmitterModuleReg{
            "event_emitter",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<EventEmitter>(name);
            }
        };


    } // namespace Module
} // namespace Engine