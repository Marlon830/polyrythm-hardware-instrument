/// @file Instrument.hpp
/// @brief Instrument module definition.
/// @ingroup engine_module
/// @details Defines the Instrument module which manages multiple voices
/// for polyphonic synthesis. It handles note on/off events and mixes audio
/// signals from active voices. The Instrument module is a ModuleGroup
/// that contains an audio graph for processing voice audio. this is caracteristic of an composite module.
#ifndef INSTRUMENT_HPP
    #define INSTRUMENT_HPP

    #include "engine/module/ModuleGroup.hpp"
    #include "engine/signal/AudioSignal.hpp"
    #include "engine/signal/EventSignal.hpp"

    #include "engine/core/voice/VoiceManager.hpp"
    #include "common/event/EmitModuleList.hpp"
    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Instrument module class.
        /// @details The Instrument manages multiple voices for polyphonic synthesis.
        /// It handles note on/off events and mixes audio signals from active voices. The Instrument module
        /// is a ModuleGroup that contains an audio graph for processing voice audio. This is characteristic of a composite module.
        class Instrument : public ModuleGroup {
        public:

            /// @brief Constructs an Instrument module.
            Instrument();

            /// @brief Constructs an Instrument module with a given name.
            /// @param name The name of the instrument.
            Instrument(std::string name);

            /// @brief Destroys the Instrument module.
            virtual ~Instrument();

            /// @brief Clones the Instrument module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Sets the main audio graph for the instrument.
            /// @param graph The audio graph to set.
            void setMainGraph(const AudioGraph& graph);

            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule process method to handle note events and mix voice audio.
            void process(Core::AudioContext& context) override;

            /// @brief Retrieves all parameters from the instrument's modules.
            /// @return A map of module names to their parameters.
            std::map<std::string, Common::ModuleInfo> getAllParams();

        private:
            /// @brief VoiceManager instance for managing voices.
            Core::VoiceManager _voiceManager;

            /// @brief Audio input port for receiving audio signals.
            std::shared_ptr<Port::InputPort> _audioInputPort;
        };
    } // namespace Module
} // namespace Engine

#endif // INSTRUMENT_HPP