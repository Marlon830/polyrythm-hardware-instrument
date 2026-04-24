/**
 * @file AudioOutModule.hpp
 * @brief Audio output module for sending audio to the output backend.
 *
 * @ingroup engine_module
 *
 * AudioOutModule handles the processing of audio signals and sends them
 * to the configured audio output backend.
 */

#ifndef AUDIO_OUT_MODULE_HPP
    #define AUDIO_OUT_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/port/OutputPort.hpp"
    #include "engine/core/backend/IAudioOutputBackend.hpp"
    #include "engine/core/IEventEmitter.hpp"

namespace Engine {
    namespace Module {
        /// @brief Audio output module for sending audio to the output backend.
        /// @details Inherits from BaseModule and processes audio signals to
        /// send them to the configured audio output backend.
        class AudioOutModule : public BaseModule {
        public:
            /// @brief Construct a new AudioOutModule.
            AudioOutModule();
            AudioOutModule(std::string name);

            /// @brief Destroy the AudioOutModule.
            ~AudioOutModule() override;

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing the basic context audio parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

        private:
            /// @brief The audio output backend used to send audio data.
            /// @details This is a shared pointer to an IAudioOutputBackend implementation.
            /// the use of shared_ptr and interface allows for flexible backend management.
            std::shared_ptr<Core::IAudioOutputBackend> _audioBackend;
            
        };
    } // namespace Module
} // namespace Engine

#endif // AUDIO_OUT_MODULE_HPP