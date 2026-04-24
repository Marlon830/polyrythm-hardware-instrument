/*
 * @file IWidget.hpp
* @brief Declaration of the IWidget interface for GUI widgets.
* @ingroup gui_widget
* Defines the IWidget interface which serves as a base for all GUI widgets.
*/
#ifndef GUI_WIDGET_IWIDGET_HPP
#define GUI_WIDGET_IWIDGET_HPP

#include <ftxui/component/component.hpp>

namespace GUI {

    /// @brief Interface for GUI widgets.
    class IWidget {
    public:
        /// @brief Virtual destructor
        virtual ~IWidget() = default;

        /// @brief Get the component for this widget
        virtual ftxui::Component component() = 0;
        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        virtual void resize(int width, int height) = 0;
        /// @brief The width of the widget
        virtual int width() const = 0;
        /// @brief The height of the widget
        virtual int height() const = 0;

        virtual unsigned int getid() const = 0;
        virtual void setid(unsigned int id) = 0;
    };
} // namespace GUI
#endif // GUI_WIDGET_IWIDGET_HPP