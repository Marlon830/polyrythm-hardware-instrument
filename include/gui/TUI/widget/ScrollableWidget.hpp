/*
@file ScrollableWidget.hpp
@brief Declaration of the ScrollableWidget class for scrollable container.
@ingroup gui_widget
*/
#ifndef SCROLLABLE_WIDGET_HPP
#define SCROLLABLE_WIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"
#include <ftxui/component/component.hpp>
#include <vector>  

namespace GUI {
    /// @brief Widget that provides a scrollable container for other widgets.
    class ScrollableWidget : public IWidget {
    public:
        /// @brief Construct a new Scrollable Widget
        ScrollableWidget();
        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief Add a child widget to the scrollable container
        /// @param child The child widget to add
        void addChild(std::shared_ptr<IWidget> child);

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }
        void setid(unsigned int id) override { /* No-op for scrollable widget */ }
        unsigned int getid() const override { return 0; /* No-op for scrollable widget */ }
    private:
        ftxui::Component _component;
        std::vector<std::shared_ptr<IWidget>> _children;
        int _width = 40;
        int _height = 10;
    };
}

#endif // SCROLLABLE_WIDGET_HPP

