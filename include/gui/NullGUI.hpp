#ifndef GUI_NULLGUI_HPP
#define GUI_NULLGUI_HPP

#include "gui/IGUI.hpp"
#include <iostream>

namespace GUI {

class NullGUI : public IGUI {
public:
    void start() override {
        std::cout << "[NullGUI] Started (headless mode)" << std::endl;
    }

    void stop() override {
        std::cout << "[NullGUI] Stopped" << std::endl;
    }

    void process() override {}

    void handleWaveformEvent(const double* /*samples*/, size_t /*numSamples*/) override {}

    void handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& /*modules*/) override {}

    void handleConnectionListEvent(
        const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& /*connections*/,
        const std::vector<std::string>& /*modules*/) override {}

    void handleMessageEvent(Common::logType type, const std::string& message) override {
        std::cout << "[NullGUI] Message(" << static_cast<int>(type) << "): " << message << std::endl;
    }
};

} // namespace GUI

#endif // GUI_NULLGUI_HPP
