#ifndef ENGINE_MODULE_MIXMODULE_HPP
    #define ENGINE_MODULE_MIXMODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include "engine/core/mix/IMixStrategy.hpp"

    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Mix module for combining two audio signals.
        /// @details Inherits from BaseModule and implements mixing of two audio input signals
        /// with adjustable levels.
        class MixModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new MixModule.
            MixModule();
            MixModule(std::string name);

            /// @brief Destroy the MixModule.
            virtual ~MixModule();
            virtual IModule* clone() const override;
            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

        private:
            /// @brief Level of input 1 (0.0 to 1.0).
            std::shared_ptr<Param<double>> _input1Level;

            /// @brief Level of input 2 (0.0 to 1.0).
            std::shared_ptr<Param<double>> _input2Level;

            /// @brief Mixing strategy.
            std::shared_ptr<Core::IMixStrategy> _mixStrategy;
        };
    } // namespace Module
} // namespace Engine

#endif // ENGINE_MODULE_MIXMODULE_HPP