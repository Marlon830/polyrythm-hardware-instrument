#include "engine/core/voice/Voice.hpp"

namespace Engine {
    namespace Core {

        Voice::Voice(AudioGraph audioGraph)
            : _state(INACTIVE), _noteNumber(-1), _velocity(0.0f), _audioGraph(audioGraph) {
            }

            Voice::~Voice() {
            }

            void Voice::noteOn(int noteNumber, float velocity) {
                _noteNumber = noteNumber;
                _velocity = velocity;
                _state = ACTIVE;
            }

            void Voice::noteOff(int noteNumber) {
                if (_noteNumber == noteNumber) {
                    _state = RELEASED;
                }
            }

            bool Voice::isActive() const {
                return _state == ACTIVE; //note add RELEASED state handling when implementing envelopes
            }

            bool Voice::isReleased() const {
                return _state == RELEASED;
            }

            bool Voice::isInactive() const {
                return _state == INACTIVE;
            }

            void Voice::setState(VoiceState state) {
                _state = state;
            }
    } // namespace Core
} // namespace Engine