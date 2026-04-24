#ifndef ENGINE_CORE_IEVENTEMITTER_HPP
#define ENGINE_CORE_IEVENTEMITTER_HPP


#include "common/event/IEvent.hpp"
#include <memory>

namespace Engine::Core {
    class IEventEmitter {
    public:
        virtual ~IEventEmitter() = default;
        virtual void emit(std::unique_ptr<Common::IEvent> ev) = 0;
    };
} // namespace Engine::Core
#endif