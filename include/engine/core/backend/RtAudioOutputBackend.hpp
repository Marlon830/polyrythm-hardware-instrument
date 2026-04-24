/**
 * @file RtAudioOutputBackend.hpp
 * @brief RtAudio audio output backend implementation.
 *
 * @ingroup engine_core
 *
 * This class implements the IAudioOutputBackend interface using the RtAudio
 * library for cross-platform audio output (Windows WASAPI, Linux ALSA, macOS CoreAudio).
 * * @note This implementation uses an internal Ring Buffer to adapt the 
 * "Push" model of the engine (blocking write) to the "Pull" model of RtAudio (callback).
 */
#ifndef RTAUDIOOUTPUTBACKEND_HPP
    #define RTAUDIOOUTPUTBACKEND_HPP

#include "RtAudio.h"
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include "engine/core/backend/IAudioOutputBackend.hpp"
#include "engine/core/backend/AudioParameters.hpp"

namespace Engine {
    namespace Core {

    /**
     * @class RtAudioOutputBackend
     * @brief Backend implementation handling audio output via RtAudio.
     */
    class RtAudioOutputBackend : public IAudioOutputBackend {
    public:
        /**
         * @brief Constructor.
         * Initializes the backend state but does not open the device.
         */
        RtAudioOutputBackend();

        /**
         * @brief Destructor.
         * Closes the stream and releases resources if active.
         */
        ~RtAudioOutputBackend();

        /**
         * @brief Opens the default audio output device.
         * * Configures the stream with the set parameters (rate, channels),
         * allocates the internal ring buffer, and starts the audio stream.
         * * @return true if the stream was successfully opened and started.
         * @return false otherwise (check lastError()).
         */
        bool open();

        /**
         * @brief Closes the audio stream.
         * Stops the stream and releases the RtAudio resources.
         */
        void close();

        /**
         * @brief Writes audio data to the output device.
         * * This method copies the provided buffer into the internal Ring Buffer.
         * * @note BLOCKING BEHAVIOR: If the internal ring buffer is full, 
         * this method will BLOCK (sleep) until the audio callback consumes 
         * enough data to make space. This mimics the blocking behavior of 
         * standard ALSA write calls.
         * * @param buffer Pointer to the interleaved double audio data.
         * @param size Total number of double elements (frames * channels).
         */
        void write(const double* buffer, std::size_t size) override;

        // --- Error Handling ---

        /**
         * @brief Gets the last error message description.
         * @return A string reference containing the error text.
         */
        const std::string& lastError() const noexcept { return _lastErrMsg; }

        /**
         * @brief Gets the last error code.
         * @return The internal or RtAudio error code.
         */
        int lastErrorNo() const noexcept { return _lastErrno; }

        /**
         * @brief Clears the last error state.
         */
        void clearError() noexcept;

        /**
         * @brief Sets the audio parameters before opening.
         * @param rate Sample rate in Hz (e.g., 44100, 48000).
         * @param channels Number of channels (1 for mono, 2 for stereo).
         */
        void setParameters(unsigned int rate, unsigned int channels) {
            _sampleRate = rate;
            _channels = channels;
        }

    private:
        /**
         * @brief Internal helper to set the error state.
         * @param where Function name or location where the error occurred.
         * @param errNo Error code.
         */
        void setError(const char* where, int errNo) noexcept;

        /**
         * @brief Static callback function required by RtAudio.
         * * This function is called by the audio driver (in a high-priority thread)
         * when it needs more audio data. It reads from the _ringBuffer.
         * * @param outputBuffer Buffer to fill with audio data.
         * @param inputBuffer Input buffer (unused here).
         * @param nBufferFrames Number of frames requested.
         * @param streamTime Stream time (unused).
         * @param status Stream status (e.g., underflow warning).
         * @param userData Pointer to the RtAudioOutputBackend instance.
         * @return 0 to continue, 1 to stop.
         */
        static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                 double streamTime, RtAudioStreamStatus status, void* userData);

        RtAudio _dac;                       ///< RtAudio instance.
        unsigned int _sampleRate = SAMPLE_RATE;   ///< Target sample rate.
        unsigned int _channels = NB_CHANNELS;         ///< Target channel count.

        // --- Ring Buffer Management ---
        
        std::vector<double> _ringBuffer;     ///< Circular buffer storage.
        std::size_t _writeIndex = 0;        ///< Current write position (engine).
        std::size_t _readIndex = 0;         ///< Current read position (audio callback).
        std::size_t _availableData = 0;     ///< Number of frames currently available to read.
        std::size_t _bufferSizeFrames = 8192; ///< Size of the internal buffer in frames.

        std::mutex _mutex;                  ///< Mutex for thread-safety between engine and callback.
        std::condition_variable _cvBlockWrite; ///< CV to wake up the engine when space is available.
        
        std::string _lastErrMsg;            ///< Last error message storage.
        int _lastErrno = 0;                 ///< Last error code storage.
        bool _streamOpen = false;           ///< Flag indicating if stream is active.
    };

    } // namespace Core
} // namespace Engine

#endif // RTAUDIOOUTPUTBACKEND_HPP