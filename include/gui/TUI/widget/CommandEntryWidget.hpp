/*
@file CommandEntryWidget.hpp
@brief Declaration of the CommandEntryWidget class for command input.
@ingroup gui_widget

Defines the CommandEntryWidget class which provides a user interface for entering
commands and displaying suggestions.
*/
#ifndef GUI_WIDGET_COMMANDENTRYWIDGET_HPP
#define GUI_WIDGET_COMMANDENTRYWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"
#include <ftxui/component/component.hpp>
#include <functional>
#include <string>

#include <ftxui/dom/elements.hpp>

namespace GUI {
    /// @brief Widget for entering commands.
    class CommandEntryWidget : public IWidget {
    public:

        /// @brief Construct a new CommandEntryWidget object
        /// @param onCommandEntered Callback for when a command is entered
        CommandEntryWidget(std::function<void(const std::string)> onCommandEntered,
                           std::vector<std::string> commandNames = {});

        /// @brief Destroy the CommandEntryWidget object
         ~CommandEntryWidget() = default;

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief Check if suggestions are visible
        bool suggestionsVisible() const { return !_currentInput.empty(); }

        /// @brief Render the overlay with suggestions
        ftxui::Element renderOverlay() const;

        /// @brief The title of the widget
        std::string title = "Command Entry";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { /* No-op for command entry */ }
        unsigned int getid() const override { return 0; /* No-op for command entry */ }
    private:
        // Core UI
        ftxui::Component _component;
        ftxui::Component _input;

        // Callback
        std::function<void(const std::string&)> _onCommandEntered;

        // Layout
        int _width = 40;
        int _height = 3;

        // State
        std::vector<std::string> _commandHistory;
        size_t _historyIndex = 0;
        std::string _currentInput;
        std::vector<std::string> _commandNames;
        std::vector<std::string> _suggestions;
        int _selectedSuggestion = 0;
        int _cursor = 0;


        // Helpers

        /// @brief Update the list of command suggestions based on the current input
        void updateSuggestions();
        /// @brief Get the current first token from the input
        std::string currentFirstToken() const;
        /// @brief Apply the selected suggestion to the current input
        void applySuggestion();
        /// @brief Calculate a fuzzy matching score between a pattern and a candidate string
        int fuzzyScore(const std::string& pattern, const std::string& candidate) const;
    };
}
#endif // GUI_WIDGET_COMMANDENTRYWIDGET_HPP