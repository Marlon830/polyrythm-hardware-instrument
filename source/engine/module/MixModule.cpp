#include "engine/module/MixModule.hpp"
#include "engine/core/mix/AditiveMix.hpp"


#include "engine/module/ModuleFactory.hpp"

#include <algorithm>

namespace Engine {
    namespace Module {
        MixModule::MixModule(): MixModule("MixModule") {}

        MixModule::MixModule(std::string name)
            : _input1Level(std::make_shared<Param<double>>("Input1 Level", 0.5)),
              _input2Level(std::make_shared<Param<double>>("Input2 Level", 0.75)) {
            _name = std::move(name);

            auto outputPort = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);
            _outputPorts.push_back(outputPort);

            auto inputPort1 = std::make_shared<Port::InputPort>("in1", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort1);

            auto inputPort2 = std::make_shared<Port::InputPort>("in2", Signal::SignalType::AUDIO);
            _inputPorts.push_back(inputPort2);

            _parameters.push_back(_input1Level);
            _parameters.push_back(_input2Level);
            _mixStrategy = std::make_shared<Core::AditiveMix>();
        }

        MixModule::~MixModule() {
        }

        IModule* MixModule::clone() const {
            return new MixModule(*this);
        }

        void MixModule::process(Core::AudioContext& context) {
            auto signal1 = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(_inputPorts[0]->get());
            auto signal2 = std::dynamic_pointer_cast<Engine::Signal::AudioSignal>(_inputPorts[1]->get());

            std::vector<std::shared_ptr<Signal::AudioSignal>> signals;
            if (signal1) {
                // Apply input 1 level
                PCMBuffer buffer1 = signal1->getBuffer();
                double level1 = std::clamp(_input1Level->get(), 0.0, 1.0);
                for (auto& sample : buffer1) {
                    sample *= level1;
                }
                signals.push_back(std::make_shared<Signal::AudioSignal>(std::move(buffer1), signal1->getBufferSize()));
            }
            if (signal2) {
                // Apply input 2 level
                PCMBuffer buffer2 = signal2->getBuffer();
                double level2 = std::clamp(_input2Level->get(), 0.0, 1.0);
                for (auto& sample : buffer2) {
                    sample *= level2;
                }
                signals.push_back(std::make_shared<Signal::AudioSignal>(std::move(buffer2), signal2->getBufferSize()));
            }

            auto mixedSignal = _mixStrategy->mix(signals);
            _outputPorts[0]->send(mixedSignal);
        }


        static AutoRegister oscModuleReg{
            "mix",
            [](std::string name) -> std::shared_ptr<IModule> {
                return std::make_shared<MixModule>(name);
            }
        };
    } // namespace Module
} // namespace Engine