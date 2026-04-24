/**
 * @file FilterModule.hpp
 * @author Allan Leherpeux
 * @brief Filter module for audio processing.
 * @version 0.1
 * @date 2025-11-14
 * @ingroup engine_module
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef FILTER_MODULE_HPP
    #define FILTER_MODULE_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"

    #include "engine/parameter/Parameters.hpp"
    #include "engine/parameter/Param.hpp"

    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Filter types available for the FilterModule.
        enum FilterType {
            LOW_PASS,
            HIGH_PASS,
            BAND_PASS
        };

        /// @brief Filter module applying audio filters.
        /// @details Inherits from BaseModule and implements basic audio filtering using biquad filters.
        /// Supports low-pass, high-pass, and band-pass filters.
        class FilterModule : public BaseModule, public Parameters {
        public:
            /// @brief Construct a new FilterModule.
            FilterModule();

            FilterModule(std::string name);

            /// @brief Destroy the FilterModule.
            virtual ~FilterModule();

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule::process().
            void process(Core::AudioContext& context) override;

            /// @brief Set the filter type.
            /// @param type The filter type.
            void setFilterType(FilterType type);

            /// @brief Set the cutoff frequency of the filter.
            /// @param frequency The cutoff frequency in Hz.
            void setCutoffFrequency(double frequency);

            /// @brief Set the resonance (Q factor) of the filter.
            /// @param resonance The resonance value.
            void setResonance(double resonance);
            
            /// @brief Get the filter type.
            /// @return The filter type.
            FilterType getFilterType() const;

            /// @brief Get the cutoff frequency of the filter.
            /// @return The cutoff frequency in Hz.
            float getCutoffFrequency() const;

            /// @brief Get the resonance (Q factor) of the filter.
            /// @return The resonance value.
            float getResonance() const;
        private:
            /// @brief Internal state variables for the filter.
            std::shared_ptr<Param<FilterType>> _filterType;

            /// @brief Cutoff frequency of the filter in Hz.
            std::shared_ptr<Param<double>> _cutoffFrequency;

            /// @brief Resonance (Q factor) of the filter.
            std::shared_ptr<Param<double>> _resonance;

            /// @brief Amount of envelope modulation on cutoff frequency.
            std::shared_ptr<Param<double>> _envAmount;

            /// @brief Previous input sample (x[n-1]).
            double _x1 = 0.0;

            /// @brief Previous previous input sample (x[n-2]).
            double _x2= 0.0;

            /// @brief Previous output sample (y[n-1]).
            double _y1= 0.0;

            /// @brief Previous previous output sample (y[n-2]).
            double _y2 = 0.0;
        };
    }
}

#endif // FILTER_MODULE_HPP