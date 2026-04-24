#include "engine/signal/ControlSignal.hpp"

namespace Engine {
    namespace Signal {

        SignalType ControlSignal::getType() const {
            return _type;
        }

        const std::vector<double>& ControlSignal::getControlValues() const {
            return _controlValues;
        }

    } // namespace Signal
} // namespace Engine