#ifndef COMMON_COMMAND_GETCONNECTIONLIST_HPP
#define COMMON_COMMAND_GETCONNECTIONLIST_HPP
#include "common/command/ICommand.hpp"
#include "engine/AudioEngine.hpp"
#include "common/event/EmitConnectionList.hpp"
#include <string>

namespace Common {
    /// @brief Command to get the list of current connections in the audio graph.
    /// @details This command can be used to request the list of all current
    /// connections between modules in the audio graph. The response will contain
    /// the connection details.
    class GetConnectionListCommand : public ICommand {
    public:
        /// @brief Construct a new GetConnectionListCommand.
        GetConnectionListCommand(unsigned int scope = 0) 
            : _scope(scope) {}

        /// @brief Destroy the GetConnectionListCommand.
        ~GetConnectionListCommand() override = default;

        unsigned int _scope;

        /// @brief Execute the command.
        /// @note Implementation will depend on the system architecture.
        void apply(Engine::AudioEngine& engine) override {
            auto connectionList = engine.getAudioGraph().getConnectionInfos(_scope);
            for (const auto& conn : connectionList) {
                std::cerr << "Connection: " << conn->sourceModule << ":" << conn->sourcePort
                          << " -> " << conn->destModule << ":" << conn->destPort
                          << " (Type: " << conn->connectionType << ")" << std::endl;
            }
            std::vector<std::string> modules;
            std::map<std::string, Common::ModuleInfo> allParams = engine.getAudioGraph().getScopedParams(_scope);
            for (const auto& [name, _] : allParams) {
                // Here we would need a way to get the actual module instances from their names
                    modules.push_back(name);
                }
            engine.emit(std::make_unique<EmitConnectionListEvent>(connectionList, modules));
        }
    };
} // namespace Common
#endif // COMMON_COMMAND_GETCONNECTIONLIST_HPP