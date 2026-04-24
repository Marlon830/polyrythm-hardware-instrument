#include "engine/module/OscModule.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"
#include "engine/signal/ControlSignal.hpp"

#include <iostream>
#include <cmath>
#include <memory>
#include <vector>

namespace Engine {
    namespace Module {

        OscillatorModule::OscillatorModule()
            : _waveform(std::make_shared<Param<WaveformType>>("waveform", SINE)),
              _pitchoffset(std::make_shared<Param<double>>("pitch", 0.0)),
              _phase(0.0) {

            _name = "OscillatorModule";
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_waveform));
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_pitchoffset));

            auto outputPort = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);
            _outputPorts.push_back(outputPort);

            auto inputPort = std::make_shared<Port::InputPort>("FreqCV", Signal::SignalType::CONTROL);
            _inputPorts.push_back(inputPort);
        }

        OscillatorModule::OscillatorModule(std::string name)
            : OscillatorModule() {
            _name = std::move(name);
        }

        OscillatorModule::~OscillatorModule() {
        }


        IModule* OscillatorModule::clone() const {
            OscillatorModule* cloned = new OscillatorModule(*this);
            return cloned;
        }

        void OscillatorModule::process(Core::AudioContext& context) {
            double baseFrequency = context.frequency;
            const size_t numSamples = context.bufferSize;
            const double sampleRate = static_cast<double>(context.sampleRate);

            double pitchSemitones = _pitchoffset->get();
            double frequency = baseFrequency * std::pow(2.0, pitchSemitones / 12.0);

            auto signal = std::dynamic_pointer_cast<Engine::Signal::ControlSignal>(_inputPorts[0]->get());
            const std::vector<double>& cvValues = signal ? signal->getControlValues() : std::vector<double>(numSamples, 1.0);
  
            context.frequency = frequency;

            PCMBuffer outputBuffer(numSamples);
            for (size_t i = 0; i < numSamples; ++i) {
                double currentFrequency = frequency;
                if (signal) {
                    constexpr double pitchModRange = 24.0;
                    double pitchMod = (cvValues[i] - 1.0) * pitchModRange;
                    currentFrequency = frequency * std::pow(2.0, pitchMod / 12.0);
                }
                const double phaseIncrement = currentFrequency / sampleRate;
                double sample = 0.0;
                
                switch (_waveform->get()) {
                    case SINE:
                        sample = std::sin(2.0 * M_PI * _phase);
                        break;
                    case SQUARE:
                        sample = (_phase < 0.5) ? 1.0 : -1.0;
                        break;
                    case TRIANGLE:
                        sample = 4.0 * std::fabs(_phase - 0.5) - 1.0;
                        break;
                    case SAWTOOTH:
                        sample = 2.0 * _phase - 1.0;
                        break;
                }

                outputBuffer[i] = sample;

                _phase += phaseIncrement;
                if (_phase >= 1.0) {
                    _phase -= 1.0;
                }
            }

            if (_outputPorts.empty() || !_outputPorts[0]) {
                std::cerr << "OscillatorModule: no output port 0 available\n";
                return;
            }
            auto audioSignal = std::make_shared<Signal::AudioSignal>(std::move(outputBuffer), numSamples);
            _outputPorts[0]->send(audioSignal);
        }

        void OscillatorModule::setWaveform(WaveformType waveform) {
            _waveform->set(waveform);
        }

        double OscillatorModule::getPhase() const {
            return _phase;
        }
        
        WaveformType OscillatorModule::getWaveform() const {
            return _waveform->get();
        }

        static AutoRegister oscModuleReg{
            "oscillator",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<OscillatorModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine

