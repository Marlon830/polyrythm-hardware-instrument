#include "engine/module/ADSRModule.hpp"
#include "engine/module/ModuleFactory.hpp"

#include <iostream>
#include <cmath>

using namespace std::chrono;

namespace Engine {
    namespace Module {

        ADSRModule::ADSRModule() 
            : ADSRModule("ADSR") {
        }

        ADSRModule::ADSRModule(std::string name) 
            : _attackTimeParam(std::make_shared<Param<double>>("attack", 0.1)),
              _decayTimeParam(std::make_shared<Param<double>>("decay", 0.1)),
              _sustainLevelParam(std::make_shared<Param<double>>("sustain", 0.5)),
              _releaseTimeParam(std::make_shared<Param<double>>("release", 0.5)),
              _stateElapsedSec(0.0),
              _releaseStartLevel(0.0) {
            _name = std::move(name);
            auto out = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);
            _outputPorts.push_back(out);
            auto in = std::make_shared<Port::InputPort>("in", Signal::SignalType::AUDIO);
            _inputPorts.push_back(in);
            _parameters.push_back(_attackTimeParam);
            _parameters.push_back(_decayTimeParam);
            _parameters.push_back(_sustainLevelParam);
            _parameters.push_back(_releaseTimeParam);
        }

        ADSRModule::~ADSRModule() {
        }

        IModule* ADSRModule::clone() const {
            ADSRModule* cloned = new ADSRModule(*this);
            // IMPORTANT: Reset complet de l'état pour une nouvelle voix
            cloned->_state = OFF;
            cloned->_currentLevel = 0.0;
            cloned->_stateElapsedSec = 0.0;
            cloned->_releaseStartLevel = 0.0;
            return cloned;
        }

        void ADSRModule::process(Core::AudioContext& context) {
            const size_t numSamples = context.bufferSize;
            const double sampleRate = static_cast<double>(context.sampleRate);
            bool isNoteOn = context.isNoteOn;
            auto signal = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(_inputPorts[0]->get());

            PCMBuffer inputBuffer = signal ? signal->getBuffer() : PCMBuffer(numSamples, 0.0);
            PCMBuffer outputBuffer(numSamples);

            // param adsr second
            double attackSec = _attackTimeParam->get();
            double decaySec = _decayTimeParam->get();
            double releaseSec = _releaseTimeParam->get();
            double sustainLevel = _sustainLevelParam->get();

            // Coefficients exponentiels (time constant)
            // Plus la constante est petite, plus la courbe est rapide
            constexpr double TARGET_RATIO = 0.001; // -60dB
            
            auto calcCoeff = [sampleRate, TARGET_RATIO](double timeSec) -> double {
                if (timeSec <= 0.0) return 0.0;
                return std::exp(-std::log((1.0 + TARGET_RATIO) / TARGET_RATIO) / (timeSec * sampleRate));
            };

            double attackCoeff = calcCoeff(attackSec);
            double decayCoeff = calcCoeff(decaySec);
            double releaseCoeff = calcCoeff(releaseSec);

            // Cibles pour les courbes exponentielles
            // On dépasse légèrement la cible pour compenser l'asymptote
            double attackTarget = 1.0 + TARGET_RATIO;
            double decayTarget = sustainLevel - TARGET_RATIO;
            double releaseTarget = -TARGET_RATIO;

            for (size_t i = 0; i < numSamples; ++i) {
                // Vérifier les transitions d'état
                if (isNoteOn && _state == OFF) {
                    // std::cerr << "ADSR : ATTACK" << std::endl;
                    _state = ATTACK;
                    // Pas de reset de _currentLevel pour éviter les clics
                } else if (!isNoteOn && _state != RELEASE && _state != OFF) {
                    // std::cerr << "ADSR : RELEASE" << std::endl;
                    _state = RELEASE;
                    _releaseStartLevel = _currentLevel;
                }

                // Mettre à jour le niveau selon l'état (courbes exponentielles)
                switch (_state) {
                    case ATTACK: {
                        if (attackSec <= 0.0) {
                            _currentLevel = 1.0;
                            // std::cerr << "ADSR : DECAY" << std::endl;
                            _state = DECAY;
                        } else {
                            // Courbe exponentielle vers attackTarget
                            _currentLevel = attackTarget + (_currentLevel - attackTarget) * attackCoeff;
                            if (_currentLevel >= 1.0) {
                                _currentLevel = 1.0;
                                // std::cerr << "ADSR : DECAY" << std::endl;
                                _state = DECAY;
                            }
                        }
                        break;
                    }
                    case DECAY: {
                        if (decaySec <= 0.0) {
                            _currentLevel = sustainLevel;
                            // std::cerr << "ADSR : SUSTAIN" << std::endl;
                            _state = SUSTAIN;
                        } else {
                            _currentLevel = decayTarget + (_currentLevel - decayTarget) * decayCoeff;
                            if (_currentLevel <= sustainLevel) {
                                _currentLevel = sustainLevel;
                                // std::cerr << "ADSR : SUSTAIN" << std::endl;
                                _state = SUSTAIN;
                            }
                        }
                        break;
                    }
                    case SUSTAIN: {
                        _currentLevel = sustainLevel;
                        break;
                    }
                    case RELEASE: {
                        if (releaseSec <= 0.0) {
                            _currentLevel = 0.0;
                                // std::cerr << "ADSR : OFF" << std::endl;
                            _state = OFF;
                        } else {
                            _currentLevel = releaseTarget + (_currentLevel - releaseTarget) * releaseCoeff;
                            if (_currentLevel <= 0.0001) {
                                _currentLevel = 0.0;
                                // std::cerr << "ADSR : OFF" << std::endl;
                                _state = OFF;
                            }
                        }
                        break;
                    }
                    case OFF: {
                        _currentLevel = 0.0;
                        break;
                    }
                }

                // Appliquer le niveau à cet échantillon
                outputBuffer[i] = inputBuffer[i] * _currentLevel;
            }
            
            // envoi du signal de sortie
            auto outSignal = std::make_shared<Engine::Signal::AudioSignal>(outputBuffer, numSamples);
            _outputPorts[0]->send(outSignal);
        }

        void ADSRModule::setState(ADSRState state) {
            if (state == _state) return;
            _state = state;
            _stateElapsedSec = 0.0;
            if (state == RELEASE) {
                _releaseStartLevel = _currentLevel;
            }
        }

        static AutoRegister adsrModuleReg{
            "adsr",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<ADSRModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine