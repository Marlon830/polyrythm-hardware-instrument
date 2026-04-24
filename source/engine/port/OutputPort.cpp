#include "engine/port/OutputPort.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/signal/ISignal.hpp"

#include <iostream>
#include <algorithm>

namespace Engine {
    namespace Port {
        OutputPort::OutputPort(const std::string& name, Signal::SignalType type)
            : Port(name, type) {}

        void OutputPort::connect(std::shared_ptr<InputPort> input) {
            _connections.push_back(input);
        }

        void OutputPort::disconnect(std::shared_ptr<InputPort> input) {
            auto it = std::remove_if(_connections.begin(), _connections.end(),
                                     [&input](const std::shared_ptr<InputPort>& port) {
                                         return port == input;
                                     });
            if (it != _connections.end()) {
                _connections.erase(it, _connections.end());
            }
        }

        void OutputPort::send(std::shared_ptr<Signal::ISignal> signal) {
            _lastSignal = signal;
            for (const auto& input : _connections) {
                if (input->getType() != signal->getType()) {
                    continue; // Skip incompatible signal types
                }
                input->receive(signal);
            }
        }
    }
}