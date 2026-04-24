#pragma once

#include "engine/module/BaseModule.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/signal/ControlSignal.hpp"
#include "engine/parameter/Param.hpp"
#include "engine/parameter/Parameters.hpp"

namespace Engine {
    namespace Module {

        class EnvelopeModule : public BaseModule, public Parameters {
        public:
            EnvelopeModule();
            explicit EnvelopeModule(std::string name);
            ~EnvelopeModule() override;

            IModule* clone() const override;
            void process(Core::AudioContext& context) override;

        private:
            enum EnvState { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
            EnvState _state = OFF;
            double _currentLevel = 0.0;
            double _stateElapsedSec = 0.0;
            double _releaseStartLevel = 0.0;

            std::shared_ptr<Param<double>> _attackTimeParam;
            std::shared_ptr<Param<double>> _decayTimeParam;
            std::shared_ptr<Param<double>> _sustainLevelParam;
            std::shared_ptr<Param<double>> _releaseTimeParam;
        };

    } // namespace Module
} // namespace Engine