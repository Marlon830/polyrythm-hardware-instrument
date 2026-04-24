/*
@file       ArpegiatorWidget.hpp
@brief      Declaration of the ArpegiatorWidget class for arpeggiator control.
@ingroup    gui_widget

Defines the ArpegiatorWidget class which provides a user interface for controlling
the parameters of an arpeggiator module.
*/
#ifndef GUI_WIDGET_ARPEGIATORWIDGET_HPP
#define GUI_WIDGET_ARPEGIATORWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"

namespace GUI {
    /// @brief Widget for controlling arpeggiator parameters.
    class ArpegiatorWidget : public IWidget {
    public:
        /// @brief Construct a new ArpegiatorWidget object
        /// @param moduleId The ID of the arpeggiator module this widget controls
        /// @param onTimeIntervalChanged Callback for when the time interval parameter changes
        /// @param onNoteSequenceChanged Callback for when the note sequence parameter changes
        ArpegiatorWidget(unsigned int moduleId,
                         std::function<void(unsigned int, float)> onTimeIntervalChanged,
                         std::function<void(unsigned int, std::vector<int>)> onNoteSequenceChanged);

        /// @brief Destroy the ArpegiatorWidget object
         ~ArpegiatorWidget() = default;

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "Arpegiator";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }

        void setid(unsigned int id) override { _moduleId = id; }
        unsigned int getid() const override { return _moduleId; }
    private:
        /// @brief The main component of the widget
        ftxui::Component _component;

        /// @brief Slider component for the time interval parameter
        ftxui::Component _timeIntervalSlider;
        /// @brief Input component for the note sequence parameter
        ftxui::Component _noteSequenceInput;
        /// @brief Container component
        ftxui::Component _container;

        /// @brief Dimensions of the widget
        int _width = 40;
        int _height = 10;

        /// @brief Arpeggiator parameters
        float _timeInterval = 500.0f;
        std::string _noteSequenceStr = "60,64,67";

        /// @brief The ID of the arpeggiator module this widget controls
        unsigned int _moduleId;

        /// @brief Callbacks for parameter changes
        std::function<void(unsigned int, float)> _onTimeIntervalChanged;
        std::function<void(unsigned int, std::vector<int>)> _onNoteSequenceChanged;
    };
}

#endif // GUI_WIDGET_ARPEGIATORWIDGET_HPP
