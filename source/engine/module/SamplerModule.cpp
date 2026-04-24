#include "engine/module/SamplerModule.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Engine {
    namespace Module {

        SamplerModule::SamplerModule()
            : BaseModule(), _maxVoices(16), _samples(std::make_shared<Param<std::map<int, Util::WavData>>>(
                "Samples", std::map<int, Util::WavData>{})) {

            _name = "SamplerModule";
            
            // Create input port for event signals (NOTE_ON/OFF)
            auto eventInputPort = std::make_shared<Port::InputPort>(
                "Sampler Event Input", 
                Signal::SignalType::EVENT
            );
            this->_inputPorts.push_back(eventInputPort);

            // Create output port for mixed audio
            auto audioOutputPort = std::make_shared<Port::OutputPort>(
                "Sampler Audio Output", 
                Signal::SignalType::AUDIO
            );
            this->_outputPorts.push_back(audioOutputPort);

            // Initialize voice pool
            this->_voices.resize(this->_maxVoices);

            std::shared_ptr<Param<std::map<int, Util::WavData>>> samplesParam = 
                std::make_shared<Param<std::map<int, Util::WavData>>>("Samples", std::map<int, Util::WavData>{});

            _parameters.push_back(_samples);
            
            std::cout << "SamplerModule initialized with " << this->_maxVoices 
                      << " voices" << std::endl;
        }

        SamplerModule::SamplerModule(std::string name)
            : BaseModule(), _maxVoices(16), _samples(std::make_shared<Param<std::map<int, Util::WavData>>>(
                "Samples", std::map<int, Util::WavData>{})) {
            _name = std::move(name);

            // Create input port for event signals (NOTE_ON/OFF)
            auto eventInputPort = std::make_shared<Port::InputPort>(
                "Sampler Event Input", 
                Signal::SignalType::EVENT
            );
            this->_inputPorts.push_back(eventInputPort);

            // Create output port for mixed audio
            auto audioOutputPort = std::make_shared<Port::OutputPort>(
                "Sampler Audio Output", 
                Signal::SignalType::AUDIO
            );
            this->_outputPorts.push_back(audioOutputPort);

            // Initialize voice pool
            this->_voices.resize(this->_maxVoices);
            
            _parameters.push_back(_samples);

            std::cout << "SamplerModule initialized with " << this->_maxVoices 
                      << " voices" << std::endl;
        }

        SamplerModule::~SamplerModule() {
        }

        IModule* SamplerModule::clone() const {
            return new SamplerModule(*this);
        }

        std::map<int, Util::WavData> SamplerModule::getSamples() const {
            return this->_samples->get();
        }

        size_t SamplerModule::getLoadedSamplesCount() const {
            return this->_samples->get().size();
        }

        void SamplerModule::clearSamples() {
            this->_samples->get().clear();
            std::cout << "SamplerModule: Cleared all samples" << std::endl;
        }

        void SamplerModule::triggerSample(int noteNumber, double velocity) {
            auto samples = this->_samples->get();
            auto it = samples.find(noteNumber);

            if (it == samples.end()) {
                std::cerr << "SamplerModule: No sample loaded for note " 
                          << noteNumber << std::endl;
                return;
            }

            // Find free voice
            SampleVoice* voice = this->findFreeVoice();
            if (!voice) {
                std::cerr << "SamplerModule: No free voices available" << std::endl;
                return;
            }

            // Initialize voice
            voice->sampleData = it->second.samples;
            voice->playbackPosition = 0;
            voice->noteNumber = noteNumber;
            voice->velocity = velocity;
            voice->isPlaying = true;
            voice->sampleRate = it->second.sampleRate;

            std::cout << "SamplerModule: Triggered sample for note " 
                      << noteNumber << " (velocity: " << velocity << ")" << std::endl;
        }

        void SamplerModule::stopSample(int noteNumber) {
            // Stop all voices playing this note
            for (auto& voice : this->_voices) {
                if (voice.isPlaying && voice.noteNumber == noteNumber) {
                    voice.isPlaying = false;
                    // std::cout << "SamplerModule: Stopped sample for note " 
                    //           << noteNumber << std::endl;
                }
            }
        }

        SampleVoice* SamplerModule::findFreeVoice() {
            for (auto& voice : this->_voices) {
                if (!voice.isPlaying) {
                    return &voice;
                }
            }
            return nullptr;
        }

        std::shared_ptr<Signal::AudioSignal> SamplerModule::mixVoices(
            const Core::AudioContext& context) {
            
            PCMBuffer mixedBuffer(context.bufferSize, 0.0);

            // Mix all active voices
            for (auto& voice : this->_voices) {
                if (!voice.isPlaying) continue;

                for (size_t i = 0; i < context.bufferSize; ++i) {
                    if (voice.playbackPosition >= voice.sampleData.size()) {
                        // Sample finished playing
                        voice.isPlaying = false;
                        break;
                    }

                    // Apply velocity scaling
                    double sample = voice.sampleData[voice.playbackPosition] * voice.velocity;
                    mixedBuffer[i] += sample;
                    
                    voice.playbackPosition++;
                }
            }

            // Normalize to prevent clipping
            double maxAbs = 0.0;
            for (const auto& sample : mixedBuffer) {
                double abs = std::fabs(sample);
                if (abs > maxAbs) maxAbs = abs;
            }

            if (maxAbs > 1.0) {
                double scale = 1.0 / maxAbs;
                for (auto& sample : mixedBuffer) {
                    sample *= scale;
                }
            }

            return std::make_shared<Signal::AudioSignal>(
                std::move(mixedBuffer), 
                context.bufferSize
            );
        }

        void SamplerModule::process(Core::AudioContext& context) {
            // Process incoming NOTE_ON/OFF events
            std::shared_ptr<Signal::EventSignal> eventSignal;
            
            while ((eventSignal = std::dynamic_pointer_cast<Signal::EventSignal>(
                this->_inputPorts[0]->get())) != nullptr) {
                
                if (eventSignal->getEventType() == Signal::EventType::NOTE_ON) {
                    int note = static_cast<int>(eventSignal->getData()[0]);
                    double velocity = static_cast<double>(eventSignal->getData()[1]) / 127.0;
                    this->triggerSample(note, velocity);
                    
                } else if (eventSignal->getEventType() == Signal::EventType::NOTE_OFF) {
                    int note = static_cast<int>(eventSignal->getData()[0]);
                    this->stopSample(note);
                }
            }

            // Mix active voices and send output
            auto mixedSignal = this->mixVoices(context);
            this->_outputPorts[0]->send(mixedSignal);
        }

        static AutoRegister samplerModuleReg{
            "sampler",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<SamplerModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine