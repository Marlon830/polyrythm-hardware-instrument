#include "gui/TUI/widget/WaveFormWidget.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <cmath>
#include <algorithm>
#include <memory>
#include <thread>
#include <ftxui/component/component.hpp>

using namespace ftxui;

namespace GUI {



static Element RenderWaveform(const std::vector<double>& data, int term_width, int term_height) {
    static Box box;
        const int w = std::max(1, box.x_max - box.x_min + 1);
        const int h = std::max(1, box.y_max - box.y_min + 1);
    term_width = w;
    term_height = h;
  auto renderer_plot_1 = Renderer([&] {
    // Account for border (1 cell on each side)
    const int inner_cells_w = std::max(0, term_width - 2);
    const int inner_cells_h = std::max(0, term_height - 2);

    // Canvas uses braille pixels (2x4 per terminal cell)
    const int canvas_w = std::max(2, inner_cells_w * 2);
    const int canvas_h = std::max(4, inner_cells_h * 4);

    Canvas c(canvas_w, canvas_h);

    // Horizontal axis
    c.DrawPointLine(0, canvas_h / 2, canvas_w - 1, canvas_h / 2, Color::GrayDark);

    if (data.empty()) {
      return canvas(std::move(c)) | border;
    }

    const size_t num_samples = data.size();
    const float x_scale = (num_samples > 1)
        ? static_cast<float>(canvas_w - 1) / static_cast<float>(num_samples - 1)
        : 0.0f;

    // Map [-1,1] -> [0, canvas_h-1] (0=top, canvas_h-1=bottom)
    auto map_y = [&](float v) -> int {
      v = std::clamp(v, -1.0f, 1.0f);
      const float y = (1.0f - v) * 0.5f * static_cast<float>(canvas_h - 1);
      return static_cast<int>(std::lround(y));
    };

    for (size_t i = 0; i + 1 < num_samples; ++i) {
      const int x1 = static_cast<int>(std::lround(i * x_scale));
      const int x2 = static_cast<int>(std::lround((i + 1) * x_scale));
      const int y1 = map_y(data[i]);
      const int y2 = map_y(data[i + 1]);
      c.DrawPointLine(x1, y1, x2, y2, Color::White);
    }

    return canvas(std::move(c));
  }) | reflect(box) | border;

    return renderer_plot_1->Render();
}


WaveformWidget::WaveformWidget()
    : _component(Renderer([&] {
        return RenderWaveform(_samples, _width, _height);
    })) {
}

ftxui::Component WaveformWidget::component() {
    return _component;
}

void WaveformWidget::setSamples(const double* samples, size_t numSamples) {
    if (numSamples == 0 || samples == nullptr) {
        return;
    }

    if (numSamples >= _maxSamples) {
        _samples.assign(samples + (numSamples - _maxSamples), samples + numSamples);
        return;
    }

    const size_t size_before = _samples.size();
    const size_t required_size = size_before + numSamples;

    if (required_size <= _maxSamples) {
        _samples.insert(_samples.end(), samples, samples + numSamples);
    } else {
        const size_t to_drop = required_size - _maxSamples; // number of oldest samples to remove
        if (to_drop >= size_before) {
            _samples.assign(samples + (numSamples - (_maxSamples)), samples + numSamples);
        } else {
            _samples.erase(_samples.begin(), _samples.begin() + to_drop);
            _samples.insert(_samples.end(), samples, samples + numSamples);
        }
    }

}

void WaveformWidget::resize(int width, int height) {
    _width = width;
    _height = height;
}
}