/*
@file ConnectionWidget.hpp
@brief Declaration of the ConnectionWidget class for displaying module connections.
@ingroup gui_widget
Defines the ConnectionWidget class which provides a user interface for displaying
and managing connections between modules.
*/
#ifndef GUI_WIDGET_CONNECTIONWIDGET_HPP
#define GUI_WIDGET_CONNECTIONWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"

#include "engine/port/Connection.hpp"
#include "engine/module/IModule.hpp"

#include <vector>
#include <string>

namespace GUI {
    /// @brief Widget for displaying module connections.
    class ConnectionWidget : public IWidget {
    public:
        /// @brief Construct a new ConnectionWidget object
        ConnectionWidget();

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Set the connections to display
        /// @param connections The list of connections
        void setConnections(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections);

        /// @brief Set the modules to display
        /// @param modules The list of module names
        void setModules(const std::vector<std::string>& modules);

        /// @brief Get the list of module names
        /// @return The list of module names
        std::vector<std::string> getModuleNames() const { return _moduleNames; }

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "Connections";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }
        void setid(unsigned int id) override { /* No-op for connection widget */ }
        unsigned int getid() const override { return 0; /* No-op for connection widget */ }
    private:
        ftxui::Component _component;
        std::vector<std::shared_ptr<Engine::ConnectionInfo>> _connections;
        // UI state
        ftxui::Component _menu;
        ftxui::Component _container;
        std::vector<std::string> _moduleNames;
        int _selectedModuleIndex = 0;

        // layout
        int _width = 40;
        int _height = 10;

        // helpers
        /// @brief Rebuild the module list UI
        void rebuildModuleList();
        /// @brief Get connections for the selected module
        /// @return A list of connections for the selected module
        std::vector<std::shared_ptr<Engine::ConnectionInfo>> connectionsForSelectedModule() const;
    };
}
#endif // GUI_WIDGET_CONNECTIONWIDGET_HPP