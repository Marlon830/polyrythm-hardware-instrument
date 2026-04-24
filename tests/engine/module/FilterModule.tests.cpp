#include "engine/module/OscModule.hpp"
#include "engine/module/FilterModule.hpp"
#include "engine/signal/AudioSignal.hpp"
#include "engine/port/InputPort.hpp"

#include <memory>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>
namespace Module = Engine::Module;
namespace Signal = Engine::Signal;
namespace Port = Engine::Port;
namespace Core = Engine::Core;

TEST(FilterModuleTest, signalOutputSend) {
    Module::OscillatorModule oscModule;
    Module::FilterModule filterModule;

    oscModule.setWaveform(Module::SINE);

    filterModule.setFilterType(Module::LOW_PASS);
    filterModule.setCutoffFrequency(1000.0f);
    filterModule.setResonance(0.7f);

    Core::AudioContext context{64, 44100, 440.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Engine::Port::InputPort> inputPort = std::make_shared<Engine::Port::InputPort>("Test Input", Engine::Signal::SignalType::AUDIO);

    oscModule.getOutputPorts()[0]->connect(filterModule.getInputPorts()[0]);
    filterModule.getOutputPorts()[0]->connect(inputPort);

    oscModule.process(context);
    filterModule.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());

    EXPECT_NE(receivedSignal, nullptr);
    EXPECT_EQ(receivedSignal->getBufferSize(), context.bufferSize);
}

TEST(FilterModuleTest, parameterAdjustment) {
    Module::FilterModule filterModule;

    filterModule.setFilterType(Module::HIGH_PASS);
    filterModule.setCutoffFrequency(5000.0f);
    filterModule.setResonance(1.2f);

    EXPECT_EQ(filterModule.getFilterType(), Module::HIGH_PASS);
    EXPECT_FLOAT_EQ(filterModule.getCutoffFrequency(), 5000.0f);
    EXPECT_FLOAT_EQ(filterModule.getResonance(), 1.2f);
}