#include "engine/port/Port.hpp"


namespace Engine {
    namespace Port {
        Port::Port(const std::string& name, Signal::SignalType type)
            : _name(name), _type(type) {}

        std::string Port::getName() const {
            return _name;
        }

        Signal::SignalType Port::getType() const {
            return _type;
        }
    }
}
