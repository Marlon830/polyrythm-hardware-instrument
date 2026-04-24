/*
@file WaveFormWidget.hpp
@brief Declaration of the WaveformWidget class for displaying audio waveforms.
@ingroup gui_widget
Defines the WaveformWidget class which provides a user interface for visualizing
audio waveforms in the TinyMB application.
*/
#ifndef GUI_WIDGET_WAVEFORMWIDGET_HPP
#define GUI_WIDGET_WAVEFORMWIDGET_HPP

#include "gui/TUI/widget/IWidget.hpp"


#include <mutex>
#include <vector>

namespace GUI {
    /// @brief Widget for displaying audio waveforms.
    class WaveformWidget : public IWidget {
    public:
        /// @brief Construct a new Waveform Widget
        WaveformWidget();

        /// @brief Get the component for this widget
        ftxui::Component component() override;

        /// @brief Set the audio samples to display
        /// @param samples Pointer to the array of audio samples
        /// @param numSamples Number of samples in the array
        void setSamples(const double* samples, size_t numSamples);

        /// @brief Resize the widget
        /// @param width The new width
        /// @param height The new height
        void resize(int width, int height) override;

        /// @brief The title of the widget
        std::string title = "Waveform";

        /// @brief Get the width of the widget
        int width() const override { return _width; }
        /// @brief Get the height of the widget
        int height() const override { return _height; }
        void setid(unsigned int id) override { /* No-op for waveform widget */ }
        unsigned int getid() const override { return 0; /* No-op for waveform widget */ }
    private:
        ftxui::Component _component;
        std::vector<double> _samples;
        size_t _maxSamples = 2048;

        int _width = 40;
        int _height = 10;
    };
}
#endif // GUI_WIDGET_WAVEFORMWIDGET_HPP