#include "engine/module/Instrument.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/signal/AudioSignal.hpp"
#include "engine/module/ModuleFactory.hpp"
#include "engine/module/InstrumentAudioOutModule.hpp"
#include "engine/parameter/Parameters.hpp"
#include "engine/module/OscModule.hpp"
#include <iostream>
#include <cmath>

namespace Engine {
    namespace Module {
        Instrument::Instrument() : _voiceManager(16) {
            _name = "InstrumentModule";

            auto eventInputPort = std::make_shared<Port::InputPort>("Instrument Event Input", Signal::SignalType::EVENT);
            _inputPorts.push_back(eventInputPort);

            auto audioInputPort = std::make_shared<Port::InputPort>("Instrument Audio Input", Signal::SignalType::AUDIO);
            _inputPorts.push_back(audioInputPort);
            _audioInputPort = audioInputPort;

            auto outputPort = std::make_shared<Port::OutputPort>("Instrument Audio Output", Signal::SignalType::AUDIO);
            _outputPorts.push_back(outputPort);
        }

        Instrument::Instrument(std::string name) : _voiceManager(16) {
            _name = std::move(name);

            auto eventInputPort = std::make_shared<Port::InputPort>("Instrument Event Input", Signal::SignalType::EVENT);
            _inputPorts.push_back(eventInputPort);

            auto audioInputPort = std::make_shared<Port::InputPort>("Instrument Audio Input", Signal::SignalType::AUDIO);
            _inputPorts.push_back(audioInputPort);
            _audioInputPort = audioInputPort;

            auto outputPort = std::make_shared<Port::OutputPort>("Instrument Audio Output", Signal::SignalType::AUDIO);
            _outputPorts.push_back(outputPort);
        }

        Instrument::~Instrument() {
        }

        IModule* Instrument::clone() const {
            return new Instrument(*this);
        }

        void Instrument::setMainGraph(const AudioGraph& graph) {
            _graph = graph;
        }

        void Instrument::process(Core::AudioContext& context) {
            _voiceManager.removeInactiveVoices();
            std::shared_ptr<Engine::Signal::EventSignal> signal;
            for (; (signal = std::dynamic_pointer_cast<Engine::Signal::EventSignal>(_inputPorts[0]->get())) != nullptr;) {
                if (signal && signal->getEventType() == Signal::EventType::NOTE_ON) {
                    AudioGraph* graphCopy = _graph.clone();
                    // Connect the audio input port of the instrument to the audio output port of the InstrumentAudioOutModule in the cloned graph
                    for (const auto& module : graphCopy->getModules()) {
                        if (module->getName().find("instrument_audio_out") != std::string::npos) {
                            // std::cerr << "Connecting audio input port to InstrumentAudioOutModule in cloned graph." << std::endl;
                            auto outModule = std::static_pointer_cast<InstrumentAudioOutModule>(module);
                            if (outModule) {
                                std::shared_ptr<Port::OutputPort> outPort = outModule->getOutputPortByName("out");
                                graphCopy->setContext(context);

                                // std::cerr << "CONNECT Found output port on " << outModule->getName() << std::endl;
                            } else {
                                std::cerr << "Failed to cast to InstrumentAudioOutModule." << std::endl;
                            }
                        }
                    }
                    _voiceManager.noteOn(static_cast<int>(signal->getData()[0]), static_cast<double>(signal->getData()[1]) / 127.0, *graphCopy);
                } else if (signal && signal->getEventType() == Signal::EventType::NOTE_OFF) {
                    _voiceManager.noteOff(static_cast<int>(signal->getData()[0]));
                }
            }
            
            std::vector<std::shared_ptr<Engine::Signal::AudioSignal>> audioSignals;
            for (const auto & voice : _voiceManager.getActiveVoices()) {
                context.frequency = 440.0 * pow(2.0, (voice->getNoteNumber() - 69) / 12.0);
                if (voice->isActive()) {
                    context.isNoteOn = true;
                } else {
                    context.isNoteOn = false;
                }
                voice->getAudioGraph().setContext(context);
                voice->getAudioGraph().process();

                std::shared_ptr<Engine::Signal::AudioSignal> voiceSignal = nullptr;
                for (const auto& module : voice->getAudioGraph().getModules()) {
                    if (module->getName().find("instrument_audio_out") != std::string::npos) {
                        auto outModule = std::static_pointer_cast<InstrumentAudioOutModule>(module);
                        if (outModule) {
                            auto outPort = outModule->getOutputPortByName("out");
                            if (outPort) {
                                voiceSignal = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(outPort->getLastSignal());
                            }
                        }
                        break;
                    }
                }

                if (voiceSignal) {
                    if (voice->isReleased() && !context.isNoteOn) {
                        voice->setState(Core::VoiceState::INACTIVE);
                    }
                    audioSignals.push_back(voiceSignal);
                }
            }
            if (!audioSignals.empty()) {
                _outputPorts[0]->send(_voiceManager.getMixStrategy()->mix(audioSignals));
            } else {
                // Send silent signal if no active voices
                PCMBuffer silentBuffer(context.bufferSize, 0.0);
                std::cerr << "Instrument: Sending silent buffer." << std::endl;
                auto silentSignal = std::make_shared<Engine::Signal::AudioSignal>(silentBuffer, context.bufferSize);
                _outputPorts[0]->send(silentSignal);
            }
        }

        std::map<std::string, Common::ModuleInfo> Instrument::getAllParams() {
            return _graph.getAllParams();
        }

        static AutoRegister instrumentModuleReg{
            "instrument",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<Instrument>(name);
            }
        };

    } // namespace Module
}