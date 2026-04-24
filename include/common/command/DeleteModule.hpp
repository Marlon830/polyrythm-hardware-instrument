/*
    @file DeleteModule.hpp
    @brief Declaration of the DeleteModule command for deleting modules.
    @ingroup common_command
*/

#ifndef COMMON_COMMAND_DELETEMODULE_HPP
    #define COMMON_COMMAND_DELETEMODULE_HPP
    #include "common/command/ICommand.hpp"
    #include "engine/AudioEngine.hpp"

    #include "common/event/EmitModuleList.hpp"

namespace Common {
    /// @brief Command to delete a module from the audio graph.
    /// @ingroup common_command
    struct DeleteModuleCommand : ICommand {
        /// @brief Construct a new DeleteModuleCommand
        /// @param moduleId The ID of the module to delete.
        /// @param scope Scope for emitting updated module list
        DeleteModuleCommand(int moduleId, unsigned int scope = 0)
            : moduleId(moduleId), _scope(scope) {}

        int moduleId;
        unsigned int _scope;

        /// @brief Apply the command to the given AudioEngine.
        /// @param engine The AudioEngine to apply the command to.
        void apply(Engine::AudioEngine& engine) override {
            try {
                engine.getScopedAudioGraph(_scope).removeModule(moduleId);
            } catch (const std::exception& e) {
                std::cerr << "Error deleting module with ID: " << moduleId << " - " << e.what() << std::endl;
                return;
            }

            std::vector<std::string> modules;
            std::map<std::string, Common::ModuleInfo> allParams = engine.getAudioGraph().getScopedParams(_scope);
            for (const auto& [name, _] : allParams) {
                modules.push_back(name);
            }
            engine.emit(std::make_unique<EmitModuleListEvent>(allParams));
            engine.emit(std::make_unique<EmitConnectionListEvent>(engine.getAudioGraph().getConnectionInfos(_scope), modules));
        }
    };
} // namespace Common
#endif // COMMON_COMMAND_DELETEMODULE_HPP