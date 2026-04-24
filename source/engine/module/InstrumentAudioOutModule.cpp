#include "engine/module/InstrumentAudioOutModule.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"

namespace Engine {
    namespace Module {

        InstrumentAudioOutModule::InstrumentAudioOutModule()
            : InstrumentAudioOutModule("InstrumentAudioOutModule") {
        }

        InstrumentAudioOutModule::InstrumentAudioOutModule(std::string name) {
            _name = std::move(name);
            auto inputPort = std::make_shared<Port::InputPort>("in", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort);
            auto outputPort = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);
            _outputPorts.push_back(outputPort);
        }

        InstrumentAudioOutModule::~InstrumentAudioOutModule() {
        }

        IModule* InstrumentAudioOutModule::clone() const {
            return new InstrumentAudioOutModule(*this);
        }

        void InstrumentAudioOutModule::process(Core::AudioContext& context) {
            auto signal = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(_inputPorts[0]->get());
            if (signal) {
                _outputPorts[0]->send(signal);
            } else {
                // Send silent signal if no input
                PCMBuffer silentBuffer(context.bufferSize, 0.0);
                auto silentSignal = std::make_shared<Engine::Signal::AudioSignal>(silentBuffer, context.bufferSize);
                _outputPorts[0]->send(silentSignal);
            }
        }

        static AutoRegister instrumentAudioOutModuleReg{
            "instrument_audio_out",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<InstrumentAudioOutModule>(name);
            }
        };
    } // namespace Module
} // namespace Engine