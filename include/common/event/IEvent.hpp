///@file IEvent.hpp
///@brief Declaration of the IEvent interface for event handling.
///@ingroup common_event

#ifndef COMMON_EVENT_IEVENT_HPP
#define COMMON_EVENT_IEVENT_HPP

/// forward declaration of IGUI to avoid circular dependency
namespace GUI {
    class IGUI;
}

namespace Common {
    /// @brief Interface for events that can be dispatched to an IGUI.
struct IEvent {
    virtual ~IEvent() = default;
    virtual void dispatch(GUI::IGUI&) = 0;
};
}
#endif // COMMON_EVENT_IEVENT_HPP