/// @file AudioGraph.hpp
/// @brief Audio graph manager implementing the Mediator pattern
/// @ingroup engine_audiograph
/// @details AudioGraph manages the connections between audio modules and ensures
/// proper execution order through topological sorting. It acts as a mediator to
/// avoid direct dependencies between modules.

#ifndef AUDIOGRAPH_HPP
#define AUDIOGRAPH_HPP

#include "engine/port/Connection.hpp"
#include "engine/module/IModule.hpp"

#include "engine/parameter/ParamBase.hpp"
#include "engine/parameter/Param.hpp"
#include "common/event/EmitModuleList.hpp"
#include <vector>
#include <map>

namespace Engine
{
    /// @brief Manages audio modules and their connections
    /// @details This class implements the Mediator design pattern to coordinate
    /// communication between audio modules without creating direct dependencies.
    /// It maintains a directed acyclic graph (DAG) of modules and performs
    /// topological sorting to determine the correct execution order.
    /// @ingroup engine_audiograph
    class AudioGraph
    {
    public:
        /// @brief Constructs an empty AudioGraph
        AudioGraph();
        
        /// @brief Destroys the AudioGraph and releases all resources
        virtual ~AudioGraph();

        AudioGraph* clone() const;

        /// @brief Adds a module to the audio graph
        /// @param module Shared pointer to the module to add
        /// @throws std::invalid_argument if module is null
        /// @note Automatically updates the execution order after adding
        /// @see updateExecutionOrder()
        void addModule(std::shared_ptr<Module::IModule> module);

        /// @brief Removes a module from the audio graph by its ID
        /// @param moduleId The ID of the module to remove
        /// @throws std::invalid_argument if moduleId is not found
        /// @note Automatically updates the execution order after removal
        /// @see updateExecutionOrder()
        void removeModule(int moduleId);
        
        /// @brief Connects an output port to an input port
        /// @param src Source output port
        /// @param dst Destination input port
        /// @throws std::invalid_argument if either port is null
        /// @note Creates a Connection object and updates execution order
        /// @see Port::Connection
        void connect(std::shared_ptr<Port::OutputPort> src, std::shared_ptr<Port::InputPort> dst);

        /// @brief Removes a connection between two ports
        /// @param src Source output port
        /// @param dst Destination input port
        /// @throws std::invalid_argument if connection does not exist
        /// @note Updates execution order after removal
        /// @see updateExecutionOrder()
        void disconnect(std::shared_ptr<Port::OutputPort> src, std::shared_ptr<Port::InputPort> dst);
        
        /// @brief Processes all modules in topological order
        /// @details Creates an AudioContext and executes each module's process()
        /// method in the cached execution order. The order guarantees that
        /// dependencies are resolved correctly.
        /// @note Uses default audio context: bufferSize=512, sampleRate=44100
        /// @see Core::AudioContext
        void process();

        /// @brief Sets the audio context for processing
        /// @param context The audio context to set
        void setContext(const Core::AudioContext& context) { _audioContext = context; }

        /// @brief Gets the current audio context
        /// @return The current audio context
        Core::AudioContext getContext() const { return _audioContext; }

        /// @brief Gets the list of all modules in the graph
        /// @return Vector of shared pointers to all modules
        std::vector<std::shared_ptr<Module::IModule>> getModules() const { return _modules; }

        std::shared_ptr<Module::IModule> getModuleByName(const std::string& name) const;

        std::map<std::string, Common::ModuleInfo> getAllParams() const;

        std::map<std::string, Common::ModuleInfo> getScopedParams(unsigned int scope) const;

        /// @brief Gets the list of all connections in the graph
        /// @return Vector of shared pointers to all connections
        std::vector<std::shared_ptr<Port::Connection>> getConnections() const { return _connections; }

        std::vector<std::shared_ptr<Engine::ConnectionInfo>> getConnectionInfos(unsigned int scope) const;

    private:
        /// @brief Computes topological sort of modules
        /// @return Vector of modules in dependency-resolved order
        /// @throws std::runtime_error if a cycle is detected in the graph
        /// @details Implements Kahn's algorithm for topological sorting.
        /// Modules with no dependencies are processed first.
        std::vector<std::shared_ptr<Module::IModule>> topologicalSort();
        
        /// @brief Updates the cached execution order
        /// @details Calls topologicalSort() and caches the result.
        /// Called automatically when modules or connections are added.
        void updateExecutionOrder();

        /// @brief List of all modules in the graph
        std::vector<std::shared_ptr<Module::IModule>> _modules;
        
        /// @brief List of all connections between modules
        /// @details Each connection links an OutputPort to an InputPort
        std::vector<std::shared_ptr<Port::Connection>> _connections;
        
        /// @brief Cached topologically sorted execution order
        /// @details Updated by updateExecutionOrder() whenever the graph changes
        std::vector<std::shared_ptr<Module::IModule>> _executionOrder;

        /// @brief Current audio context for processing
        Core::AudioContext _audioContext;
        
        // TODO: IModuleFactory and VoiceManager can be added here later
    };
}

#endif // AUDIOGRAPH_HPP
