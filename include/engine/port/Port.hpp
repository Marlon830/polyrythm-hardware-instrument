/**
 * @file Port.hpp
 * @brief Generic port interface for modules.
 *
 * @ingroup engine_port
 *
 * Ports connect modules; an InputPort receives data from an OutputPort.
 */

#ifndef PORT_HPP
    #define PORT_HPP

    #include "engine/signal/ISignal.hpp"
    #include <string>

namespace Engine {
    namespace Port {
        /// @brief Base class for all ports.
        /// @details Ports serve as a guarantee for avoiding ownership issues between modules.
        class Port {
        public:
            /// @brief Constructor for Port.
            /// @param name The name of the port.
            /// @param type The type signal that the port can send or receive.
            /// @see Signal::SignalType
            Port(const std::string& name, Signal::SignalType type);

            /// @brief Virtual destructor for Port.
            virtual ~Port() = default;

            /// @brief Get the name of the port.
            /// @return The name of the port.
            std::string getName() const;

            /// @brief Get the signal type of the port.
            /// @return The signal type of the port.
            Signal::SignalType getType() const;

        protected:
            /// @brief The name of the port.
            std::string _name;

            /// @brief The signal type of the port.
            Signal::SignalType _type;
        };
    }
}

#endif // PORT_HPP