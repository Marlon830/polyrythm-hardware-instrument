#include "gui/TUI/widget/ADSRWidget.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/component/component.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#include "engine/AudioEngine.hpp"
#include "gui/TUI/widget/WidgetFactory.hpp"
#include "gui/TUI/TUI.hpp"

#include "common/command/SetParameter.hpp"

using namespace ftxui;

namespace GUI {

static Element RenderADSRGraph(float attack, float decay, float sustain, float release,
                               Box& graph_box) {
  const int term_width  = std::max(1, graph_box.x_max - graph_box.x_min + 1);
  const int term_height = std::max(1, graph_box.y_max - graph_box.y_min + 1);

  auto renderer = Renderer([&] {
    const int inner_cells_w = std::max(0, term_width - 2);
    const int inner_cells_h = std::max(0, term_height - 2);

    const int canvas_w = std::max(2, inner_cells_w * 2);
    const int canvas_h = std::max(4, inner_cells_h * 4);

    Canvas c(canvas_w, canvas_h);

    c.DrawPointLine(0, canvas_h - 1, canvas_w - 1, canvas_h - 1, Color::GrayDark);

    const float eps = 1e-6f;
    float A = std::max(eps, attack);
    float D = std::max(eps, decay);
    float R = std::max(eps, release);
    float S = std::clamp(sustain, 0.0f, 1.0f);

    const float total_adr = A + D + R;
    int ax = std::max(2, (int)std::round((A / total_adr) * canvas_w));
    int dx = std::max(2, (int)std::round((D / total_adr) * canvas_w));
    int rx = std::max(2, (int)std::round((R / total_adr) * canvas_w));
    if (ax + dx + rx > canvas_w) {
      int overflow = ax + dx + rx - canvas_w;
      rx = std::max(2, rx - overflow);
    }

    int x0 = 0;
    int x1 = std::min(canvas_w - 1, x0 + ax);
    int x2 = std::min(canvas_w - 1, x1 + dx);
    int x3 = canvas_w - 1;
    int xs = x3 - rx;

    auto map_y = [&](float v) -> int {
      v = std::clamp(v, 0.0f, 1.0f);
      return (int)std::lround((1.0f - v) * (canvas_h - 1));
    };

    auto attack_y = [&](int x) {
      float t = ax > 0 ? float(x - x0) / float(std::max(1, ax)) : 1.0f;
      float v = 1.0f - std::pow(1.0f - std::clamp(t, 0.0f, 1.0f), 3.0f);
      return map_y(v);
    };
    auto decay_y = [&](int x) {
      float t = dx > 0 ? float(x - x1) / float(std::max(1, dx)) : 1.0f;
      const float k = 4.0f;
      float v = S + (1.0f - S) * std::exp(-k * std::clamp(t, 0.0f, 1.0f));
      return map_y(v);
    };
    int sustain_y = map_y(S);
    auto release_y = [&](int x) {
      float t = rx > 0 ? float(x - xs) / float(std::max(1, rx)) : 1.0f;
      const float k = 4.0f;
      float v = S * std::exp(-k * std::clamp(t, 0.0f, 1.0f));
      return map_y(v);
    };

    for (int x = x0; x <= x1; ++x) c.DrawPoint(x, attack_y(x),  true, Color::Cyan);
    for (int x = x1; x <= x2; ++x) c.DrawPoint(x, decay_y(x),   true, Color::Cyan);
    for (int x = x2; x <= xs; ++x) c.DrawPoint(x, sustain_y,    true, Color::Cyan);
    for (int x = xs; x <= x3; ++x) c.DrawPoint(x, release_y(x), true, Color::Cyan);

    auto draw_vline = [&](int x, Color col) { c.DrawPointLine(x, 0, x, canvas_h - 1, col); };
    draw_vline(x1, Color::GrayDark);
    draw_vline(x2, Color::GrayDark);
    draw_vline(xs, Color::GrayDark);

    return canvas(std::move(c)) | reflect(graph_box);
  });

  return renderer->Render();
}
ADSRWidget::ADSRWidget(unsigned int moduleId,
                       std::function<void(unsigned int, float)> onAttackChanged,
                       std::function<void(unsigned int, float)> onDecayChanged,
                       std::function<void(unsigned int, float)> onSustainChanged,
                       std::function<void(unsigned int, float)> onReleaseChanged)
    : _moduleId(moduleId),
      _onAttackChanged(std::move(onAttackChanged)),
      _onDecayChanged(std::move(onDecayChanged)),
      _onSustainChanged(std::move(onSustainChanged)),
      _onReleaseChanged(std::move(onReleaseChanged)) {

  SliderOption<float> aopt;
  aopt.value = &_attack; aopt.min = 0.0f; aopt.max = 5.0f; aopt.increment = 0.05f;
  aopt.on_change = [this]{ if (_onAttackChanged) _onAttackChanged(_moduleId, _attack); };
  _attackSlider = Slider(aopt);

  SliderOption<float> dopt;
  dopt.value = &_decay; dopt.min = 0.0f; dopt.max = 5.0f; dopt.increment = 0.05f;
  dopt.on_change = [this]{ if (_onDecayChanged) _onDecayChanged(_moduleId, _decay); };
  _decaySlider = Slider(dopt);
        
  SliderOption<float> sopt;
  sopt.value = &_sustain; sopt.min = 0.0f; sopt.max = 1.0f; sopt.increment = 0.02f;
  sopt.on_change = [this]{ if (_onSustainChanged) _onSustainChanged(_moduleId, _sustain); };
  _sustainSlider = Slider(sopt);

  SliderOption<float> ropt;
  ropt.value = &_release; ropt.min = 0.0f; ropt.max = 5.0f; ropt.increment = 0.05f;
  ropt.on_change = [this]{ if (_onReleaseChanged) _onReleaseChanged(_moduleId, _release); };
  _releaseSlider = Slider(ropt);

  auto slider_row = [](const std::string& label, Component slider, std::function<std::string()> value_text) {
    return hbox({
      text(label) | bold | size(WIDTH, EQUAL, 10),
      separator(),
      slider->Render() | xflex,
      separator(),
      text(value_text()) | size(WIDTH, EQUAL, 6) | dim,
    }) | size(HEIGHT, EQUAL, 1);
  };

  _graph_box = Box{};

  _container = Container::Vertical({
    Renderer([this] {
      return RenderADSRGraph(_attack, _decay, _sustain, _release, _graph_box) | yflex;
    }),
    Renderer(_attackSlider, [this, slider_row] {
      std::ostringstream oss; oss << std::fixed << std::setprecision(2) << _attack << " s";
      return slider_row("A (s):", _attackSlider, [s=oss.str()]{ return s; });
    }),
    Renderer(_decaySlider, [this, slider_row] {
      std::ostringstream oss; oss << std::fixed << std::setprecision(2) << _decay << " s";
      return slider_row("D (s):", _decaySlider, [s=oss.str()]{ return s; });
    }),
    Renderer(_sustainSlider, [this, slider_row] {
      std::ostringstream oss; oss << std::fixed << std::setprecision(2) << _sustain;
      return slider_row("S (0-1):", _sustainSlider, [s=oss.str()]{ return s; });
    }),
    Renderer(_releaseSlider, [this, slider_row] {
      std::ostringstream oss; oss << std::fixed << std::setprecision(2) << _release << " s";
      return slider_row("R (s):", _releaseSlider, [s=oss.str()]{ return s; });
    }),
  });

  _component = Renderer(_container, [this] {
  return vbox({
           text(title) | center,
           separator(),
           _container->ChildAt(0)->Render() | yflex,
           separator(),                 // graph (no border)
           _container->ChildAt(1)->Render() | size(HEIGHT, EQUAL, 1),// A
           _container->ChildAt(2)->Render() | size(HEIGHT, EQUAL, 1),// D
           _container->ChildAt(3)->Render() | size(HEIGHT, EQUAL, 1),// S
           _container->ChildAt(4)->Render() | size(HEIGHT, EQUAL, 1),// R
         }) | size(WIDTH, EQUAL, _width)
           | size(HEIGHT, GREATER_THAN, _height) // ensures room for title
           | border;
  });
}

ftxui::Component ADSRWidget::component() {
  return _component;
}

void ADSRWidget::resize(int width, int height) {
  _width = width;
  _height = height;
}

static AutoRegisterWidget adsrWidgetReg {
    "adsr",
    [](std::string name, TUI* tui) {
    unsigned id = WidgetFactory::parse_id(name);
    auto widget = std::make_shared<ADSRWidget>(id,
        [tui](unsigned int id, float attack) {
            tui->pushCommand(std::make_unique<Common::SetParameterCommand<float>>(
                id, "Attack", attack));
        },
        [tui](unsigned int id, float decay) {
            tui->pushCommand(std::make_unique<Common::SetParameterCommand<float>>(
                id, "Decay", decay));
        },
        [tui](unsigned int id, float sustain) {
            tui->pushCommand(std::make_unique<Common::SetParameterCommand<float>>(
                id, "Sustain", sustain));
        },
        [tui](unsigned int id, float release) {
            tui->pushCommand(std::make_unique<Common::SetParameterCommand<float>>(
                id, "Release", release));
        });
        widget->title = name;
        return widget;
    }
};

} // namespace GUI