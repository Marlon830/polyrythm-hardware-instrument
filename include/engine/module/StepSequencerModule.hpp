/**
 * @file StepSequencerModule.hpp
 * @brief Step sequencer module for pattern-based note generation.
 *
 * @ingroup engine_module
 *
 * StepSequencerModule provides a programmable step sequencer that generates
 * NOTE_ON/NOTE_OFF events based on programmed patterns.
 */

#ifndef STEP_SEQUENCER_MODULE_HPP
    #define STEP_SEQUENCER_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/EventSignal.hpp"
    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"
    #include <vector>
    #include <memory>
    #include <chrono>

namespace Engine {
    namespace Module {
        
        /// @brief Structure representing a drum track with its pattern
        struct SequencerTrack {
            /// @brief MIDI note number for this track
            int noteNumber;
            
            /// @brief Velocity for note events (0-127)
            //TODO: velocity is a vector to support velocity per step
            int velocity;
            
            /// @brief Pattern of steps (true = hit, false = rest)
            std::vector<bool> pattern;
            
            /// @brief Constructor
            SequencerTrack(int note = 36, int vel = 100, size_t steps = 16)
                : noteNumber(note), velocity(vel), pattern(steps, false) {}
        };

        /// @brief Step sequencer module for generating rhythmic patterns
        /// @details Inherits from BaseModule and implements a step sequencer
        /// that emits NOTE_ON/NOTE_OFF events based on programmed patterns.
        class StepSequencerModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new StepSequencerModule with default 16 steps
            /// @param steps Number of steps in the sequence (default: 16)
            /// @param bpm Tempo in beats per minute (default: 120)
            StepSequencerModule();

            /// @brief Construct a new StepSequencerModule with a name
            /// @param name Name of the module
            StepSequencerModule(std::string name);

            /// @brief Destroy the StepSequencerModule
            virtual ~StepSequencerModule();

            /// @brief Clone the StepSequencer module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context
            /// @param context The audio context containing buffers and parameters
            void process(Core::AudioContext& context) override;

            /// @brief Reset the sequencer to the first step
            void reset();

            void setTracks(const std::vector<SequencerTrack>& tracks) {
                _tracks->set(tracks);
            }
            
            template<typename T>
            T getParameter(const std::string& name) const {
                if (name == "tracks") {
                    return _tracks->get();
                }
                return T();
            }

            /// @brief Get current step position
            size_t getCurrentStep() const { return _currentStep; }

            /// @brief Get current sample counter within current step
            size_t getSampleCounter() const { return _sampleCounter; }

            /// @brief Get samples per step
            size_t getSamplesPerStep() const { return _samplesPerStep; }

            /// @brief Get current BPM
            float getBPM() const { return _bpm->get(); }

        private:
            /// @brief Calculate samples per step based on BPM and sample rate
            void calculateStepDuration(float sampleRate);

            /// @brief List of sequencer tracks
            std::shared_ptr<Param<std::vector<SequencerTrack>>> _tracks;

            /// @brief Current step in the sequence
            size_t _currentStep;

            /// @brief Total number of steps
            std::shared_ptr<Param<size_t>> _stepCount;

            /// @brief Tempo in beats per minute
            std::shared_ptr<Param<float>> _bpm;

            /// @brief Sample counter for timing
            size_t _sampleCounter;

            /// @brief Samples per step (calculated from BPM)
            size_t _samplesPerStep;

            /// @brief Last time the sequencer advanced
            std::chrono::steady_clock::time_point _lastStepTime;

            /// @brief Flag to track first process call
            bool _isFirstProcess;
        };
    } // namespace Module
} // namespace Engine

#endif // DRUMBOX_MODULE_HPP