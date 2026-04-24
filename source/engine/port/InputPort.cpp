#include "engine/port/InputPort.hpp"
#include "engine/signal/ISignal.hpp"
#include "engine/signal/AudioSignal.hpp"
#include <functional>
#include <memory>
#include <iostream>

namespace Engine {
    namespace Port {
        InputPort::InputPort(const std::string& name, Signal::SignalType type)
            : Port(name, type) {}

        void InputPort::receive(std::shared_ptr<Signal::ISignal> signal) {
            _receivedSignal.push_back(signal);
        }

        std::shared_ptr<Signal::ISignal> InputPort::get() const {
            if (_receivedSignal.empty()) {
                return nullptr;
            }
            std::shared_ptr<Signal::ISignal> s = _receivedSignal.back();
            const_cast<InputPort*>(this)->_receivedSignal.pop_back();
            return s;
        }


    }
}