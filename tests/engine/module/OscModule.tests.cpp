#include "engine/module/OscModule.hpp"
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

TEST(OscillatorModuleTest, signalOutputSend) {
    Module::OscillatorModule module;
    module.setWaveform(Module::SINE);

    Core::AudioContext context{64, 44100, 4410.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Engine::Port::InputPort> inputPort = std::make_shared<Engine::Port::InputPort>("Test Input", Engine::Signal::SignalType::AUDIO);

    module.getOutputPorts()[0]->connect(inputPort);

    module.process(context);
    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());

    EXPECT_NE(receivedSignal, nullptr);
    EXPECT_EQ(receivedSignal->getBufferSize(), context.bufferSize);
}

TEST(OscillatorModuleTest, SetFrequencyAndWaveform) {
    Module::OscillatorModule module;

    module.setWaveform(Module::SQUARE);
    EXPECT_EQ(module.getWaveform(), Module::SQUARE);

    module.setWaveform(Module::TRIANGLE);
    EXPECT_EQ(module.getWaveform(), Module::TRIANGLE);

    module.setWaveform(Module::SAWTOOTH);
    EXPECT_EQ(module.getWaveform(), Module::SAWTOOTH);
}

TEST(OscillatorModuleTest, ProcessWaveSine) {
    Module::OscillatorModule module;
    module.setWaveform(Module::SINE);

    Core::AudioContext context{15, 44100, 4410.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Port::InputPort> inputPort = std::make_shared<Port::InputPort>("Test Input", Signal::SignalType::AUDIO);

    module.getOutputPorts()[0]->connect(inputPort);

    module.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());

    // Basic checks
    ASSERT_NE(receivedSignal, nullptr);
    std::vector<double> buffer = receivedSignal->getBuffer();
    ASSERT_EQ(buffer.size(), context.bufferSize);

    const std::vector<double> expected = {
        0.0,
        0.5877852522924731,
        0.9510565162951535,
        0.9510565162951536,
        0.5877852522924734,
        0.0,
        -0.5877852522924731,
        -0.9510565162951535,
        -0.9510565162951536,
        -0.5877852522924734,
        0.0,
        0.5877852522924731,
        0.9510565162951535,
        0.9510565162951536,
        0.5877852522924734
    };

    const double tol = 1e-6;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(expected[i], buffer[i], tol) << "Mismatch at sample " << i;
    }
}

TEST(OscillatorModuleTest, ProcessWaveSquare) {
    Module::OscillatorModule module;
    module.setWaveform(Module::SQUARE);

    Core::AudioContext context{10, 44100, 4410.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Engine::Port::InputPort> inputPort = std::make_shared<Engine::Port::InputPort>("Test Input", Engine::Signal::SignalType::AUDIO);

    module.getOutputPorts()[0]->connect(inputPort);

    module.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());


    // Basic checks
    ASSERT_NE(receivedSignal, nullptr);
    std::vector<double> buffer = receivedSignal->getBuffer();
    ASSERT_EQ(buffer.size(), context.bufferSize);

    const std::vector<double> expected = {
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        -1.0,
        -1.0,
        -1.0,
        -1.0,
        -1.0
    };

    const double tol = 1e-6;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(expected[i], buffer[i], tol) << "Mismatch at sample " << i;
    }
}

TEST(OscillatorModuleTest, ProcessWaveSawtooth) {
    Module::OscillatorModule module;
    module.setWaveform(Module::SAWTOOTH);

    Core::AudioContext context{10, 44100, 4410.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Engine::Port::InputPort> inputPort = std::make_shared<Engine::Port::InputPort>("Test Input", Engine::Signal::SignalType::AUDIO);

    module.getOutputPorts()[0]->connect(inputPort);

    module.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());

    // Basic checks
    ASSERT_NE(receivedSignal, nullptr);
    std::vector<double> buffer = receivedSignal->getBuffer();
    ASSERT_EQ(buffer.size(), context.bufferSize);

    const std::vector<double> expected = {
        -1.0,
        -0.8,
        -0.6,
        -0.4,
        -0.2,
        0.0,
        0.2,
        0.4,
        0.6,
        0.8
    };

    const double tol = 1e-6;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(expected[i], buffer[i], tol) << "Mismatch at sample " << i;
    }
}

TEST(OscillatorModuleTest, ProcessWaveTriangle) {
    Module::OscillatorModule module;
    module.setWaveform(Module::TRIANGLE);

    Core::AudioContext context{10, 44100, 4410.0f};

    std::shared_ptr<Signal::AudioSignal> receivedSignal = nullptr;

    std::shared_ptr<Port::InputPort> inputPort = std::make_shared<Port::InputPort>("Test Input", Signal::SignalType::AUDIO);

    module.getOutputPorts()[0]->connect(inputPort);

    module.process(context);

    receivedSignal = std::dynamic_pointer_cast<Signal::AudioSignal>(inputPort->get());


    // Basic checks
    ASSERT_NE(receivedSignal, nullptr);
    std::vector<double> buffer = receivedSignal->getBuffer();
    ASSERT_EQ(buffer.size(), context.bufferSize);
    
    const std::vector<double> expected = {
        1.0,
        0.6,
        0.2,
        -0.2,
        -0.6,
        -1.0,
        -0.6,
        -0.2,
        0.2,
        0.6
    };

    const double tol = 1e-6;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(expected[i], buffer[i], tol) << "Mismatch at sample " << i;
    }
}