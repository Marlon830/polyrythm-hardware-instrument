/**
 * @file DebugModule.hpp
 * @brief Debug module for testing and diagnostics.
 *
 * @ingroup engine_module
 *
 * DebugModule provides functionalities for debugging audio signals
 * within the engine.
 */

#ifndef DEBUG_MODULE_HPP
    #define DEBUG_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"

    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Debug module for testing and diagnostics.
        /// @details Inherits from BaseModule and provides functionalities
        /// for debugging audio signals within the engine.
        class DebugModule : public BaseModule {
        public:
            /// @brief Construct a new DebugModule.
            DebugModule();
            DebugModule(std::string name);

            /// @brief Destroy the DebugModule.
            virtual ~DebugModule();

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

        };
    } // namespace Module
} // namespace Engine

#endif // DEBUG_MODULE_HPP