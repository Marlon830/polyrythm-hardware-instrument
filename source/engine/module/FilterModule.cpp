#include "engine/module/FilterModule.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"
#include "engine/signal/ControlSignal.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Engine {
    namespace Module {
        FilterModule::FilterModule(): FilterModule("FilterModule") {} 

        FilterModule::FilterModule(std::string name) 
            : _filterType(std::make_shared<Param<FilterType>>("FilterType", LOW_PASS)),
              _cutoffFrequency(std::make_shared<Param<double>>("CutoffFrequency", 1000.0)),
              _resonance(std::make_shared<Param<double>>("Resonance", 1.0)),
              _envAmount(std::make_shared<Param<double>>("EnvAmount", 2000.0)) {

                _name = std::move(name);
                auto outputPort = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);
                _outputPorts.push_back(outputPort);
                auto inputPort = std::make_shared<Port::InputPort>("in", Signal::SignalType::AUDIO);
                _inputPorts.push_back(inputPort);

                auto cutoffPort = std::make_shared<Port::InputPort>("cutoffCV", Signal::SignalType::CONTROL);
                _inputPorts.push_back(cutoffPort);

                _parameters.push_back(_filterType);
                _parameters.push_back(_cutoffFrequency);
                _parameters.push_back(_resonance);
        }

        FilterModule::~FilterModule() {
        }

        IModule* FilterModule::clone() const {
            return new FilterModule(*this);
        }

        void FilterModule::process(Core::AudioContext& context) {
            const size_t numSamples = context.bufferSize;
            const int sampleRate = context.sampleRate;
            auto signal = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(_inputPorts[0]->get());
            auto cutoff_signal = std::dynamic_pointer_cast<Engine::Signal::ControlSignal>(_inputPorts[1]->get());
            
            PCMBuffer inputBuffer = signal ? signal->getBuffer() : PCMBuffer(numSamples, 0.0);
            PCMBuffer outputBuffer(numSamples);

            // Récupérer les valeurs de contrôle (une par sample ou interpoler)
            std::vector<double> envValues;
            if (cutoff_signal && !cutoff_signal->getControlValues().empty()) {
                envValues = cutoff_signal->getControlValues();
            }

            double baseCutoff = _cutoffFrequency->get();
            double envAmount = _envAmount->get();
            double resonance = _resonance->get();
            FilterType filterType = _filterType->get();

            for (size_t i = 0; i < numSamples; ++i) {
                // Calculer le cutoff modulé pour ce sample
                double modulatedCutoff = baseCutoff;
                if (!envValues.empty()) {
                    // Si l'enveloppe a autant de samples que le buffer
                    size_t envIndex = (envValues.size() == numSamples) ? i : 0;
                    modulatedCutoff = baseCutoff + (envValues[envIndex] * envAmount);
                }
                
                // Limiter le cutoff
                modulatedCutoff = std::clamp(modulatedCutoff, 20.0, static_cast<double>(sampleRate / 2 - 100));

                // Recalculer les coefficients biquad pour ce sample
                double w0 = 2.0 * M_PI * modulatedCutoff / sampleRate;
                double cos_w0 = cos(w0);
                double sin_w0 = sin(w0);
                double alpha = sin_w0 / (2.0 * resonance);

                double b0, b1, b2, a0, a1, a2;

                switch (filterType) {
                    case LOW_PASS:
                        b0 = (1 - cos_w0) / 2;
                        b1 = 1 - cos_w0;
                        b2 = (1 - cos_w0) / 2;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case HIGH_PASS:
                        b0 = (1 + cos_w0) / 2;
                        b1 = -(1 + cos_w0);
                        b2 = (1 + cos_w0) / 2;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                    case BAND_PASS:
                        b0 = sin_w0 / 2;
                        b1 = 0;
                        b2 = -sin_w0 / 2;
                        a0 = 1 + alpha;
                        a1 = -2 * cos_w0;
                        a2 = 1 - alpha;
                        break;
                }

                // Appliquer le filtre
                outputBuffer[i] = (b0 / a0) * inputBuffer[i] +
                                  (b1 / a0) * _x1 +
                                  (b2 / a0) * _x2 -
                                  (a1 / a0) * _y1 -
                                  (a2 / a0) * _y2;
                _x2 = _x1;
                _x1 = inputBuffer[i];
                _y2 = _y1;
                _y1 = outputBuffer[i];
            }

            auto audioSignal = std::make_shared<Signal::AudioSignal>(std::move(outputBuffer), numSamples);
            _outputPorts[0]->send(audioSignal);
        }

        void FilterModule::setFilterType(FilterType type) {
            _filterType->set(type);
        }
        void FilterModule::setCutoffFrequency(double frequency) {
            _cutoffFrequency->set(frequency);
        }
        void FilterModule::setResonance(double reso) {
            _resonance->set(reso);
        }
        FilterType FilterModule::getFilterType() const {
            return _filterType->get();
        }
        float FilterModule::getCutoffFrequency() const {
            return _cutoffFrequency->get();
        }
        float FilterModule::getResonance() const {
            return _resonance->get();
        }

        static AutoRegister filterModuleReg{
            "filter",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<FilterModule>(name);
            }
        };
    }
}