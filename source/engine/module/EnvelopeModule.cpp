#include "engine/module/EnvelopeModule.hpp"
#include "engine/module/ModuleFactory.hpp"
                #include <iostream>

namespace Engine {
    namespace Module {

        EnvelopeModule::EnvelopeModule()
            : _attackTimeParam(std::make_shared<Param<double>>("attack", 0.0)),
              _decayTimeParam(std::make_shared<Param<double>>("decay", 1.0)),
              _sustainLevelParam(std::make_shared<Param<double>>("sustain", 0.0)),
              _releaseTimeParam(std::make_shared<Param<double>>("release", .0)) {
            _name = "EnvelopeModule";
            
            // Sortie CV uniquement (pas d'entrée audio)
            auto out = std::make_shared<Port::OutputPort>("out", Signal::SignalType::CONTROL);
            _outputPorts.push_back(out);
            
            _parameters.push_back(_attackTimeParam);
            _parameters.push_back(_decayTimeParam);
            _parameters.push_back(_sustainLevelParam);
            _parameters.push_back(_releaseTimeParam);
        }

        EnvelopeModule::EnvelopeModule(std::string name) : EnvelopeModule() {
            _name = std::move(name);
        }

        EnvelopeModule::~EnvelopeModule() {}

        IModule* EnvelopeModule::clone() const {
            return new EnvelopeModule(*this);
        }

        void EnvelopeModule::process(Core::AudioContext& context) {
            const size_t numSamples = context.bufferSize;
            const float sampleRate = context.sampleRate;
            bool isNoteOn = context.isNoteOn;

            double attackSec = _attackTimeParam->get();
            double decaySec = _decayTimeParam->get();
            double releaseSec = _releaseTimeParam->get();
            double sustainLevel = _sustainLevelParam->get();

            // Transition d'état
            if (isNoteOn && _state == OFF) {
                _state = ATTACK;
                _stateElapsedSec = 0.0;
            } else if (!isNoteOn && _state != OFF && _state != RELEASE) {
                _state = RELEASE;
                _releaseStartLevel = _currentLevel;
                _stateElapsedSec = 0.0;
            }

            std::vector<double> controlValues(numSamples);

            for (size_t i = 0; i < numSamples; ++i) {
                double deltaSec = 1.0 / sampleRate;
                _stateElapsedSec += deltaSec;

                switch (_state) {
                    case OFF:
                        _currentLevel = 0.0;
                        break;
                    case ATTACK:
                        if (attackSec > 0.0) {
                            _currentLevel = _stateElapsedSec / attackSec;
                            if (_currentLevel >= 1.0) {
                                _currentLevel = 1.0;
                                _state = DECAY;
                                _stateElapsedSec = 0.0;
                            }
                        } else {
                            _currentLevel = 1.0;
                            _state = DECAY;
                            _stateElapsedSec = 0.0;
                        }
                        break;
                    case DECAY:
                        if (decaySec > 0.0) {
                            _currentLevel = 1.0 - ((_stateElapsedSec / decaySec) * (1.0 - sustainLevel));
                            if (_currentLevel <= sustainLevel) {
                                _currentLevel = sustainLevel;
                                _state = SUSTAIN;
                            }
                        } else {
                            _currentLevel = sustainLevel;
                            _state = SUSTAIN;
                        }
                        break;
                    case SUSTAIN:
                        _currentLevel = sustainLevel;
                        break;
                    case RELEASE:
                        if (releaseSec > 0.0) {
                            _currentLevel = _releaseStartLevel * (1.0 - (_stateElapsedSec / releaseSec));
                            if (_currentLevel <= 0.0) {
                                _currentLevel = 0.0;
                                _state = OFF;
                            }
                        } else {
                            _currentLevel = 0.0;
                            _state = OFF;
                        }
                        break;
                }
                controlValues[i] = _currentLevel;
            }
            auto controlSignal = std::make_shared<Signal::ControlSignal>(controlValues);
            _outputPorts[0]->send(controlSignal);
        }

        static AutoRegister envelopeModuleReg{
            "envelope",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<EnvelopeModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine