#include "gui/TUI/widget/FilterWidget.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "ftxui/component/captured_mouse.hpp"     
#include "ftxui/component/component.hpp"          
#include "ftxui/component/screen_interactive.hpp"
#include <sstream>
#include <iomanip>

#include "engine/AudioEngine.hpp"
#include "common/command/SetParameter.hpp"
#include "gui/TUI/widget/WidgetFactory.hpp"
#include "gui/TUI/TUI.hpp"

using namespace ftxui;

//TODO: add filter wave visualization on a canvas
namespace GUI {

    FilterWidget::FilterWidget(unsigned int moduleId,std::function<void(unsigned int, Engine::Module::FilterType)> onFilterTypeChanged,
                               std::function<void(unsigned int, double)> onCutoffChanged,
                               std::function<void(unsigned int, double)> onResonanceChanged)
        : _moduleId(moduleId),
          _selectedIndex(0),
          _filterType(Engine::Module::LOW_PASS),
          _cutoffFrequency(1000.0),
          _resonance(1.0),
          _onFilterTypeChanged(std::move(onFilterTypeChanged)),
          _onCutoffChanged(std::move(onCutoffChanged)),
          _onResonanceChanged(std::move(onResonanceChanged)) {

        _radio = Radiobox(&_options, &_selectedIndex);

        SliderOption<double> cutoffOption;
            cutoffOption.value = &_cutoffFrequency;
            cutoffOption.min = 20.0;
            cutoffOption.max = 20000.0;
            cutoffOption.increment = 10.0;
            cutoffOption.direction = Direction::Right;
            cutoffOption.on_change = [this]() {
                _onCutoffChanged(this->_moduleId, this->_cutoffFrequency);
            };
        _cutoffSlider = Slider(cutoffOption);

        SliderOption<double> resonanceOption;
            resonanceOption.value = & _resonance;
            resonanceOption.min = 0.1;
            resonanceOption.max = 10.0f;
            resonanceOption.increment = 0.1f;
            resonanceOption.direction = Direction::Right;
            resonanceOption.on_change = [this]() {
                _onResonanceChanged(this->_moduleId, this->_resonance);
            };
        _resonanceSlider = Slider(resonanceOption);

        _container = Container::Vertical({
            _radio,
            _cutoffSlider,
            _resonanceSlider
        });

        _component = Renderer(_container, [this] {

            auto newType = static_cast<Engine::Module::FilterType>(_selectedIndex);
            if (newType != _filterType && _onFilterTypeChanged) {
                _filterType = newType;
                _onFilterTypeChanged(this->_moduleId, _filterType);
            }

            return vbox({
                        text(title) | center,
                        separator(),
                        _radio->Render(),
                        separator(),
                        vbox({
                            hbox({
                                [&]{
                                  std::ostringstream oss;
                                  oss << std::fixed << std::setprecision(1) << _cutoffFrequency;
                                  return text("Cutoff: " + oss.str() + " Hz");
                                }(),
                                separator(),
                                _cutoffSlider->Render(),
                            }) | xflex,
                            separator(),
                            hbox({
                                [&]{
                                  std::ostringstream oss;
                                  oss << std::fixed << std::setprecision(1) << _resonance;
                                  return text("Resonance: " + oss.str());
                                }(),
                                separator(),
                                _resonanceSlider->Render(),
                            }) | xflex,
                            separator(),

                        }) | xflex,
                   })
                   | size(WIDTH, EQUAL, _width)
                   | size(HEIGHT, GREATER_THAN, _height)
                   | border;
        });
    } 

    ftxui::Component FilterWidget::component() {
        return _component;
    }

    void FilterWidget::resize(int width, int height) {
        _width = width;
        _height = height;
    }

    static AutoRegisterWidget filterWidgetReg {
        "filter",
        [](std::string name, TUI* tui) {
            unsigned int id =  WidgetFactory::parse_id(name);
            auto widget = std::make_shared<FilterWidget>(
                id,
                [tui](unsigned int moduleId, Engine::Module::FilterType filterType) {
                    tui->pushCommand(std::make_unique<Common::SetParameterCommand<Engine::Module::FilterType>>(
                        moduleId, "FilterType", filterType));
                },
                [tui](unsigned int moduleId, double cutoff) {
                    tui->pushCommand(std::make_unique<Common::SetParameterCommand<double>>(
                        moduleId, "CutoffFrequency", cutoff));
                },
                [tui](unsigned int moduleId, double resonance) {
                    tui->pushCommand(std::make_unique<Common::SetParameterCommand<double>>(
                        moduleId, "Resonance", resonance));
                });
            widget->title = name;
            return widget;
        }
    };

} // namespace GUI