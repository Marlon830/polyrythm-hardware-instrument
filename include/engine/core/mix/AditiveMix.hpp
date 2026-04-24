/*
    @file AditiveMix.hpp
    @brief Declaration of the AditiveMix class for audio signal mixing.

    @ingroup engine_core

    The AditiveMix class implements the IMixStrategy interface to provide
    additive mixing of multiple audio signals. It sums the audio samples from
    all input signals and normalizes the result to prevent clipping.
*/

#ifndef ADITIVEMIX_HPP
    #define ADITIVEMIX_HPP

    #include "engine/core/mix/IMixStrategy.hpp"
    #include "engine/signal/AudioSignal.hpp"
    #include <vector>
    #include <memory>

namespace Engine::Core {

    /// @brief Implements additive mixing of audio signals.
    class AditiveMix : public IMixStrategy {
    public:
        /// @brief Construct the AditiveMix object.
        AditiveMix() = default;

        /// @brief Destroy the AditiveMix object.
        ~AditiveMix() = default;
        
        /// @brief Mix multiple audio signals additively.
        /// @param signals Vector of shared pointers to AudioSignal objects.
        /// @return Shared pointer to the mixed AudioSignal.
        std::shared_ptr<Signal::AudioSignal> mix(const std::vector<std::shared_ptr<Signal::AudioSignal>>& signals) override;
    };
} // namespace Engine::Core

#endif // ADITIVEMIX_HPP