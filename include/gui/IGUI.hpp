#ifndef GUI_IGUI_HPP
#define GUI_IGUI_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "engine/parameter/Param.hpp"
#include "engine/port/Connection.hpp"
#include "engine/module/IModule.hpp"
#include "common/LogType.hpp"
#include "common/event/ModuleEvent.hpp"
namespace GUI {
    /// @brief Interface for GUI components
    /// @ingroup gui
    /// @details This interface defines the basic functionalities that any GUI
    /// component must implement to interact with the rest of the application.
    class IGUI {
    public:
        /// @brief Virtual destructor
        virtual ~IGUI() = default;

        /// @brief Start the GUI
        virtual void start() = 0;

        /// @brief Stop the GUI
        virtual void stop() = 0;

        virtual void process() = 0;

        // TODO: delete when lvgl fully implemented
        /// event handling implementation
        virtual void handleWaveformEvent(const double* samples, size_t numSamples) = 0;
        virtual void handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& modules) = 0;
        virtual void handleConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                                const std::vector<std::string>& modules) = 0;

        virtual void handleMessageEvent(Common::logType type, const std::string& message) = 0;
    };
} // namespace GUI

#endif // GUI_IGUI_HPP