/**
 * @file AlsaOutputBackend.hpp
 * @brief ALSA audio output backend implementation.
 *
 * @ingroup engine_core
 *
 * This class implements the IAudioOutputBackend interface using the ALSA
 * (Advanced Linux Sound Architecture) library for audio output on Linux systems.
 */
#ifndef ALSAOUTPUTBACKEND_HPP
    #define ALSAOUTPUTBACKEND_HPP

#include "engine/core/backend/IAudioOutputBackend.hpp"
#include "engine/core/backend/AudioParameters.hpp"

#include <alsa/asoundlib.h>
#include <string>

namespace Engine {
namespace Core {
/// @brief ALSA audio output backend implementation.
/// @details This class implements the IAudioOutputBackend interface using the ALSA
/// (Advanced Linux Sound Architecture) library for audio output on Linux systems.
class AlsaOutputBackend : public IAudioOutputBackend {
public:
    /// @brief Construct a new AlsaOutputBackend.
    AlsaOutputBackend();

    /// @brief Destroy the AlsaOutputBackend.
    ~AlsaOutputBackend();

    /// @brief Open the audio output backend.
    /// @return True if the backend was opened successfully, false otherwise.
    bool open() override;

    /// @brief Close the audio output backend.
    void close() override;

    /// @brief Write audio data to the output backend.
    /// @param buffer Pointer to the audio data buffer.
    /// @param size Size of the audio data buffer in samples.
    void write(const double* buffer, std::size_t size) override;

    /// @brief Get the last error message.
    /// @return A string describing the last error that occurred.
    const std::string& lastError() const noexcept;

    /// @brief Get the last error number.
    /// @return The error number of the last error that occurred.
    int lastErrorNo() const noexcept;

    /// @brief Clear the last error message and number.
    void clearError() noexcept;

private:
    // ALSA-specific members
    snd_pcm_t* _pcmHandle;

    /// @brief Last error number.
    int _lastErrno;

    /// @brief Last error message.
    std::string _lastErrMsg;

    /// @brief Audio parameters
    //TODO: make configurable
    unsigned int _sampleRate{SAMPLE_RATE};
    unsigned int _channels{NB_CHANNELS};

    /// @brief Set the last error message and number.
    /// @param where Description of where the error occurred.
    /// @param errNo The error number.
    void setError(const char *where, int errNo) noexcept;
};

} // namespace Core
} // namespace Engine

#endif // ALSAOUTPUTBACKEND_HPP