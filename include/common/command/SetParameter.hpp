#ifndef COMMON_COMMAND_SETPARAMETER_HPP
#define COMMON_COMMAND_SETPARAMETER_HPP

#include "common/command/ICommand.hpp"
#include "common/event/MessageEvent.hpp"
#include <string>

namespace Common {
    /// @brief Command to set a parameter in the audio engine.  
    /// @tparam T 
    template<typename T>
    struct SetParameterCommand : ICommand {
        SetParameterCommand(unsigned i, std::string n, T v)
            : id(i), name(std::move(n)), value(v) {}

        unsigned id;
        std::string name;
        T value;

        void apply(Engine::AudioEngine& engine) override {
            if(engine.template setParameter<T>(static_cast<int>(id), name, value)) {
                engine.emit(std::make_unique<Common::MessageEvent>(
                    Common::logType::INFO, "Set parameter '" + name + "' of module " + std::to_string(id) + "."));
            } else {
                engine.emit(std::make_unique<Common::MessageEvent>(
                    Common::logType::ERROR, "Failed to set parameter '" + name + "' of module " + std::to_string(id) + "."));
            }
        }
    };
} // namespace Common
#endif // COMMON_COMMAND_SETPARAMETER_HPP