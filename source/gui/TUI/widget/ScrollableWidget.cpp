#include "gui/TUI/widget/ScrollableWidget.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace GUI {
ScrollableWidget::ScrollableWidget() {
    auto container = Container::Vertical({});
    _component = Renderer(container, [this, container] {
        return container->Render() | frame | border
                | size(WIDTH, EQUAL, _width)
                | size(HEIGHT, GREATER_THAN, _height);
    });
}

ftxui::Component ScrollableWidget::component() {
    return _component;
}

void ScrollableWidget::resize(int width, int height) {
    _width = width;
    _height = height;
}

void ScrollableWidget::addChild(std::shared_ptr<IWidget> child) {
    _children.push_back(child);
    // Mount child component into the container so it receives events.
    if (_component) {
        // _component is a Renderer wrapping a Container::Vertical.
        // Retrieve the inner container and Add the child component.
        // We stored Renderer(child_container,...). We can Add via _component->Add(child_component)
        // but only ComponentBase exposes Add. Instead, rebuild the container when children change:
        auto vertical = Container::Vertical({});
        for (auto& c : _children) {
            c->resize(_width - 6, c->height()); // -6 for border
            vertical->Add(c->component());
        }
        _component = Renderer(vertical, [this, vertical] {
            return vertical->Render() | frame | border
                   | size(WIDTH, EQUAL, _width)
                   | size(HEIGHT, GREATER_THAN, _height);
        });
    }
}
} // namespace GUI
