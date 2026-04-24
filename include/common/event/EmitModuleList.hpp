#ifndef COMMON_EVENT_EMIT_MODULE_LIST_HPP
#define COMMON_EVENT_EMIT_MODULE_LIST_HPP
#include <gui/IGUI.hpp> 
#include "engine/parameter/Param.hpp"
#include "common/event/IEvent.hpp"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <iostream>

namespace Common {
    /// @brief Event to emit the current list of modules and their parameters.
    /// @ingroup common_event


    struct EmitModuleListEvent : public IEvent {
        /// @brief Construct a new EmitModuleListEvent
        /// @param modules The map of module names to their parameters.
        EmitModuleListEvent(const std::map<std::string, ModuleInfo>& modules)
            : _modules(modules) {}

        /// @brief Destroy the EmitModuleListEvent
        ~EmitModuleListEvent() override = default;

        /// @brief Dispatch the event to the given IGUI.
        /// @param gui The IGUI to dispatch the event to.
        void dispatch(GUI::IGUI& gui) override {
            gui.handleModuleListEvent(_modules);
        }

        std::map<std::string, ModuleInfo> _modules;
    };
} // namespace Common

#endif // COMMON_EVENT_EMIT_MODULE_LIST_HPP