/// @file InputPort.hpp
/// @brief Input port for modules.
///
/// @ingroup engine_port
///
/// An InputPort receives signals from an OutputPort of another module.
/// It's an implementation of the Port interface.

#ifndef INPUTPORT_HPP
    #define INPUTPORT_HPP

    #include "engine/port/Port.hpp"
    #include "engine/signal/ISignal.hpp"
    
    #include <functional>
    #include <memory>
    #include <vector>

namespace Engine {
    namespace Port {
        /// @brief Input port class for receiving signals.
        /// @details InputPort allows binding a callback function that is invoked
        /// when a signal is received. Allow to change an internal state of a module
        /// based on incoming signals.
        class InputPort : public Port {
        public:

            
            /// @brief Constructor for InputPort.
            /// @param name The name of the port.
            /// @param type The type of signal the port can receive.
            /// @see Signal::SignalType
            /// @note we use explicit to avoid implicit conversions.
            explicit InputPort(const std::string& name, Signal::SignalType type);

            /// @brief Virtual destructor for InputPort.
            virtual ~InputPort() = default;

            /// @brief Receive a signal and invoke the bound callback.
            /// @param signal The signal being received.
            void receive(std::shared_ptr<Signal::ISignal> signal);

            /// @brief Get the last received signal.
            /// @return A shared pointer to the last received signal.
            std::shared_ptr<Signal::ISignal> get() const;

        private:

            std::vector<std::shared_ptr<Signal::ISignal>> _receivedSignal; ///< The last received signal.
        };
    }
}

#endif // INPUTPORT_HPP