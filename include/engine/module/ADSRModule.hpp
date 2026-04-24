/*
 *
 * @file ADSRModule.hpp
 * @author Allan Leherpeux
 * @brief ADSR module definition
 * @ingroup engine_module
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef ADSR_MODULE_HPP
    #define ADSR_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"

    #include "engine/core/voice/VoiceManager.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include <vector>
    #include <memory>
    #include <chrono>

namespace Engine {
    namespace Module {
        /// @brief Enum representing the states of the ADSR envelope.
        enum ADSRState {
            ATTACK,
            DECAY,
            SUSTAIN,
            RELEASE,
            OFF
        };

        /// @brief ADSR (Attack, Decay, Sustain, Release) envelope module.
        /// @details This module applies an ADSR envelope to incoming audio signals
        /// based on note on/off events and configurable parameters.
        class ADSRModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new ADSR Module object
            ADSRModule();

            /// @brief break Construct a new ADSR Module object
            /// @param name The name of the module
            ADSRModule(std::string name);

            /// @brief Clone the ADSR module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Destroy the ADSRModule.
            virtual ~ADSRModule();

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

            /// @brief Set the current state of the ADSR envelope.
            /// @param state The new state to set.
            void setState(ADSRState state);
        private:
            /// @brief Parameter for the attack time of the ADSR envelope. Seconds.
            std::shared_ptr<Param<double>> _attackTimeParam;

            /// @brief Parameter for the decay time of the ADSR envelope. Seconds.
            std::shared_ptr<Param<double>> _decayTimeParam;

            /// @brief Parameter for the sustain level of the ADSR envelope. (0.0 to 1.0)
            std::shared_ptr<Param<double>> _sustainLevelParam;

            /// @brief Parameter for the release time of the ADSR envelope. Seconds.
            std::shared_ptr<Param<double>> _releaseTimeParam;

            /// @brief Current state of the ADSR envelope.
            ADSRState _state = OFF;

            /// @brief Current level of the ADSR envelope.
            double _currentLevel = 0.0;

            /// @brief Time point marking the start of the current ADSR state.
            std::chrono::steady_clock::time_point _stateElapsedTime;

            /// @brief Elapsed time in seconds since the start of the current state.
            double _stateElapsedSec = 0.0;

            /// @brief Level at the start of the release phase.
            double _releaseStartLevel = 0.0;
        }; // class ADSRModule
    } // namespace Module
} // namespace Engine
#endif // ADSR_MODULE_HPP