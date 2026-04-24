#include "engine/module/OscModule.hpp"
#include "engine/module/InstrumentAudioOutModule.hpp"
#include "engine/module/FilterModule.hpp"
#include "engine/signal/AudioSignal.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/module/Instrument.hpp"
#include "engine/AudioGraph/AudioGraph.hpp"

#include <memory>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>
namespace Module = Engine::Module;
namespace Signal = Engine::Signal;
namespace Port = Engine::Port;
namespace Core = Engine::Core;

TEST(InstrumentModuleTest, signalOutputSend) {
    Module::Instrument instrumentModule;
    Engine::AudioGraph instrumentGraph;

    Core::AudioContext context{512, 48000, 440.0f};

    Module::OscillatorModule oscModule;
    Module::InstrumentAudioOutModule instrumentAudioOutModule;

    instrumentGraph.addModule(std::make_shared<Module::OscillatorModule>(oscModule));
    instrumentGraph.addModule(std::make_shared<Module::InstrumentAudioOutModule>(instrumentAudioOutModule));

    instrumentGraph.connect(
        oscModule.getOutputPorts()[0],
        instrumentAudioOutModule.getInputPorts()[0]
    );

    instrumentModule.setMainGraph(instrumentGraph);

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;
    std::shared_ptr<Engine::Port::InputPort> inputPort = std::make_shared<Engine::Port::InputPort>("Test Input", Engine::Signal::SignalType::AUDIO);

    instrumentModule.getOutputPorts()[0]->connect(inputPort);
    instrumentModule.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());

    EXPECT_NE(receivedSignal, nullptr);
    EXPECT_EQ(receivedSignal->getBufferSize(), context.bufferSize);
}