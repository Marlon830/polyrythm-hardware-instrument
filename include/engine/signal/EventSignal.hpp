#ifndef EVENTSIGNAL_HPP
    #define EVENTSIGNAL_HPP

    #include "engine/signal/ISignal.hpp"
    #include <memory>
    #include <vector>
    #include <cstdint>

    namespace Engine {
        namespace Signal {

            enum class EventType {
                NOTE_ON,
                NOTE_OFF,
                CONTROL_CHANGE
            };

            /// @brief Representation of a signal containing event data.
            /// @details This class is a concrete implementation of ISignal for event data.
            /// @see ISignal
            class EventSignal : public ISignal {
            public:
                EventSignal(EventType eventType, uint32_t offset, uint8_t number, uint8_t velocity)
                    : _type(SignalType::EVENT), _eventType(eventType), _offset(offset) {
                    data[0] = number;
                    data[1] = velocity;
                }
                /// @brief Destroy the EventSignal object.
                ~EventSignal() = default;

                /// @brief Get the type of the signal.
                /// @return The signal type (EVENT).
                SignalType getType() const override;

                /// @brief Get the event type.
                /// @return The event type (NOTE_ON, NOTE_OFF, CONTROL_CHANGE).
                EventType getEventType() const;

                /// @brief Get the offset of the event.
                /// @return The offset of the event.
                uint32_t getOffset() const;

                /// @brief Get the event data.
                /// @return Pointer to the event data.
                const uint8_t* getData() const;

                /// @brief Set the event type.
                /// @param type The event type to set.
                void setEventType(EventType type);

                /// @brief Set the offset of the event.
                /// @param offset The offset to set.
                void setOffset(uint32_t offset);

                /// @brief Set the event data.
                /// @param inputData Pointer to the input data.
                /// @param length The length of the input data.
                void setData(const uint8_t* inputData, size_t length);

            private:
                /// @brief The type of the signal. (EVENT)
                SignalType _type;
                
                /// @brief The type of the event.
                EventType _eventType;

                /// @brief The offset of the event.
                uint32_t _offset;

                /// @brief The event data (e.g., note number, velocity).
                uint8_t data[2];
            };
        }
    }
#endif // EVENTSIGNAL_HPP