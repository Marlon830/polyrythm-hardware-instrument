#ifndef COMMON_EVENT_MESSAGEEVENT_HPP
#define COMMON_EVENT_MESSAGEEVENT_HPP

#include "common/event/IEvent.hpp"
#include "gui/IGUI.hpp"
#include <string>
#include "common/LogType.hpp"

namespace Common {
    /// @brief Event to carry a simple message string.
    /// @details This event encapsulates a message string that can be used
    /// for logging, notifications, or simple communication between components.

    class MessageEvent : public IEvent {
    public:
        /// @brief Construct a new MessageEvent.
        /// @param message The message string.
        explicit MessageEvent(logType type, const std::string& message)
            : _type(type), _message(message) {}

        /// @brief Destroy the MessageEvent.
        ~MessageEvent() override = default;

        /// @brief Dispatch the event to the appropriate handler.
        /// @note Implementation will depend on the system architecture.
        void dispatch(GUI::IGUI& handler) override {
            // For now, we just print the message to standard output
            handler.handleMessageEvent(_type, _message);
        }

        /// @brief Get the message string.
        /// @return The message string.
        const std::string& getMessage() const {
            return _message;
        }

        logType getType() const {
            return _type;
        }

    private:
        logType _type;
        std::string _message;
    };
} // namespace Common
#endif // COMMON_EVENT_MESSAGEEVENT_HPP