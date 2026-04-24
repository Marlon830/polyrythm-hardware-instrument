/*
@file FilterWidget.hpp
@brief Declaration of the FilterWidget class for filter control.
@ingroup gui_widget

Defines the FilterWidget class which provides a user interface for controlling
the parameters of a filter module.
*/


#ifndef GUI_WIDGET_FILTERWIDGET_HPP
#define GUI_WIDGET_FILTERWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"
#include "engine/module/FilterModule.hpp"
#include <functional> 

namespace GUI {
    /// @brief Widget for controlling filter parameters.
    class FilterWidget : public IWidget {
    public:
        /// @brief Construct a new FilterWidget object
        /// @param moduleId The ID of the filter module this widget controls
        /// @param onFilterTypeChanged Callback for when the filter type parameter changes
        /// @param onCutoffChanged Callback for when the cutoff frequency parameter changes
        /// @param onResonanceChanged Callback for when the resonance parameter changes
        FilterWidget(unsigned int moduleId,std::function<void(unsigned int, Engine::Module::FilterType)> onFilterTypeChanged,
                     std::function<void(unsigned int, double)> onCutoffChanged,
                     std::function<void(unsigned int, double)> onResonanceChanged);

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "Filter";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { _moduleId = id; }
        unsigned int getid() const override { return _moduleId; }
    private:
        ftxui::Component _component;

        ftxui::Component _radio;
        ftxui::Component _cutoffSlider;
        ftxui::Component _resonanceSlider;
        ftxui::Component _container;
        
        
        int _width = 40;
        int _height = 10;

        std::vector<std::string> _options{ "Low Pass", "High Pass", "Band Pass"};
        int _selectedIndex = 0;

        Engine::Module::FilterType _filterType = Engine::Module::LOW_PASS;

        double _cutoffFrequency = 1000.0;
        double _resonance = 1.0;

        std::function<void(unsigned int, Engine::Module::FilterType)> _onFilterTypeChanged;
        std::function<void(unsigned int, double)> _onCutoffChanged;
        std::function<void(unsigned int, double)> _onResonanceChanged;
        unsigned int _moduleId;

    };
}
#endif // GUI_WIDGET_FILTERWIDGET_HPP 