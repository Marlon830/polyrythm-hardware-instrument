#ifndef CONTROL_SIGNAL_HPP
    #define CONTROL_SIGNAL_HPP

    #include "engine/signal/ISignal.hpp"
    #include <memory>
    #include <vector>
    #include <cstdint>

    namespace Engine {
        namespace Signal {
            /// @brief Representation of a signal containing control data.
            /// @details This class is a concrete implementation of ISignal for control data.
            /// @see ISignal
            class ControlSignal : public ISignal {
            public:
                /// @brief Construct a new ControlSignal object.
                /// @param controlValue The control value.
                ControlSignal(const std::vector<double>& controlValues)
                    : _type(SignalType::CONTROL), _controlValues(controlValues) {}
                /// @brief Destroy the ControlSignal object.
                ~ControlSignal() = default;

                /// @brief Get the type of the signal.
                /// @return The signal type (CONTROL).
                SignalType getType() const override;

                /// @brief Get the control value.
                /// @return The control value.
                const std::vector<double>& getControlValues() const;

            private:
                /// @brief The type of the signal. (CONTROL)
                SignalType _type;

                /// @brief The control value.
                std::vector<double> _controlValues; // vector for having on controle value for each sample of the pcm buffer
            };
        }
    }

#endif // CONTROL_SIGNAL_HPP