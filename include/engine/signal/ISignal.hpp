/// @file ISignal.hpp
/// @brief Signal interface for modules.
/// @ingroup engine_signal
///
/// ISignal defines the interface for signals exchanged between modules.

#ifndef ISIGNAL_HPP
#define ISIGNAL_HPP

namespace Engine {
    namespace Signal {
        /// @brief Enumeration of signal types.
        /// @details Defines the types of signals that can be sent and received by ports.
        /// @see Port::Port
        enum class SignalType {
            NONE,
            AUDIO,
            CONTROL,
            EVENT
        };
        
        /// @brief Interface class for signals.
        /// @details ISignal defines the interface for signals exchanged between modules.
        class ISignal {
        public:
            /// @brief Get the type of the signal.
            /// @return The type of the signal.
            virtual ~ISignal() = default;

            /// @brief Get the type of the signal.
            /// @return The type of the signal.
            virtual SignalType getType() const = 0;
        };
    }
}

#endif // ISIGNAL_HPP