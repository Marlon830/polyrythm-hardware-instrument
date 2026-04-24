#include "engine/port/Connection.hpp"

namespace Engine
{
    namespace Port
    {
        Connection::Connection(std::shared_ptr<OutputPort> source, std::shared_ptr<InputPort> target)
            : _source(source), _target(target) {}

        std::shared_ptr<OutputPort> Connection::getSource() const
        {
            return this->_source;
        }

        std::shared_ptr<InputPort> Connection::getTarget() const
        {
            return this->_target;
        }
    }
}
