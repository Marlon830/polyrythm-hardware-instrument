#ifndef COMMON_COMMAND_CREATE_MODULE_HPP
#define COMMON_COMMAND_CREATE_MODULE_HPP

#include "common/command/ICommand.hpp"
#include "engine/AudioEngine.hpp"
#include "engine/module/ModuleFactory.hpp"
#include <string>

namespace Common {
    /// @brief Command to create a new module in the audio engine.
    /// @ingroup common_command
    struct CreateModuleCommand : ICommand {
        /// @brief Construct a new CreateModuleCommand
        /// @param type The type of the module to create
        /// @param scope Scope for emitting updated module list
        CreateModuleCommand(const std::string& type, unsigned int scope = 0)
            : moduleType(type), _scope(scope) {}

        std::string moduleType;
        unsigned int _scope;

        /// @brief Apply the command to the given AudioEngine.
        /// @param engine The AudioEngine to apply the command to.
        void apply(Engine::AudioEngine& engine) override {
            try {
                auto module = Engine::Module::ModuleFactory::instance().create(moduleType, &engine);
                if (module) {
                    engine.getScopedAudioGraph(_scope).addModule(module);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error creating module of type: " << moduleType << " - " << e.what() << std::endl;
                return;
            }

            std::vector<std::string> modules;
            std::map<std::string, Common::ModuleInfo> allParams = engine.getAudioGraph().getScopedParams(_scope);
            for (const auto& [name, _] : allParams) {
                modules.push_back(name);
            }
            engine.emit(std::make_unique<EmitModuleListEvent>(engine.getAudioGraph().getScopedParams(_scope)));
            engine.emit(std::make_unique<EmitConnectionListEvent>(engine.getAudioGraph().getConnectionInfos(_scope), modules));
        }
    };
} // namespace Common
#endif // COMMON_COMMAND_CREATE_MODULE_HPP