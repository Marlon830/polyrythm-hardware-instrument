#include "engine/module/BaseModule.hpp"
#include <stdio.h>
#include <vector>
#include <memory>

namespace Engine {
    namespace Module {
        BaseModule::BaseModule(): _name("BaseModule") {}

        BaseModule::BaseModule(std::string name) : _name(std::move(name)) {}

        BaseModule::~BaseModule() = default;

        IModule* BaseModule::clone() const {
            return new BaseModule(*this);
        }

        void BaseModule::process(Core::AudioContext& context) {
            // Default implementation does nothing
            printf("Processing audio context in BaseModule\n");
        }

        std::vector<std::shared_ptr<Port::InputPort>>& BaseModule::getInputPorts() {
            return _inputPorts;
        }

        std::vector<std::shared_ptr<Port::OutputPort>>& BaseModule::getOutputPorts() {
            return _outputPorts;
        }

        std::shared_ptr<Port::InputPort> BaseModule::getInputPortByName(const std::string& name) {
            for (const auto& port : _inputPorts) {
                if (port->getName() == name) {
                    return port;
                }
            }
            return nullptr;
        }

        std::shared_ptr<Port::OutputPort> BaseModule::getOutputPortByName(const std::string& name) {
            for (const auto& port : _outputPorts) {
                if (port->getName() == name) {
                    return port;
                }
            }
            return nullptr;
        }
        
        void BaseModule::setEventEmitter(Core::IEventEmitter* emitter) {
            _eventEmitter = emitter;
        }
    }
}