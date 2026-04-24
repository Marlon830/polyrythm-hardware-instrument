#include "engine/module/StepSequencerModule.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"
#include <iostream>
#include <cmath>

namespace Engine
{
    namespace Module
    {

        StepSequencerModule::StepSequencerModule()
            : _tracks(std::make_shared<Param<std::vector<SequencerTrack>>>("Tracks", std::vector<SequencerTrack>())), _currentStep(0), _stepCount(std::make_shared<Param<size_t>>("Step Count", 16)), _bpm(std::make_shared<Param<float>>("BPM", 60)), _sampleCounter(0), _samplesPerStep(0), _lastStepTime(std::chrono::steady_clock::now()), _isFirstProcess(true)
        {
            _name = "StepSequencerModule";
            // Create output port for event signals
            auto outputPort = std::make_shared<Port::OutputPort>(
                "StepSequencer Event Output",
                Signal::SignalType::EVENT);
            this->_outputPorts.push_back(outputPort);

            std::cerr << "StepSequencerModule initialized with " << _stepCount->get()
                      << " steps at " << _bpm->get() << " BPM" << std::endl;

            _parameters.push_back(std::static_pointer_cast<ParamBase>(_stepCount));
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_bpm));
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_tracks));
        }

        StepSequencerModule::StepSequencerModule(std::string name)
            : _tracks(std::make_shared<Param<std::vector<SequencerTrack>>>("Tracks", std::vector<SequencerTrack>())), _currentStep(0), _stepCount(std::make_shared<Param<size_t>>("Step Count", 16)), _bpm(std::make_shared<Param<float>>("BPM", 60)), _sampleCounter(0), _samplesPerStep(0), _lastStepTime(std::chrono::steady_clock::now()), _isFirstProcess(true)
        {
            _name = std::move(name);
            // Create output port for event signals
            auto outputPort = std::make_shared<Port::OutputPort>(
                "StepSequencer Event Output",
                Signal::SignalType::EVENT);
            this->_outputPorts.push_back(outputPort);

            std::cerr << "StepSequencerModule initialized with " << _stepCount->get()
                      << " steps at " << _bpm->get() << " BPM" << std::endl;

            _parameters.push_back(std::static_pointer_cast<ParamBase>(_stepCount));
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_bpm));
            _parameters.push_back(std::static_pointer_cast<ParamBase>(_tracks));
        }

        StepSequencerModule::~StepSequencerModule()
        {
        }

        IModule* StepSequencerModule::clone() const
        {
            return new StepSequencerModule(*this);
        }

        void StepSequencerModule::calculateStepDuration(float sampleRate)
        {
            // Calculate samples per step
            // 1 beat = 1 quarter note
            // At 120 BPM: 120 beats/minute = 2 beats/second
            // For 16th notes: 4 steps per beat
            float beatsPerSecond = this->_bpm->get() / 60.0f;
            float stepsPerBeat = 4.0f; // 16th notes
            float stepsPerSecond = beatsPerSecond * stepsPerBeat;
            this->_samplesPerStep = static_cast<size_t>(sampleRate / stepsPerSecond);
        }

        void StepSequencerModule::reset()
        {
            this->_currentStep = 0;
            this->_sampleCounter = 0;
            this->_lastStepTime = std::chrono::steady_clock::now();
            this->_isFirstProcess = true;
        }

        void StepSequencerModule::process(Core::AudioContext &context)
        {
            // Calculate step duration if not yet done
            calculateStepDuration(context.sampleRate);
            
            // Handle first process call: play step 0 immediately
            if (this->_isFirstProcess) {
                this->_isFirstProcess = false;
                
                // Play step 0 immediately
                for (const auto &track : this->_tracks->get())
                {
                    if (track.pattern[this->_currentStep])
                    {
                        auto noteOnSignal = std::make_shared<Signal::EventSignal>(
                            Signal::EventType::NOTE_ON,
                            0,
                            track.noteNumber,
                            track.velocity);
                        this->_outputPorts[0]->send(noteOnSignal);

                        auto noteOffSignal = std::make_shared<Signal::EventSignal>(
                            Signal::EventType::NOTE_OFF,
                            context.bufferSize / 4,
                            track.noteNumber,
                            0);
                        this->_outputPorts[0]->send(noteOffSignal);
                    }
                }
            }

            // Update sample counter
            this->_sampleCounter += context.bufferSize;

            // Check if we need to advance to the next step
            if (this->_sampleCounter >= this->_samplesPerStep)
            {
                // Reset counter and advance to next step FIRST
                this->_sampleCounter = 0;
                this->_currentStep = (this->_currentStep + 1) % this->_stepCount->get();
                this->_lastStepTime = std::chrono::steady_clock::now();
                
                // THEN emit NOTE_ON events for the NEW step
                for (const auto &track : this->_tracks->get())
                {
                    if (track.pattern[this->_currentStep])
                    {
                        // Create NOTE_ON event
                        auto noteOnSignal = std::make_shared<Signal::EventSignal>(
                            Signal::EventType::NOTE_ON,
                            0, // offset in samples (immediate)
                            track.noteNumber,
                            track.velocity);
                        this->_outputPorts[0]->send(noteOnSignal);

                        // std::cerr << "StepSequencerModule: Step " << this->_currentStep
                        //           << " - NOTE_ON: " << track.noteNumber
                        //           << " velocity: " << track.velocity << std::endl;

                        // Immediately send NOTE_OFF to simulate a short hit
                        //TODO: In a real step sequencer, note length would be configurable
                        auto noteOffSignal = std::make_shared<Signal::EventSignal>(
                            Signal::EventType::NOTE_OFF,
                            context.bufferSize / 4, // offset slightly to avoid instant cutoff
                            track.noteNumber,
                            0);
                        this->_outputPorts[0]->send(noteOffSignal);
                    }
                }
            }
        }

        static AutoRegister stepSequencerModuleReg{
            "step_sequencer",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<StepSequencerModule>(name);
            }
        };

    } // namespace Module
} // namespace Engine