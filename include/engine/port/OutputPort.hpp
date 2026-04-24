/// @file OutputPort.hpp
/// @brief Output port for modules.
/// @ingroup engine_port
/// An OutputPort sends signals to InputPorts of other modules.
/// It's an implementation of the Port interface.

#ifndef OUTPUTPORT_HPP
    #define OUTPUTPORT_HPP

    #include "engine/port/Port.hpp"
    #include "engine/port/InputPort.hpp"
    #include "engine/signal/ISignal.hpp"
    #include <vector>
    #include <memory>

namespace Engine {
    namespace Port {

        /// @brief Output port class for sending signals.
        /// @details OutputPort manages connections to InputPorts of other modules
        /// and sends signals to them.
        /// @note Each OutputPort have its own list of connection to InputPorts
        /// for optimisation issue. But it's the responsibility of the AudioGraph
        /// to execute the connections between OutputPorts and InputPorts of modules.
        class OutputPort : public Port {
        public:
            /// @brief Constructor for OutputPort.
            /// @param name The name of the port.
            /// @param type The type of signal the port can send.
            /// @see Signal::SignalType
            /// @note we use explicit to avoid implicit conversions.
            explicit OutputPort(const std::string& name, Signal::SignalType type);

            /// @brief Virtual destructor for OutputPort.
            virtual ~OutputPort() = default;

            /// @brief Connect this OutputPort to an InputPort.
            /// @param input The InputPort to connect to.
            void connect(std::shared_ptr<InputPort> input);

            /// @brief Disconnect this OutputPort from an InputPort.
            /// @param input The InputPort to disconnect from.
            void disconnect(std::shared_ptr<InputPort> input);

            /// @brief Send a signal to all connected InputPorts.
            /// @param signal The signal to send.
            void send(std::shared_ptr<Signal::ISignal> signal);

            std::shared_ptr<Signal::ISignal> getLastSignal() const { return _lastSignal; }

        private:
            /// @brief List of connected InputPorts.
            std::vector<std::shared_ptr<InputPort>> _connections;

            std::shared_ptr<Signal::ISignal> _lastSignal;
        };
    }
}

#endif // OUTPUTPORT_HPP