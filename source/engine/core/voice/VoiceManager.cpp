#include "engine/core/voice/VoiceManager.hpp"
#include <algorithm>

namespace Engine {
    namespace Core {

        VoiceManager::VoiceManager(size_t maxVoices)
            : _maxVoices(maxVoices) {
            _mixStrategy = std::make_shared<AditiveMix>();
            for (size_t i = 0; i < _maxVoices; ++i) {
                _voices.push_back(std::make_shared<Voice>(AudioGraph()));
            }
        }

        VoiceManager::~VoiceManager() {
        }

        std::shared_ptr<Voice> VoiceManager::noteOn(int noteNumber, float velocity, AudioGraph& graph) {
            auto voice = findFreeVoice();
            if (voice) {
                voice->noteOn(noteNumber, velocity);
                voice->setAudioGraph(graph);
                _activeVoices.push_back(voice);
            }
            return voice;
        }

        void VoiceManager::noteOff(int noteNumber) {
            for (auto& voice : _activeVoices) {
                if (voice->isActive() && voice->getNoteNumber() == noteNumber) {
                    voice->noteOff(noteNumber);
                }
            }
        }

        void VoiceManager::removeInactiveVoices() {
            _activeVoices.erase(std::remove_if(_activeVoices.begin(), _activeVoices.end(),
                [](const std::shared_ptr<Voice>& v) { return v->isInactive(); }),
                _activeVoices.end());
        }

        std::shared_ptr<Voice> VoiceManager::findFreeVoice() {
            for (auto& voice : _voices) {
                if (voice->isInactive()) {
                    return voice;
                }
            }
            return nullptr; // No free voice available implement voice stealing if needed
        }
    } // namespace Core
} // namespace Engine