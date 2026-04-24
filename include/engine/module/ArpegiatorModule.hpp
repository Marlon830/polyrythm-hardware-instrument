/**
 * @file ArpegiatorModule.hpp
 * @author Allan Leherpeux
 * @brief Arpegiator module definition
 * @ingroup engine_module
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef ARPEGGIATOR_MODULE_HPP
    #define ARPEGGIATOR_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/EventSignal.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include <vector>
    #include <memory>
    #include <chrono>

namespace Engine {
    namespace Module {
        /// @brief Arpegiator module that sequences notes based on a defined pattern and time interval.
        class ArpegiatorModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new Arpegiator Module object
            ArpegiatorModule();

            /// @brief Construct a new Arpegiator Module object with a given name
            /// @param name The name of the module
            ArpegiatorModule(std::string& name);

            /// @brief Destroy the Arpegiator Module object
            virtual ~ArpegiatorModule();

            /// @brief Clone the Arpegiator module.
            /// @return A pointer to the cloned module.
            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;
        private:
            /// @brief Time interval between arpeggiated notes in milliseconds.
            std::shared_ptr<Param<float>> _timeInterval;

            /// @brief Sequence of MIDI note numbers for the arpeggiator.
            std::shared_ptr<Param<std::vector<int>>> _noteSequence;

            /// @brief Last MIDI note played by the arpeggiator.
            int _lastNote = -1;

            /// @brief Current index in the note sequence.
            size_t _currentNoteIndex = 0;

            /// @brief Timestamp of the last note event.
            std::chrono::steady_clock::time_point _lastEventTime;};
    } // namespace Module
} // namespace Engine
#endif // ARPEGGIATOR_MODULE_HPP