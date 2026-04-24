#include "engine/module/AudioOutModule.hpp"

#include "engine/core/backend/RtAudioOutputBackend.hpp" // temporary move to a factory later
#include "engine/core/backend/AlsaOutputBackend.hpp"
#include "engine/module/ModuleFactory.hpp"

#include "engine/port/InputPort.hpp"
#include "engine/signal/AudioSignal.hpp"
#include "common/event/AudioWaveEvent.hpp"

#include <memory>
#include <stdexcept>

namespace Engine {
    namespace Module {
        AudioOutModule::AudioOutModule() {
            _name = "AudioOutModule";
            _audioBackend = std::make_shared<Core::RtAudioOutputBackend>();
            if (!_audioBackend->open()) {
                // Handle error opening backend
                throw std::runtime_error("Failed to open ALSA output backend");
            }

            auto inputPort = std::make_shared<Port::InputPort>("AudioIn", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort);
        }

        AudioOutModule::AudioOutModule(std::string name) {
            _name = std::move(name);
            _audioBackend = std::make_shared<Core::RtAudioOutputBackend>();
            if (!_audioBackend->open()) {
                // Handle error opening backend
                throw std::runtime_error("Failed to open ALSA output backend");
            }

            auto inputPort = std::make_shared<Port::InputPort>("AudioIn", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort);
        }

        AudioOutModule::~AudioOutModule() {
            _audioBackend->close();
        }

        IModule* AudioOutModule::clone() const {
            return new AudioOutModule(*this);
        }

        void AudioOutModule::process(Core::AudioContext& context) {
            if (_inputPorts.empty() || !_inputPorts[0]) {
                // No input port available
                return;
            }

            auto signal = _inputPorts[0]->get();
            if (!signal) {
                // No signal received
                return;
            }

            auto audioSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(signal);
            if (!audioSignal) {
                // Received signal is not an audio signal
                return;
            }

            const PCMBuffer& buffer = audioSignal->getBuffer();
            if (_eventEmitter) {
                _eventEmitter->emit(std::make_unique<Common::AudioWaveEvent>(buffer.data(), buffer.size()));
            }
            _audioBackend->write(buffer.data(), buffer.size());
        }

        static AutoRegister audioOutModuleReg{
            "audio_out",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<AudioOutModule>(name);
            }
        };
    } // namespace Module
} // namespace Engine