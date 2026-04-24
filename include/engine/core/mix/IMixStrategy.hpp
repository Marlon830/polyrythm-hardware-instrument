/*
 @file IMixStrategy.hpp
    @brief Declaration of the IMixStrategy interface for audio mixing strategies.

    @ingroup engine_core

    The IMixStrategy interface defines the contract for audio mixing strategies.
    Implementations of this interface provide different algorithms for mixing
    multiple audio signals into a single output signal.
*/

#ifndef IMIXSTRATEGY_HPP
    #define IMIXSTRATEGY_HPP

    #include "engine/signal/AudioSignal.hpp"
    #include <vector>
    #include <memory>

namespace Engine::Core {
    /// @brief Interface for audio mixing strategies.
    class IMixStrategy {
    public:
        /// @brief Virtual destructor for proper cleanup of derived classes.
        virtual ~IMixStrategy() = default;

        /// @brief Mix multiple audio signals into a single output signal.
        /// @param signals Vector of shared pointers to AudioSignal objects.
        /// @return Shared pointer to the mixed AudioSignal.
        virtual std::shared_ptr<Signal::AudioSignal> mix(const std::vector<std::shared_ptr<Signal::AudioSignal>>& signals) = 0;
    };
} // namespace Engine::Core

#endif // IMIXSTRATEGY_HPP