#include "engine/signal/AudioSignal.hpp"

namespace Engine {
    namespace Signal {
        AudioSignal::AudioSignal(PCMBuffer buffer, size_t bufferSize)
            : _type(SignalType::AUDIO), _buffer(std::move(buffer)), _bufferSize(bufferSize) {
        }

        SignalType AudioSignal::getType() const {
            return _type;
        }

        PCMBuffer AudioSignal::getBuffer() {
            return _buffer;
        }

        size_t AudioSignal::getBufferSize() {
            return _bufferSize;
        }
    }
}