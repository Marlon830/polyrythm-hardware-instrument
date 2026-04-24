/// @file EventEmitter.hpp
/// @brief EventEmitter module definition.
/// @ingroup engine_module
/// @details Defines the EventEmitter module which generates note on/off events
/// at specified intervals. It toggles note events based on a set time and note value.

#ifndef EventEmitter_HPP
    #define EventEmitter_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/EventSignal.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include <vector>
    #include <memory>
#include <chrono>


namespace Engine {
    namespace Module {
        /// @brief EventEmitter module class.
        /// @details The EventEmitter generates note on/off events at specified intervals.
        class EventEmitter : public BaseModule, public Parameters {
        public:
            /// @brief Constructs an EventEmitter module.
            EventEmitter();

            EventEmitter(std::string& name);

            /// @brief Destroys the EventEmitter module.
            virtual ~EventEmitter();

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule process method to generate note events.
            void process(Core::AudioContext& context) override;

            void setNote(int note) {
                _note->set(note);
            }

            void setTime(int time) {
                _time->set(time);
            }

            int getNote() const {
                return _note->get();
            }

            int getTime() const {
                return _time->get();
            }

        private:
            /// @brief The time interval for note events.
            std::shared_ptr<Param<int>> _time;
            
            /// @brief The note value for note events.
            std::shared_ptr<Param<int>> _note;

            /// @brief The last toggle time point.
            std::chrono::steady_clock::time_point _lastToggle{};
            /// @brief Indicates whether the note is currently on.
            bool _isOn{false};
        };
    } // namespace Module
} // namespace Engine

#endif // EventEmitter_HPP