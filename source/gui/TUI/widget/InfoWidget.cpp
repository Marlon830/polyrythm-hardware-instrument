#include "gui/TUI/widget/InfoWidget.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace GUI {
    InfoWidget::InfoWidget() {
        _component = Renderer([this] {
            using namespace ftxui;
            std::vector<Element> items;

            items.push_back(text(title) | bold | center);
            items.push_back(separator()); // header separator

            int border_lines  = 2;          // top + bottom Border
            int header_lines  = 2;          // title + separator
            int available     = std::max(0, _height - border_lines - header_lines);

            int max_entries = (available + 1);

            // Sélection des entrées à afficher (les plus récentes en bas)
            int total = static_cast<int>(_entries.size());
            int start_index = std::max(0, total - max_entries);

            std::vector<Element> lines;
            lines.reserve(std::min(max_entries, total) * 2);

            for (int i = start_index; i < total; ++i) {
                const auto& e = _entries[i];
                Element type_badge;
                Decorator type_style = nothing;
                switch (e.type) {
                    case Common::logType::ERROR:
                        type_badge = text("ERROR");
                        type_style = color(Color::Red) | bold;
                        break;
                    case Common::logType::WARNING:
                        type_badge = text("WARN");
                        type_style = color(Color::Yellow);
                        break;
                    case Common::logType::INFO:
                    default:
                        type_badge = text("INFO");
                        type_style = dim;
                        break;
                }

                Element row = hbox({
                    text(e.timestamp) | dim,
                    text("  "),
                    type_badge | type_style,
                    text("  "),
                    paragraph(e.message) | xflex,
                });

                lines.push_back(row);
            }

            Element list = vbox(std::move(lines)) | yflex | xflex;

            return vbox({
                items[0],
                items[1],
                list,
            }) | border | yflex | xflex;
        }) | yflex | xflex;
    }

    ftxui::Component InfoWidget::component() {
        return _component;
    }

    void InfoWidget::setInfoText(Common::logType type, const std::string& infoText) {
        // Append a single log entry with timestamp; keep message as-is
        _entries.push_back(LogEntry{
            .timestamp = nowTimestamp(),
            .message   = infoText,
            .type      = type
        });
    }

    void InfoWidget::resize(int width, int height) {
        _width = width;
        _height = height;
    }

    std::string InfoWidget::nowTimestamp() {
        using namespace std::chrono;
        const auto tp = system_clock::now();
        const auto t  = system_clock::to_time_t(tp);
        const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

        std::tm tm{};
        // Linux: use localtime_r
        localtime_r(&t, &tm);

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min  << ":"
            << std::setw(2) << tm.tm_sec  << "."
            << std::setw(3) << ms.count();
        return oss.str();
    }
} // namespace GUI