/// @file Connection.hpp
/// @brief Connection between output and input ports
/// @ingroup engine_port
/// @details A Connection represents a directed edge in the audio graph,
/// linking an OutputPort of one module to an InputPort of another module.

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "engine/port/OutputPort.hpp"
#include "engine/port/InputPort.hpp"
#include <memory>

namespace Engine
{
    namespace Port
    {
        /// @brief Represents a connection between two ports
        /// @details A Connection is a directed edge in the audio processing graph,
        /// connecting an OutputPort (source) to an InputPort (target).
        /// This class is used by AudioGraph to maintain the graph topology
        /// and perform topological sorting.
        /// @ingroup engine_port
        /// @see AudioGraph
        /// @see OutputPort
        /// @see InputPort
        class Connection
        {
        public:
            /// @brief Constructs a connection between two ports
            /// @param source The source OutputPort
            /// @param target The target InputPort
            /// @note The connection does not take ownership of the ports,
            /// they are managed by their respective modules
            Connection(std::shared_ptr<OutputPort> source, std::shared_ptr<InputPort> target);

            /// @brief Default destructor
            ~Connection() = default;

            /// @brief Gets the source output port
            /// @return Shared pointer to the source OutputPort
            std::shared_ptr<OutputPort> getSource() const;

            /// @brief Gets the target input port
            /// @return Shared pointer to the target InputPort
            std::shared_ptr<InputPort> getTarget() const;

        private:
            /// @brief Source output port of the connection
            std::shared_ptr<OutputPort> _source;

            /// @brief Target input port of the connection
            std::shared_ptr<InputPort> _target;
        };
    }

    struct ConnectionInfo { // info about a connection (for events purposes only)
        std::string sourceModule;
        std::string sourcePort;
        std::string destModule;
        std::string destPort;
        std::string connectionType;
    };
}

#endif // CONNECTION_HPP
