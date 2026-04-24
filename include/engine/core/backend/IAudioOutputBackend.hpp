/**
 * @file IAudioOutputBackend.hpp
 * @brief Interface for audio output backends.
 *
 * @ingroup engine_core
 *
 * This interface defines the necessary methods for audio output backends
 * to implement in order to handle audio data output.
 */
#ifndef IAUDIOOUTPUTBACKEND_HPP
    #define IAUDIOOUTPUTBACKEND_HPP

#include <cstdint>

namespace Engine {
namespace Core {
/// @brief Interface for audio output backends.
/// @details This interface defines the necessary methods for audio output backends
/// to implement in order to handle audio data output.
class IAudioOutputBackend {
public:
    virtual ~IAudioOutputBackend() = default;

    /// @brief Open the audio output backend.
    virtual bool open() = 0;

    /// @brief Close the audio output backend.
    virtual void close() = 0;

    /// @brief Write audio data to the output backend.
    /// @param buffer Pointer to the audio data buffer.
    /// @param size Size of the audio data buffer in samples.
    virtual void write(const double* buffer, std::size_t size) = 0;
};

} // namespace Core
} // namespace Engine

#endif // IAUDIOOUTPUTBACKEND_HPP