#ifndef COMMON_COMMAND_GET_MODULE_LIST_HPP
#define COMMON_COMMAND_GET_MODULE_LIST_HPP

#include "common/command/ICommand.hpp"
#include "engine/AudioEngine.hpp"
#include "common/event/EmitModuleList.hpp"
#include <string>

namespace Common {
    /// @brief Command to get the list of available modules.
    /// @details This command can be used to request the list of all available
    /// modules in the system. The response will contain the module names.
    class GetModuleListCommand : public ICommand {
    public:
        /// @brief Construct a new GetModuleListCommand.
        GetModuleListCommand(unsigned int scope = 0)
            : _scope(scope) {}

        /// @brief Destroy the GetModuleListCommand.
        ~GetModuleListCommand() override = default;

        unsigned int _scope;

        /// @brief Execute the command.
        /// @note Implementation will depend on the system architecture.
        void apply(Engine::AudioEngine& engine) override {
            auto moduleList = engine.getAudioGraph().getScopedParams(_scope);
            engine.emit(std::make_unique<EmitModuleListEvent>(moduleList));
        }
    };
} // namespace Common

#endif // COMMON_COMMAND_GET_MODULE_LIST_HPP