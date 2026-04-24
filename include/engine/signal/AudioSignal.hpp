/**
* @file AudioSignal.hpp
* @brief Audio signal representation.
*
* @ingroup engine_signal
*
* This class represents an audio signal, encapsulating a buffer of audio samples
* and providing methods to access the signal type and buffer information.
*/

#ifndef AUDIOSIGNAL_HPP
    #define AUDIOSIGNAL_HPP

    #include "engine/signal/ISignal.hpp"
    #include <memory>
    #include <vector>

    using PCMBuffer = std::vector<double>;

    namespace Engine {
        namespace Signal {
            /// @brief Representation of a signal containing audio data.
            /// @details This class is a concrete implementation of ISignal for audio data.
            /// @see ISignal
            class AudioSignal : public ISignal {
            public:
                /// @brief Construct a new AudioSignal object.
                /// @param buffer The audio data buffer.
                /// @param bufferSize The size of the audio data buffer.
                AudioSignal(PCMBuffer buffer, size_t bufferSize);
                /// @brief Destroy the AudioSignal object.
                ~AudioSignal() = default;

                /// @brief Get the type of the signal.
                /// @return The signal type (AUDIO).
                SignalType getType() const override;

                /// @brief Get the audio data buffer.
                /// @return A vector containing the audio samples.
                PCMBuffer getBuffer();

                /// @brief Get the size of the audio data buffer.
                /// @return The size of the audio buffer.
                size_t getBufferSize();

            private:
                /// @brief The type of the signal. (AUDIO)
                SignalType _type;

                /// @brief The audio data buffer.
                PCMBuffer _buffer;

                /// @brief The size of the audio data buffer.
                size_t _bufferSize;
            };
        }
    }

#endif // ISIGNAL_HPP