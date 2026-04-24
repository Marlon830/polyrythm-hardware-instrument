#include "gui/TUI/widget/ConnectionWidget.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/table.hpp>

#include "engine/module/IModule.hpp"

#include <set>
#include <iostream>
#include <map>

using namespace ftxui;

namespace GUI {
ConnectionWidget::ConnectionWidget() {

    // Menu for modules list
    _menu = ftxui::Menu(&_moduleNames, &_selectedModuleIndex);

    // Container with left menu + right details
    _container = Container::Horizontal({
        _menu,
    });

    _component = Renderer(_container, [this] {
        rebuildModuleList();

        Element left = vbox({
            text("Modules") | center,
            separator(),
            _menu->Render() | xflex,
        });

        // Connexions filtrées pour le module sélectionné
        auto selected_conns = connectionsForSelectedModule();

        const std::string sel = _moduleNames.empty() ? "" : _moduleNames[_selectedModuleIndex];

        // Regrouper par port (entrée/sortie) pour le module sélectionné
        // inputs_map: key = port d'entrée du module sélectionné
        // outputs_map: key = port de sortie du module sélectionné
        struct PortInfo {
            std::string type;                // "Audio", "Event", "Control"
            std::vector<std::string> links;
            std::string port; // "Module:Port" connected
        };
        std::map<std::string, PortInfo> inputs_map;
        std::map<std::string, PortInfo> outputs_map;

        for (const auto& c : selected_conns) {
            if (c->destModule == sel) {
                auto& info = inputs_map[c->destPort];
                if (info.type.empty()) info.type = c->connectionType;
                info.links.push_back(c->sourceModule );
                info.port = c->sourcePort;
                
            } else if (c->sourceModule == sel) {
                auto& info = outputs_map[c->sourcePort];
                if (info.type.empty()) info.type = c->connectionType;
                info.links.push_back(c->destModule );
                info.port = c->destPort;
            }
        }

        // Construction des tableaux
        auto build_table = [](const std::string& header_port,
                              const std::string& header_type,
                              const std::string& header_conn,
                              const std::string& header_conn_port,
                              const std::map<std::string, PortInfo>& ports) {
            std::vector<std::vector<Element>> rows;
            rows.push_back({
                text(header_port) | bold,
                text(header_type) | bold,
                text(header_conn) | bold,
                text(header_conn_port) | bold,
            });

            if (ports.empty()) {
                rows.push_back({
                    text("(empty)"),
                    text(""),
                    text("none"),
                    text(""),
                });
            } else {
                for (const auto& [port_name, info] : ports) {
                    std::string linked;
                    if (info.links.empty()) {
                        linked = "none";
                    } else {
                        // Join multiple connections if there are any (separator " | ")
                        for (size_t i = 0; i < info.links.size(); ++i) {
                            if (i) linked += " | ";
                            linked += info.links[i];
                        }
                    }
                    rows.push_back({
                        text(port_name),
                        text(info.type.empty() ? "-" : info.type),
                        text(linked),
                        text(info.port.empty() ? "-" : info.port),
                    });
                }
            }

            auto table = Table(rows);
            table.SelectAll().Border(LIGHT);
            table.SelectRow(0).Decorate(bold);
            table.SelectRow(0).SeparatorVertical(LIGHT);
            table.SelectRow(0).SeparatorHorizontal(LIGHT);

            table.SelectRows(0, rows.size() - 1).SeparatorVertical(LIGHT);
            table.SelectRows(0, rows.size() - 1).SeparatorHorizontal(LIGHT);

            table.SelectColumn(1).DecorateCells(center);
            table.SelectColumn(3).DecorateCells(center);
            return table.Render();
        };

        auto inputs_table = build_table("Entry Port", "Type", "Connected From", "Port", inputs_map);
        auto outputs_table = build_table("Output Port", "Type", "Connected To", "Port", outputs_map);

        Element right = vbox({
            text(title) | center,
            separator(),
            text("Entry Ports of " + sel) | dim,
            inputs_table,
            separator(),
            text("Output Ports of " + sel) | dim,
            outputs_table,
        });

        Element content = hbox({
            left,
            separator(),
            right | xflex,
        }) | border;

        return content
             | yflex
             | xflex
             | vscroll_indicator
             | yframe;
    });
}

ftxui::Component ConnectionWidget::component() {
    return _component;
}

void ConnectionWidget::setConnections(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections) {
    _connections = connections;
}

void ConnectionWidget::setModules(const std::vector<std::string>& modules) {
    _moduleNames.clear();
    for (const auto& m : modules) {
        _moduleNames.push_back(m);
    }
}

void ConnectionWidget::resize(int width, int height) {
    _width = width;
    _height = height;
}


void ConnectionWidget::rebuildModuleList() {
    // Collect unique module names from both source and dest
    std::set<std::string> unique;
    for (const auto& c : _connections) {
        if (!c->sourceModule.empty()) unique.insert(c->sourceModule);
        if (!c->destModule.empty())   unique.insert(c->destModule);
    }
    // keep the module set by setModules
    if (_moduleNames.empty()) {
        _moduleNames.push_back("No modules");
        _selectedModuleIndex = 0;
    }
}

std::vector<std::shared_ptr<Engine::ConnectionInfo>>
ConnectionWidget::connectionsForSelectedModule() const {
    if (_moduleNames.empty()) return {};
    const std::string& sel = _moduleNames[_selectedModuleIndex];
    std::vector<std::shared_ptr<Engine::ConnectionInfo>> filtered;
    filtered.reserve(_connections.size());
    for (const auto& c : _connections) {
        if (c->sourceModule == sel || c->destModule == sel) {
            filtered.push_back(c);
        }
    }
    return filtered;
}
}