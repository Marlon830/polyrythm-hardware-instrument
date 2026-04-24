#include "gui/TUI/widget/ArpegiatorWidget.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"

#include "engine/AudioEngine.hpp"
#include "gui/TUI/widget/WidgetFactory.hpp"
#include "common/command/SetParameter.hpp"
#include "gui/TUI/TUI.hpp"


#include <iomanip>
#include <memory>

using namespace ftxui;

namespace GUI {
    ArpegiatorWidget::ArpegiatorWidget(unsigned int moduleId,
                                       std::function<void(unsigned int, float)> onTimeIntervalChanged,
                                       std::function<void(unsigned int, std::vector<int>)> onNoteSequenceChanged)
        : _moduleId(moduleId),
          _onTimeIntervalChanged(onTimeIntervalChanged),
          _onNoteSequenceChanged(onNoteSequenceChanged) {

        SliderOption<float> timeIntervalOption;
        timeIntervalOption.value = &_timeInterval;
        timeIntervalOption.min = 100.0f;
        timeIntervalOption.max = 2000.0f;
        timeIntervalOption.increment = 50.0f;
        timeIntervalOption.on_change = [this]() {
            _onTimeIntervalChanged(_moduleId, _timeInterval);
        };
        _timeIntervalSlider = Slider(timeIntervalOption);

        InputOption noteSequenceOption;
        noteSequenceOption.content = &(_noteSequenceStr);
        noteSequenceOption.placeholder = "Note Sequence (comma separated): ";

        noteSequenceOption.on_change = [this]() {
            std::string sanitized;
            sanitized.reserve(_noteSequenceStr.size());

            bool last_was_comma = false;
            for (char c : _noteSequenceStr) {
                if ((c >= '0' && c <= '9')) {
                    sanitized.push_back(c);
                    last_was_comma = false;
                } else if (c == ',') {
                    if (!last_was_comma) {
                        sanitized.push_back(',');
                        last_was_comma = true;
                    }
                }
            }
            if (sanitized != _noteSequenceStr) {
                _noteSequenceStr = sanitized;
            }
            while (!sanitized.empty() && sanitized.back() == ',') {
                sanitized.pop_back();
            }
            while (!sanitized.empty() && sanitized.front() == ',') {
                sanitized.erase(sanitized.begin());
            }
            std::vector<int> notes;
            size_t start = 0;
            while (start < _noteSequenceStr.size()) {
                size_t end = _noteSequenceStr.find(',', start);
                std::string token = (end == std::string::npos)
                    ? _noteSequenceStr.substr(start)
                    : _noteSequenceStr.substr(start, end - start);

                if (!token.empty()) {
                    try {
                        int value = std::stoi(token);
                        notes.push_back(value);
                    } catch (...) {
                        // Ignore invalid integers
                    }
                }

                if (end == std::string::npos) break;
                start = end + 1;
            }
            _onNoteSequenceChanged(_moduleId, notes);
        };

        _noteSequenceInput = Input(noteSequenceOption);

        _container = Container::Vertical({
            _timeIntervalSlider,
            _noteSequenceInput,
        });

        _component = Renderer(_container, [this] {
            return 
                vbox({
                    text(title) | center,
                    separator(),
                    vbox({
                        [&]{
                          std::ostringstream oss;
                          oss << std::fixed << std::setprecision(1) << _timeInterval;
                          return text("Time Interval: " + oss.str() + " ms");
                        }(),
                        separator(),
                        _timeIntervalSlider->Render(),
                    }) | xflex,
                    separator(),
                    hbox({
                        text("Note Sequence:"),
                        _noteSequenceInput->Render(),
                    }) | xflex,
                }) | border | size(WIDTH, GREATER_THAN, _width) | size(HEIGHT, GREATER_THAN, _height);
        });

    }
    ftxui::Component ArpegiatorWidget::component() {
        return _component;
    }
    void ArpegiatorWidget::resize(int width, int height) {
        _width = width;
        _height = height;
    }

    static AutoRegisterWidget registerArpegiatorWidget{
        "arpegiator",
        [](std::string name, TUI* tui) {
            unsigned int id = WidgetFactory::parse_id(name);
            auto widget = std::make_shared<ArpegiatorWidget>(
                id,
                [tui](unsigned int moduleId, float timeInterval) {
                    tui->pushCommand(std::make_unique<Common::SetParameterCommand<float>>(
                        moduleId, "TInt", timeInterval));
                },
                [tui](unsigned int moduleId, std::vector<int> noteSequence) {
                    tui->pushCommand(std::make_unique<Common::SetParameterCommand<std::vector<int>>>(
                        moduleId, "NoteSeq", noteSequence));
                });
            widget->title = name;
            return widget;
        }
    };
}