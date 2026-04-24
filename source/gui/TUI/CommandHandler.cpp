#include "gui/TUI/CommandHandler.hpp"

namespace GUI {

    void CommandHandler::registerCommand(const std::string& name, Handler handler) {
        _handlers[name] = handler;
    }

    bool CommandHandler::dispatchCommand(const ParsedCommand& cmd) const {
        auto it = _handlers.find(cmd.name);
        if (it != _handlers.end()) {
            it->second(cmd);
            return true;
        }
        return false;
    }

} // namespace GUI