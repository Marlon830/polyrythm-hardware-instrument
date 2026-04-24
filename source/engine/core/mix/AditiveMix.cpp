#include "engine/core/mix/AditiveMix.hpp"
#include <algorithm>
#include <cmath>

#include <iostream>

namespace Engine {
    namespace Core {

        std::shared_ptr<Signal::AudioSignal> AditiveMix::mix(const std::vector<std::shared_ptr<Signal::AudioSignal>>& signals) {
            if (signals.empty()) {
                return nullptr;
            }

            const size_t numSamples = signals[0]->getBuffer().size();
            const size_t numSignals = signals.size();

            const double voiceGain = 0.9 / std::sqrt(static_cast<double>(numSignals));


            PCMBuffer mixedBuffer(numSamples, 0.0);

            for (const auto& signal : signals) {
                const auto& buffer = signal->getBuffer();
                for (size_t i = 0; i < numSamples; ++i) {
                    mixedBuffer[i] += buffer[i] * voiceGain;
                }
            }

            for (auto& sample: mixedBuffer) {
                sample = std::tanh(sample);
            }

            return std::make_shared<Signal::AudioSignal>(std::move(mixedBuffer), numSamples);
        }

    } // namespace Core
} // namespace Engine   