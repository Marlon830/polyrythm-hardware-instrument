#include "engine/signal/EventSignal.hpp"

namespace Engine {
    namespace Signal {
        SignalType EventSignal::getType() const {
            return _type;
        }

        EventType EventSignal::getEventType() const {
            return _eventType;
        }

        uint32_t EventSignal::getOffset() const {
            return _offset;
        }

        const uint8_t* EventSignal::getData() const {
            return data;
        }

        void EventSignal::setEventType(EventType type) {
            _eventType = type;
        }

        void EventSignal::setOffset(uint32_t offset) {
            _offset = offset;
        }

        void EventSignal::setData(const uint8_t* inputData, size_t length) {
            size_t copyLength = (length < sizeof(data)) ? length : sizeof(data);
            for (size_t i = 0; i < copyLength; ++i) {
                data[i] = inputData[i];
            }
        }
    }
}