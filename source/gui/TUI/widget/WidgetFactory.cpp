#include "gui/TUI/widget/WidgetFactory.hpp"

namespace GUI {

    namespace {
        std::unordered_map<std::string, Creator>& registryStorage() {
            static std::unordered_map<std::string, Creator> map;
            return map;
        }
    }


    WidgetFactory& WidgetFactory::instance() {
        static WidgetFactory factory;
        return factory;
    }

    std::unordered_map<std::string, Creator>& WidgetFactory::registry() {
        return registryStorage();
    }

    bool WidgetFactory::registerModule(std::string key, Creator fn) {
        return registryStorage().emplace(std::move(key), std::move(fn)).second;
    }

    void WidgetFactory::unregisterModule(std::string_view key) {
        registryStorage().erase(std::string{key});
    }

    std::shared_ptr<IWidget> WidgetFactory::create(std::string_view name, TUI* tui) const {
        if (auto it = registryStorage().find(parse_name(std::string{name})); it != registryStorage().end()) {
            return it->second(std::string{name}, tui);
        }
        throw std::runtime_error("WidgetFactory::create - unknown widget: " + std::string{name});
    }

    AutoRegisterWidget::AutoRegisterWidget(std::string key, Creator fn) {
        WidgetFactory::registerModule(std::move(key), std::move(fn));
    }

    unsigned WidgetFactory::parse_id(const std::string& name) {
        const auto pos = name.find_last_of('_');
        if (pos == std::string::npos || pos + 1 >= name.size()) return 0u;
        const std::string suffix = name.substr(pos + 1);
        if (!std::all_of(suffix.begin(), suffix.end(), [](unsigned char c){ return std::isdigit(c); }))
            return 0u;
        try { return static_cast<unsigned>(std::stoul(suffix)); }
        catch (...) { return 0u; }
    }

    std::string WidgetFactory::parse_name(const std::string& name) {
        const auto pos = name.find_last_of('_');
        if (pos == std::string::npos) return name;
        return name.substr(0, pos);
    }
} // namespace GUI