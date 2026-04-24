/**
 * @file SamplerModule.hpp
 * @brief Sampler module for playing audio samples.
 *
 * @ingroup engine_module
 *
 * SamplerModule loads WAV files and plays them in response to NOTE_ON/NOTE_OFF events.
 * It supports polyphonic playback and MIDI note mapping.
 */

#ifndef SAMPLER_MODULE_HPP
    #define SAMPLER_MODULE_HPP

    #include "engine/module/ModuleGroup.hpp"
    #include "engine/signal/EventSignal.hpp"
    #include "engine/signal/AudioSignal.hpp"
    #include "engine/utils/WavLoader.hpp"
    #include "engine/core/voice/VoiceManager.hpp"
    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"
    #include <map>
    #include <memory>
    #include <string>

namespace Engine {
    namespace Module {
        
        /// @brief Structure representing a playing sample voice
        struct SampleVoice {
            PCMBuffer sampleData;  ///< The audio sample data
            size_t playbackPosition;         ///< Current playback position
            int noteNumber;                  ///< MIDI note number
            double velocity;                  ///< Note velocity
            bool isPlaying;                  ///< Whether the voice is active
            uint32_t sampleRate;             ///< Original sample rate
            
            SampleVoice() : playbackPosition(0), noteNumber(-1), 
                           velocity(0.0), isPlaying(false), sampleRate(44100) {}
        };

        /// @brief Sampler module for playing audio samples
        /// @details Inherits from ModuleGroup and manages sample playback
        /// with polyphony support. Samples are triggered by NOTE_ON events.
        class SamplerModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new SamplerModule
            SamplerModule();

            /// @brief Construct a new SamplerModule with a name
            /// @param name Name of the module
            SamplerModule(std::string name);

            /// @brief Destroy the SamplerModule
            virtual ~SamplerModule();

            /// @brief Clone the Sampler module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context
            /// @param context The audio context containing buffers and parameters
            void process(Core::AudioContext& context) override;

            /// @brief Get the loaded samples map
            /// @return Map of MIDI note numbers to WavData samples
            std::map<int, Util::WavData> getSamples() const;

            /// @brief Get the number of loaded samples
            /// @return Number of samples currently loaded
            size_t getLoadedSamplesCount() const;

            /// @brief Clear all loaded samples
            void clearSamples();

        private:
            /// @brief Trigger a sample to play
            /// @param noteNumber MIDI note number
            /// @param velocity Note velocity (0.0-1.0)
            void triggerSample(int noteNumber, double velocity);

            /// @brief Stop a playing sample
            /// @param noteNumber MIDI note number
            void stopSample(int noteNumber);

            /// @brief Find a free voice for playback
            /// @return Pointer to free voice or nullptr
            SampleVoice* findFreeVoice();

            /// @brief Mix active voices into output buffer
            /// @param context Audio context
            /// @return Mixed audio signal
            std::shared_ptr<Signal::AudioSignal> mixVoices(const Core::AudioContext& context);

            /// @brief Map of MIDI notes to sample data
            std::shared_ptr<Param<std::map<int, Util::WavData>>> _samples;

            /// @brief Pool of sample voices for polyphony
            std::vector<SampleVoice> _voices;

            /// @brief Maximum number of voices
            size_t _maxVoices;
        };
    } // namespace Module
} // namespace Engine

#endif // SAMPLER_MODULE_HPP