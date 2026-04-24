/**
 * @file InstrumentAudioOutModule.hpp
 * @author Allan Leherpeux
 * @brief Instrument Audio Output module definition
 * @ingroup engine_module
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef InstrumentAudioOutModule_HPP
    #define InstrumentAudioOutModule_HPP

    #include "engine/module/AudioOutModule.hpp"
    #include "engine/module/Instrument.hpp"
    #include "engine/module/ModuleGroup.hpp"
    #include "engine/AudioGraph/AudioGraph.hpp"
    #include "engine/module/BaseModule.hpp"
    #include "engine/parameter/Parameters.hpp"
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Instrument Audio Output module that combines an Instrument and AudioOutModule.
        class InstrumentAudioOutModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new Instrument Audio Out Module object
            InstrumentAudioOutModule();

            /// @brief Construct a new Instrument Audio Out Module object with a given name
            /// @param name The name of the module
            InstrumentAudioOutModule(std::string name);

            /// @brief Destroy the Instrument Audio Out Module object
            virtual ~InstrumentAudioOutModule();

            /// @brief Clone the Instrument Audio Out module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;
        };
    } // namespace Module
} // namespace Engine

#endif // InstrumentAudioOutModule_HPP