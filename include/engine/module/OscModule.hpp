/*
 * @file OscModule.hpp
 * @brief Oscillator module definition.
 *
 * @ingroup engine_module
 *
 * Defines the OscillatorModule class which generates audio waveforms
 * such as sine, square, triangle, and sawtooth waves.
*/
#ifndef OSC_MODULE_HPP
    #define OSC_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"

    #include "engine/core/voice/VoiceManager.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {

        /// @brief Enum representing different waveform types.
        enum WaveformType {
            SINE,
            SQUARE,
            TRIANGLE,
            SAWTOOTH
        };

        /// @brief Oscillator module generating audio waveforms.
        /// @details Inherits from BaseModule and implements waveform generation.
        /// Supports sine, square, triangle, and sawtooth waveforms.
        class OscillatorModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new OscillatorModule.
            OscillatorModule();

            OscillatorModule(std::string name);

            virtual IModule* clone() const override;

            /// @brief Destroy the OscillatorModule.
            virtual ~OscillatorModule();

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

            /// @brief Set the waveform type of the oscillator.
            /// @param waveform The waveform type.
            void setWaveform(WaveformType waveform);

            void setPitch(double pitch);

            /// @brief Get the current phase of the oscillator.
            /// @return The current phase in radians.
            double getPhase() const;

            /// @brief Get the waveform type of the oscillator.
            /// @return The waveform type.
            WaveformType getWaveform() const;

        private:
            /// @brief Current phase of the oscillator.
            std::shared_ptr<Param<WaveformType>> _waveform;

            std::shared_ptr<Param<double>> _pitchoffset;

            /// @brief Waveform type of the oscillator.
            double _phase;
        };
    }
}
#endif // OSC_MODULE_HPP