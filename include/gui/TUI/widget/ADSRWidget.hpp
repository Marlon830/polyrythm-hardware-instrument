/*
@file ADSRWidget.hpp
@brief Declaration of the ADSRWidget class for ADSR envelope control.
@ingroup gui_widget

Defines the ADSRWidget class which provides a user interface for controlling
the Attack, Decay, Sustain, and Release parameters of an ADSR envelope module
*/

#ifndef GUI_WIDGET_ADSRWIDGET_HPP
#define GUI_WIDGET_ADSRWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"

namespace GUI {

    /// @brief Widget for controlling ADSR envelope parameters.
    class ADSRWidget : public IWidget {
    public:
        /// @brief Construct a new ADSRWidget object
        /// @param moduleId The ID of the ADSR module this widget controls
        /// @param onAttackChanged Callback for when the Attack parameter changes
        /// @param onDecayChanged Callback for when the Decay parameter changes
        /// @param onSustainChanged Callback for when the Sustain parameter changes
        /// @param onReleaseChanged Callback for when the Release parameter changes
        ADSRWidget(unsigned int moduleId,
                   std::function<void(unsigned int, float)> onAttackChanged,
                   std::function<void(unsigned int, float)> onDecayChanged,
                   std::function<void(unsigned int, float)> onSustainChanged,
                   std::function<void(unsigned int, float)> onReleaseChanged);

        /// @brief Destroy the ADSRWidget object
         ~ADSRWidget() = default;

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "ADSR Envelope";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { _moduleId = id; }
        unsigned int getid() const override { return _moduleId; }

    private:
        /// @brief Attack, Decay, Sustain, Release parameters
        float _attack = 0.1f;
        float _decay = 0.1f;
        float _sustain = 0.8f;
        float _release = 0.2f;

        /// @brief Callbacks for parameter changes
        std::function<void(unsigned int, float)> _onAttackChanged;
        std::function<void(unsigned int, float)> _onDecayChanged;
        std::function<void(unsigned int, float)> _onSustainChanged;
        std::function<void(unsigned int, float)> _onReleaseChanged;

        /// @brief The ID of the ADSR module this widget controls
        unsigned int _moduleId;

        /// @brief The main component of the widget
        ftxui::Component _component;

        /// @brief The box area for the ADSR graph
        ftxui::Box _graph_box;

        /// @brief Slider components for each ADSR parameter
        ftxui::Component _attackSlider;
        ftxui::Component _decaySlider;
        ftxui::Component _sustainSlider;
        ftxui::Component _releaseSlider;

        /// @brief Canvas for drawing the ADSR envelope
        ftxui::Canvas _adsrCanvas;
        /// @brief Container component
        ftxui::Component _container;

        /// @brief Dimensions of the widget
        int _width = 40;
        int _height = 12;
    };
}
#endif // GUI_WIDGET_ADSRWIDGET_HPP