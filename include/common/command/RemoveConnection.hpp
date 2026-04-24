/*
@file RemoveConnection.hpp
@brief Declaration of the RemoveConnection command for removing connections.
@ingroup common_command
*/
#ifndef COMMON_COMMAND_REMOVECONNECTION_HPP
#define COMMON_COMMAND_REMOVECONNECTION_HPP

#include "common/command/ICommand.hpp"
#include "engine/AudioEngine.hpp"
#include "common/event/EmitConnectionList.hpp"

namespace Common {
    /// @brief Command to remove a connection between two module ports.
    /// @ingroup common_command
    struct RemoveConnectionCommand : ICommand {
        /// @brief Construct a new RemoveConnectionCommand
        /// @param source Source module ID
        /// @param outPort Source output port ID
        /// @param destination Destination module ID
        /// @param inPort Destination input port ID
        /// @param scope Scope for emitting updated connection list
        RemoveConnectionCommand(const std::string& source, const std::string& outPort,
                                const std::string& destination, const std::string& inPort, unsigned int scope = 0)
            : sourceId(source), outPortId(outPort), destinationId(destination), inPortId(inPort), _scope(scope) {}

        std::string sourceId;
        std::string outPortId;
        std::string destinationId;
        std::string inPortId;
        unsigned int _scope;

        /// @brief Apply the command to the given AudioEngine.
        /// @param engine The AudioEngine to apply the command to.
        void apply(Engine::AudioEngine& engine) override {
            engine.removeConnection(sourceId, outPortId, destinationId, inPortId, _scope);

            std::vector<std::string> modules;
            std::map<std::string, Common::ModuleInfo> allParams = engine.getAudioGraph().getScopedParams(_scope);
            for (const auto& [name, _] : allParams) {
                modules.push_back(name);
            }
            engine.emit(std::make_unique<EmitConnectionListEvent>(engine.getAudioGraph().getConnectionInfos(_scope), modules));
        }
    };
} // namespace Common

#endif // COMMON_COMMAND_REMOVECONNECTION_HPP