#ifndef COMMON_EVENT_EMITCONNECTIONLIST_HPP
#define COMMON_EVENT_EMITCONNECTIONLIST_HPP
#include "common/event/IEvent.hpp"
#include "engine/port/Connection.hpp"
#include "gui/IGUI.hpp"
#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace Common {
    /// @brief Event to emit the list of current connections in the audio graph.
    /// @details This event carries the list of all current connections between
    /// modules in the audio graph. It can be used to inform interested parties
    /// about the current state of connections.
    class EmitConnectionListEvent : public IEvent {

    public:
        /// @brief Construct a new EmitConnectionListEvent.
        /// @param connections The list of current connections.
        EmitConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                const std::vector<std::string>& modules = {})
            : _connections(connections), _modules(modules) {}

        /// @brief Destroy the EmitConnectionListEvent.
        ~EmitConnectionListEvent() override = default;

        /// @brief Dispatch the event to the appropriate handler.
        /// @note Implementation will depend on the system architecture.
        void dispatch(GUI::IGUI& handler) override {
            // print connection list for debugging
            handler.handleConnectionListEvent(_connections, _modules);
        }

        /// @brief Get the list of connections.
        /// @return The list of current connections.
        const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& getConnections() const {
            return _connections;
        }

        /// @brief Get the list of modules.
        /// @return The list of current modules.
        const std::vector<std::string>& getModules() const {
            return _modules;
        }

    private:        
        std::vector<std::shared_ptr<Engine::ConnectionInfo>> _connections;
        std::vector<std::string> _modules;
    };
} // namespace Common
#endif // COMMON_EVENT_EMITCONNECTIONLIST_HPP   