#include "gui/TUI/widget/CommandEntryWidget.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <iostream>

#include "gui/TUI/widget/WidgetFactory.hpp"


using namespace ftxui;

namespace GUI {
    std::string CommandEntryWidget::currentFirstToken() const {
        auto pos = _currentInput.find(' ');
        return pos == std::string::npos ? _currentInput : _currentInput.substr(0, pos);
    }

    int CommandEntryWidget::fuzzyScore(const std::string& pattern, const std::string& candidate) const {
        if (pattern.empty()) return 0;
        if (candidate.rfind(pattern, 0) == 0) return 100 + int(pattern.size());
        int score = 0;
        size_t j = 0;
        for (char c : candidate) {
            if (j < pattern.size() && std::tolower(c) == std::tolower(pattern[j])) {
                score++;
                j++;
            }
        }
        return score;
    }

    void CommandEntryWidget::updateSuggestions() {
        std::string token = currentFirstToken();
        _suggestions.clear();
        if (token.empty()) {
            _suggestions = _commandNames;
        } else {
            struct Entry { std::string cmd; int score; };
            std::vector<Entry> temp;
            for (auto& cmd : _commandNames) {
                int s = fuzzyScore(token, cmd);
                if (s > 0) temp.push_back({cmd, s});
            }
            std::sort(temp.begin(), temp.end(), [](auto& a, auto& b){
                return a.score > b.score || (a.score == b.score && a.cmd < b.cmd);
            });
            for (auto& e : temp) _suggestions.push_back(e.cmd);
            if (_suggestions.empty()) {
                _suggestions = _commandNames;
            }
        }
        if (_selectedSuggestion >= (int)_suggestions.size())
            _selectedSuggestion = 0;
    }

    void CommandEntryWidget::applySuggestion() {
        if (_suggestions.empty()) return;
        std::string chosen = _suggestions[_selectedSuggestion];
        auto restPos = _currentInput.find(' ');
        std::string rest = (restPos == std::string::npos) ? "" : _currentInput.substr(restPos);
        _currentInput = chosen + (rest.empty() ? " " : rest);
        _cursor = static_cast<int>(_currentInput.size());
    }

    CommandEntryWidget::CommandEntryWidget(std::function<void(const std::string)> onCommandEntered, std::vector<std::string> commandNames)
        : _onCommandEntered(std::move(onCommandEntered)), _commandNames(std::move(commandNames)) {

        InputOption option = InputOption::Default();
        option.on_enter = [this]() {
            if (_onCommandEntered) {
                std::string trimmed = _currentInput;

                // Safe trim: handle npos
                auto last = trimmed.find_last_not_of(" \n\r\t");
                if (last == std::string::npos) {
                    // Only whitespace -> ignore
                    return;
                }
                trimmed.erase(last + 1);
                auto first = trimmed.find_first_not_of(" \n\r\t");
                trimmed.erase(0, first);

                // Execute and store exactly the same string
                _onCommandEntered(trimmed);
                _commandHistory.push_back(trimmed);

                _historyIndex = 0;
                _currentInput.clear();
                _cursor = 0;
                updateSuggestions();
            }
        };
        option.content = &_currentInput;
        option.placeholder = "...";
        option.cursor_position = &_cursor;
        _input = Input(option);

        updateSuggestions();

        auto renderer = Renderer(_input, [this] {
            updateSuggestions();
            Element input_box = vbox({
                filler() | size(HEIGHT, EQUAL, 1),
                hbox({
                    text("> "),
                    _input->Render() | xflex | focus,
                }),
            }) | size(HEIGHT, EQUAL, 3);
            return input_box;
        });

        _component = CatchEvent(renderer, [this](Event e) {
            const bool show = suggestionsVisible();

            if (show && e == Event::Tab) {
                applySuggestion();
                return true;
            }
            if (show && (e == Event::ArrowDownCtrl )) {
                if (!_suggestions.empty())
                    _selectedSuggestion = (_selectedSuggestion + 1) % (int)_suggestions.size();
                return true;
            }
            if (show && (e == Event::ArrowUpCtrl )) {
                if (!_suggestions.empty())
                    _selectedSuggestion = (_selectedSuggestion - 1 + (int)_suggestions.size()) % (int)_suggestions.size();
                return true;
            }
            if (e == Event::ArrowUp) {
                if (_commandHistory.empty()) return false;
                if (_historyIndex < _commandHistory.size()) {
                    _historyIndex++;
                    _currentInput = _commandHistory[_commandHistory.size() - _historyIndex];
                    _cursor = static_cast<int>(_currentInput.size());
                }
                return true;
            }
            if (e == Event::ArrowDown) {
                if (_commandHistory.empty()) return false;
                if (_historyIndex > 1) {
                    _historyIndex--;
                    _currentInput = _commandHistory[_commandHistory.size() - _historyIndex];
                    _cursor = static_cast<int>(_currentInput.size());
                } else {
                    _historyIndex = 0;
                    _currentInput.clear();
                    _cursor = 0;
                }
                return true;
            }
            return false;
        });
    }

    Element CommandEntryWidget::renderOverlay() const {
        if (!suggestionsVisible())
            return emptyElement();

        std::vector<Element> rows;
        for (size_t i = 0; i < _suggestions.size(); ++i) {
            bool sel = (int)i == _selectedSuggestion;
            rows.push_back(text(_suggestions[i]) | (sel ? inverted : nothing));
        }

        Element panel =
            window(text("Suggestions"),
                   vbox(rows) | vscroll_indicator | yframe
            )
            | size(WIDTH, GREATER_THAN, std::max(18, _width / 2))
            | size(HEIGHT, LESS_THAN, std::max(6, _height));

        return vbox({
            hbox({ filler(), panel }),
            filler(),
        });
    }

    ftxui::Component CommandEntryWidget::component() { return _component; }


    void CommandEntryWidget::resize(int width, int height) {
        _width = width;
        _height = height;
    }
}