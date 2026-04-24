#include "gui/TUI/widget/OscilatorWidget.hpp"
#include "ftxui/component/captured_mouse.hpp"      // for ftxui
#include "ftxui/component/component.hpp"           // for Radiobox
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive

#include "engine/AudioEngine.hpp"
#include "common/command/SetParameter.hpp"
#include "gui/TUI/widget/WidgetFactory.hpp"
#include "gui/TUI/TUI.hpp"

using namespace ftxui;

namespace GUI {


OscilatorWidget::OscilatorWidget(unsigned int moduleId,
                                 std::function<void(unsigned int, Engine::Module::WaveformType)> onWaveformChange)
    : _onWaveformChange(std::move(onWaveformChange)),
      _associatedModuleId(moduleId),
      _selectedIndex(0),
      _waveformType(Engine::Module::SINE) {

    _radio = Radiobox(&_options, &_selectedIndex);

    _component = Renderer(_radio, [&] {
        auto newType = static_cast<Engine::Module::WaveformType>(_selectedIndex);
        if (newType != _waveformType && _onWaveformChange) {
            _waveformType = newType;
            _onWaveformChange(_associatedModuleId, _waveformType);
        }
        return vbox({
                   text(title) | center,
                   separator(),
                   _radio->Render(),
                   separator()
               })
               | size(WIDTH, EQUAL, _width)
               | size(HEIGHT, GREATER_THAN, _height)
               | border;
    });
}

ftxui::Component OscilatorWidget::component() {
    return _component;
}

void OscilatorWidget::resize(int width, int height) {
    _width = width;
    _height = height;
}

static AutoRegisterWidget registerOscilatorWidget{
    "oscillator",
    [](std::string name, TUI* tui) {
        unsigned int id = WidgetFactory::parse_id(name);
        auto widget = std::make_shared<OscilatorWidget>(
            id,
            [tui](unsigned int moduleId, Engine::Module::WaveformType waveformType) {
                tui->pushCommand(std::make_unique<Common::SetParameterCommand<Engine::Module::WaveformType>>(
                    moduleId, "Waveform", waveformType));
            });
        widget->title = name;
        return widget;
    }
};

} // namespace GUI