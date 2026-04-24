/*
@file LVGL.hpp
@brief Declaration of the LVGL class for LVGL-based user interface.
@ingroup gui

LVGL based implementation of the IGUI interface using the LVGL library.
*/

#ifndef GUI_LVGL_HPP
#define GUI_LVGL_HPP

#include "gui/IGUI.hpp"
#include "lvgl/lvgl.h"
#include "ipc/SharedMemory/SharedMemory.hpp"

namespace GUI {
    /// @brief LVGL based implementation of the IGUI interface.
    /// @details This class provides a GUI implementation using the LVGL library.
    /// It depends on producer-consumer queues for command and event handling.
    class LVGL : public IGUI {
    public:
        /// @brief Constructs the LVGL GUI.
        LVGL();

        /// @brief Starts the LVGL GUI event loop.
        void start() override;

        /// @brief Stops the LVGL GUI event loop.
        void stop() override;

        /// @brief Process GUI events
        void process() override;

        /// @brief Handle waveform event
        void handleWaveformEvent(const double* samples, size_t numSamples) override;

        /// @brief Handle module list event
        void handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& modules) override;

        /// @brief Handle connection list event
        void handleConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                      const std::vector<std::string>& modules) override;

        /// @brief Handle message event
        void handleMessageEvent(Common::logType type, const std::string& message) override;
    };
}

#endif // GUI_LVGL_HPP