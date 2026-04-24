/*
@file InfoWidget.hpp
@brief Declaration of the InfoWidget class for info/log display.
@ingroup gui_widget
*/

#ifndef GUI_WIDGET_INFOWIDGET_HPP
#define GUI_WIDGET_INFOWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/screen/box.hpp>
#include "common/event/MessageEvent.hpp"
#include <string>
#include <vector>

namespace GUI {
    /// @brief Widget for displaying info and log messages.
    class InfoWidget : public IWidget {
    public:
        /// @brief Construct a new InfoWidget object
        InfoWidget();

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Add a log entry to the widget
        /// @param type The type of the log message
        /// @param infoText The log message text
        void setInfoText(Common::logType type, const std::string& infoText);

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "Log";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { /* No-op for info widget */ }
        unsigned int getid() const override { return 0; /* No-op for info widget */ }
    private:
        ftxui::Component _component;
        struct LogEntry {
            std::string timestamp;   // e.g. "12:34:56.789"
            std::string message;     // raw message
            Common::logType type;
        };
        std::vector<LogEntry> _entries;

        // layout
        int _width = 40;
        int _height = 10;

        /// @brief Get the current timestamp as a string
        static std::string nowTimestamp();
    };
}

#endif // GUI_WIDGET_INFOWIDGET_HPP