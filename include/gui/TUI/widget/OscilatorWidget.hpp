/*
    * @file OscilatorWidget.hpp
    * @brief Declaration of the OscilatorWidget class for oscillator waveform selection.
    * @ingroup gui_widget
    * Defines the OscilatorWidget class which provides a GUI component for selecting
    * oscillator waveforms in the TinyMB application.
*/

#ifndef GUI_WIDGET_OSCILATORWIDGET_HPP
    #define GUI_WIDGET_OSCILATORWIDGET_HPP

    #include "gui/TUI/widget/IWidget.hpp"
    #include "engine/module/OscModule.hpp"

#include <functional>
#include <string>

namespace GUI {
    /// @brief Widget for selecting oscillator waveforms.
    class OscilatorWidget : public IWidget {
    public:
        /// @brief Construct a new Oscilator Widget
        /// @param moduleId The ID of the associated oscillator module
        /// @param onWaveformChange Callback function when waveform changes
        OscilatorWidget(unsigned int moduleId,
                        std::function<void(unsigned int, Engine::Module::WaveformType)> onWaveformChange);

        /// @brief Get the component for this widget
        ftxui::Component component() override;
        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { _associatedModuleId = id; }
        unsigned int getid() const override { return _associatedModuleId; }

        /// @brief The title of the widget
        std::string title = "Oscillator";
    private:
        // UI component
        ftxui::Component _component;
        ftxui::Component _radio;

        // Callback
        std::function<void(unsigned int, Engine::Module::WaveformType)> _onWaveformChange;

        unsigned int _associatedModuleId;

        // UI state
        std::vector<std::string> _options{ "Sine", "Square", "Triangle", "Sawtooth" };
        int _selectedIndex = 0;
        Engine::Module::WaveformType _waveformType = Engine::Module::SINE;

        // Requested size 
        int _width = 40;
        int _height = 10;
    };
}

#endif // GUI_WIDGET_OSCILATORWIDGET_HPP