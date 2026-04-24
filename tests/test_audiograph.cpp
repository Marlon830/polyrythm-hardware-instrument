#include <gtest/gtest.h>
#include "engine/AudioGraph/AudioGraph.hpp"
#include "engine/module/BaseModule.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/port/OutputPort.hpp"

using namespace Engine;

class TestModule : public Module::BaseModule {
public:
    TestModule(const std::string& name) : _name(name), _processCount(0) {
        auto input = std::make_shared<Port::InputPort>("in", Signal::SignalType::AUDIO);
        auto output = std::make_shared<Port::OutputPort>("out", Signal::SignalType::AUDIO);

        this->getInputPorts().push_back(input);
        this->getOutputPorts().push_back(output);
    }

    void process(Core::AudioContext& context) override {
        this->_processCount++;
        std::cout << "Processing " << this->_name << std::endl;
    }

    int getProcessCount() const { return this->_processCount; }
    std::string getName() const { return this->_name; }

private:
    std::string _name;
    int _processCount;
};

// Tests
TEST(AudioGraphTest, AddModuleSucceeds) {
    AudioGraph graph;
    auto module = std::make_shared<TestModule>("TestModule");
    
    EXPECT_NO_THROW(graph.addModule(module));
}

TEST(AudioGraphTest, ConnectPortsSucceeds) {
    AudioGraph graph;
    auto mod1 = std::make_shared<TestModule>("Module1");
    auto mod2 = std::make_shared<TestModule>("Module2");
    
    graph.addModule(mod1);
    graph.addModule(mod2);
    
    EXPECT_NO_THROW(
        graph.connect(mod1->getOutputPorts()[0], mod2->getInputPorts()[0])
    );
}

TEST(AudioGraphTest, ProcessExecutesModules) {
    AudioGraph graph;
    auto module = std::make_shared<TestModule>("Module");
    
    graph.addModule(module);
    graph.process();
    
    EXPECT_EQ(module->getProcessCount(), 1);
}

TEST(AudioGraphTest, TopologicalOrderLinearChain) {
    AudioGraph graph;
    auto modA = std::make_shared<TestModule>("A");
    auto modB = std::make_shared<TestModule>("B");
    auto modC = std::make_shared<TestModule>("C");
    
    // Ajouter dans le désordre
    graph.addModule(modC);
    graph.addModule(modA);
    graph.addModule(modB);
    
    // Connecter A -> B -> C
    graph.connect(modA->getOutputPorts()[0], modB->getInputPorts()[0]);
    graph.connect(modB->getOutputPorts()[0], modC->getInputPorts()[0]);
    
    EXPECT_NO_THROW(graph.process());
}

TEST(AudioGraphTest, NullModuleThrows) {
    AudioGraph graph;
    EXPECT_THROW(graph.addModule(nullptr), std::invalid_argument);
}

TEST(AudioGraphTest, NullPortsThrow) {
    AudioGraph graph;
    auto module = std::make_shared<TestModule>("Module");
    
    graph.addModule(module);
    
    EXPECT_THROW(graph.connect(nullptr, nullptr), std::invalid_argument);
}
