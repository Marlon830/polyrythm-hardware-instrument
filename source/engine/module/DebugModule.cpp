#include "engine/module/DebugModule.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/module/ModuleFactory.hpp"
#include <iostream>

namespace Engine {
    namespace Module {

        DebugModule::DebugModule() {
            _name = "DebugModule";
            std::shared_ptr<Port::InputPort> inputPort = std::make_shared<Port::InputPort>("Debug Input", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort);
        }

        DebugModule::DebugModule(std::string name) {
            _name = std::move(name);
            std::shared_ptr<Port::InputPort> inputPort = std::make_shared<Port::InputPort>("Debug Input", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort);
        }

        DebugModule::~DebugModule() {

        }

        IModule* DebugModule::clone() const {
            return new DebugModule(*this);
        }

        void DebugModule::process(Core::AudioContext& context) {
            // For debugging purposes, simply log the reception of audio signals

            if (_inputPorts.empty() || !_inputPorts[0]) {
                std::cerr << "DebugModule: no input port 0 available\n";
                return;
            }
            std::shared_ptr<Signal::ISignal> signal = _inputPorts[0]->get();

            if (signal && signal->getType() == Signal::SignalType::AUDIO) {
                auto audioSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(signal);
                if (audioSignal) {
                    const PCMBuffer& buffer = audioSignal->getBuffer();
                    for (size_t i = 0; i < buffer.size(); ++i) {
                        // Log each sample value (in a real scenario, consider performance implications)
                        std::cout << "DebugModule Sample [" << i << "]: " << buffer[i] << std::endl;
                    }
                }
            }
        }

        static AutoRegister debugModuleReg{
            "debug",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<DebugModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine
