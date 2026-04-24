#include "gui/TUI/TUI.hpp"
#include "common/Producer.hpp"
#include "common/Consumer.hpp"
#include "common/command/ICommand.hpp"
#include "common/event/IEvent.hpp"
#include "common/SPSCQueue.hpp"

#include <ftxui/component/animation.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp> 
#include <ftxui/screen/screen.hpp> 

#include "common/command/GetModuleList.hpp"
#include "common/command/SetParameter.hpp"
#include "common/command/GetConnectionList.hpp"
#include "common/command/CreateModule.hpp"
#include "common/command/CreateConnection.hpp"
#include "common/command/DeleteModule.hpp"
#include "common/command/RemoveConnection.hpp"

#include "gui/TUI/widget/WaveFormWidget.hpp"
#include "gui/TUI/widget/OscilatorWidget.hpp"
#include "gui/TUI/widget/FilterWidget.hpp"
#include "gui/TUI/widget/ConnectionWidget.hpp"
#include "gui/TUI/widget/ScrollableWidget.hpp"
#include "gui/TUI/widget/CommandEntryWidget.hpp"
#include "gui/TUI/widget/ArpegiatorWidget.hpp"
#include "gui/TUI/widget/ADSRWidget.hpp"
#include "gui/TUI/widget/InfoWidget.hpp"
#include "gui/TUI/widget/WidgetFactory.hpp"
#include "gui/TUI/CommandHandler.hpp"

#include <iostream>

namespace {
    class EventPump : public ftxui::ComponentBase {
      GUI::TUI& tui_;
      ftxui::ScreenInteractive& screen_;
      ftxui::Component child_;
    public:
      EventPump(GUI::TUI& tui, ftxui::ScreenInteractive& screen, ftxui::Component child)
        : tui_(tui), screen_(screen), child_(std::move(child)) {
        Add(child_);
      }
      void OnAnimation(ftxui::animation::Params& params) override { 
        tui_.processEvents();                 // process pending events
        screen_.RequestAnimationFrame();      // request another frame (keep the pump running)
      }
    };
}

namespace GUI {

    TUI::TUI(Common::SPSCQueue<std::unique_ptr<Common::ICommand>, 1024>& q,
               Common::SPSCQueue<std::unique_ptr<Common::IEvent>, 1024>& e)
        : Common::Producer<std::unique_ptr<Common::ICommand>>(q),
          Common::Consumer<std::unique_ptr<Common::IEvent>>(e),
        _screen(ftxui::ScreenInteractive::Fullscreen())
    {
        _container = ftxui::Container::Vertical({});
       auto root_child = ftxui::Renderer(_container, [this] {
         return _container->Render() | ftxui::xflex | ftxui::yflex;
        });
        _root = ftxui::Make<EventPump>(*this, _screen, root_child);
        _scopesList.push_back(0);
        _scopePaths.push_back("/");
    }

    void TUI::addWidget(std::shared_ptr<IWidget> widget) {
        _container->Add(widget->component());
        _widgets.push_back(widget);
    }

    void TUI::init() {
        pushCommand(std::make_unique<Common::GetModuleListCommand>( _scopesList.back()));
        pushCommand(std::make_unique<Common::GetConnectionListCommand>( _scopesList.back()));
        commandHandler(""); // initialization of router
        buildUI();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        processEvents(); // initial event processing
    }

    void TUI::start() {
        _running = true;
        _thread = std::thread([this]() {
            init();
            _screen.RequestAnimationFrame();  // start the animation loop
            _screen.Loop(_root);
        });

    }

    void TUI::stop() {
        _running = false;
        _screen.Exit();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void TUI::handleWaveformEvent(const double* samples, size_t numSamples) {
        if (!_running) return;
        for (auto& widget : _widgets) {
            auto waveformWidget = std::dynamic_pointer_cast<WaveformWidget>(widget);
            if (waveformWidget) {
                waveformWidget->setSamples(samples, numSamples);
            }
        }
        _screen.PostEvent(ftxui::Event::Custom); // Trigger a UI update
    }

    void TUI::handleConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                        const std::vector<std::string>& modules) {
            if (_connection_renderer) {
            auto connectionWidget = std::dynamic_pointer_cast<ConnectionWidget>(_connection_renderer);
            if (connectionWidget) {
                connectionWidget->setConnections(connections);
            }
            connectionWidget->setModules(modules);
        }
        _screen.PostEvent(ftxui::Event::Custom); // Trigger a UI update
    }

    void TUI::handleMessageEvent(Common::logType type, const std::string& message) {
        if (_info_widget) {
            auto infoWidget = std::dynamic_pointer_cast<InfoWidget>(_info_widget);
            if (infoWidget) {
                infoWidget->setInfoText(type, message);
            }
        }
        _screen.PostEvent(ftxui::Event::Custom); // Trigger a UI update
    }


    void TUI::handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& modules) {
        using namespace ftxui;

        _moduleParams = modules;

        //call the factory to create the widgets according to the module list
        auto scrollable = std::make_shared<ScrollableWidget>();
        for (auto& [moduleName, params] : _moduleParams) {
            WidgetFactory& factory = WidgetFactory::instance();
            try {
                std::cerr << "TUI :: " << "type: " << params.type << " name: " << moduleName << std::endl;
                auto widget = factory.create(std::string(params.type + "_" + std::to_string(params.id)), this);
                widget->setid(params.id);
                scrollable->addChild(widget);
            } catch (const std::exception& e) {
                std::cerr << "TUI::handleModuleListEvent - could not create widget for module " << moduleName << ": " << e.what() << std::endl;
            }
        }

        auto right_comp = scrollable->component();

        auto left_center_content = Container::Horizontal({
            _connection_component,
            _info_component | size(WIDTH, EQUAL, 60) | yflex,
        });

        auto left_content = Container::Vertical({
             left_center_content,
            _command_component,
        });

        auto left_stack = Renderer(left_content, [this, left_content] {
            using namespace ftxui;
            Element base = left_content->Render();
            return base;
        });

        auto main_container = Container::Horizontal({
            _waveform_renderer->component(),
            left_stack,
            right_comp,
        });

        std::string path_display = "Scope: ";
        for (const auto& p : _scopePaths) {
            path_display += p;
        }
        auto root_renderer = Renderer(main_container, [=] {
            using namespace ftxui;
            auto left_col = vbox({
                _waveform_component->Render(),
                left_stack->Render(),
                filler(),
                text(path_display) | dim,
            }) | xflex | yflex;

            return hbox({
                left_col,
                right_comp->Render() | size(WIDTH, EQUAL, 40) | yflex,
            }) | xflex | yflex | border;
        });

        _container->DetachAllChildren();
        _container->Add(root_renderer);
        _widgets.push_back(scrollable);

        _screen.PostEvent(Event::Custom);
    }

    void TUI::commandHandler(std::string command) {

        static bool initialized = false;
        if (!initialized) {

            // create [<moduleType>, ...]
            router.registerCommand("create", [this](const GUI::ParsedCommand& cmd) {
                if (cmd.args.empty())
                    return;
                for (const auto& moduleType : cmd.args) {
                    try {
                        pushCommand(std::make_unique<Common::CreateModuleCommand>(moduleType, _scopesList.back()));
                    } catch (const std::exception& e) {
                        std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                            ->setInfoText(Common::logType::ERROR, "Invalid module type: " + moduleType);
                        continue;
                    }
                }
            });

            // delete [<moduleId>, ...]
            router.registerCommand("delete", [this](const GUI::ParsedCommand& cmd) {
                if (cmd.args.empty())
                    return;
                for (const auto& moduleIdStr : cmd.args) {
                    try {
                        const int moduleId = std::stoi(moduleIdStr);
                        pushCommand(std::make_unique<Common::DeleteModuleCommand>(moduleId, _scopesList.back()));
                    } catch (...) {
                        std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                            ->setInfoText(Common::logType::ERROR, "Invalid module ID: " + moduleIdStr);
                    }
                }
            });

            // connect <srcId> <srcPort> <dstId> <dstPort>
            router.registerCommand("connect", [this](const GUI::ParsedCommand& cmd) {
                if (cmd.args.size() != 4) {
                    std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                        ->setInfoText(Common::logType::ERROR, "Usage: connect <srcId> <srcPort> <dstId> <dstPort>");
                    return;
                }
                pushCommand(std::make_unique<Common::CreateConnectionCommand>(
                    cmd.args[0], cmd.args[1], cmd.args[2], cmd.args[3], _scopesList.back()));
            });

            // disconnect <srcId> <srcPort> <dstId> <dstPort>
            router.registerCommand("disconnect", [this](const GUI::ParsedCommand& cmd) {
                if (cmd.args.size() != 4) {
                    std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                        ->setInfoText(Common::logType::ERROR, "Usage: disconnect <srcId> <srcPort> <dstId> <dstPort>");
                    return;
                }
                pushCommand(std::make_unique<Common::RemoveConnectionCommand>(
                    cmd.args[0], cmd.args[1], cmd.args[2], cmd.args[3], _scopesList.back()));
            });

            // enter <scopeId>
            router.registerCommand("enter", [this](const ParsedCommand& cmd) {
                if (cmd.args.size() != 1) return;
                try {
                    const unsigned int scopeId = static_cast<unsigned int>(std::stoul(cmd.args[0]));
                    _scopesList.push_back(scopeId);

                    auto conn = std::static_pointer_cast<GUI::ConnectionWidget>(_connection_renderer);
                    bool found = false;
                    for (const auto& name : conn->getModuleNames()) {
                        if (name.find("instrument_" + std::to_string(scopeId)) != std::string::npos) {
                            _scopePaths.push_back(name + "/");
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        _scopesList.pop_back();
                        std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                            ->setInfoText(Common::logType::ERROR, "Scope id " + std::to_string(scopeId) + " not found");
                        return;
                    }
                    pushCommand(std::make_unique<Common::GetModuleListCommand>(scopeId));
                    pushCommand(std::make_unique<Common::GetConnectionListCommand>(scopeId));
                    std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                        ->setInfoText(Common::logType::INFO, "Entered " + _scopePaths.back());
                } catch (...) {
                    // ignore parse errors
                }
            });

            // exit
            router.registerCommand("exit", [this](const ParsedCommand&) {
                if (_scopesList.size() > 1) {
                    std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                        ->setInfoText(Common::logType::INFO, "Exited " + _scopePaths.back());
                    _scopesList.pop_back();
                    _scopePaths.pop_back();
                    const unsigned int scopeId = _scopesList.back();
                    pushCommand(std::make_unique<Common::GetModuleListCommand>(scopeId));
                    pushCommand(std::make_unique<Common::GetConnectionListCommand>(scopeId));
                } else {
                    std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                        ->setInfoText(Common::logType::ERROR, "Already at root scope");
                }
            });

            // help
            router.registerCommand("help", [this](const ParsedCommand&) {
                std::string helpText = "Available commands:\n";
                helpText += "  create <moduleType>       - Create a new module of the specified type\n";
                helpText += "  delete <moduleId>         - Delete the module with the specified ID\n";
                helpText += "  connect <srcId> <srcPort> <dstId> <dstPort> - Create a connection between modules\n";
                helpText += "  enter <scopeId>          - Enter the specified scope\n";
                helpText += "  exit                     - Exit the current scope\n";
                helpText += "  help                     - Show this help message\n";
                std::static_pointer_cast<GUI::InfoWidget>(_info_widget)
                    ->setInfoText(Common::logType::INFO, helpText);
            });

            initialized = true;
        }
        auto parsedCommand = GUI::ParseCommandLine(command);
        router.dispatchCommand(parsedCommand);
    }

    void TUI::buildUI() {
        using namespace ftxui;

        //waveform widget builder
        auto waveformWidget  = std::make_shared<GUI::WaveformWidget>();
        auto waveform_renderer = Renderer(waveformWidget->component(), [this, waveformWidget] {
            int term_h = _screen.dimy();
            return waveformWidget->component()->Render()
                   | size(HEIGHT, EQUAL, std::max(10, term_h / 4))
                   | xflex;
        });
        _waveform_renderer = waveformWidget;
        _waveform_component = waveform_renderer;


        //command entry widget builder
        auto commandEntryWidget = std::make_shared<GUI::CommandEntryWidget>(
            [this](const std::string& commandText) {
                commandHandler(commandText);
            }
            , router.getRegisteredCommands()
        );

        auto cmd_panel = Renderer(commandEntryWidget->component(), [this, commandEntryWidget] {
            using namespace ftxui;
            const int term_w = _screen.dimx();

            Element input_el = commandEntryWidget->component()->Render();
            Element base = input_el | reflect(cmd_input_box) | xflex;

            if (commandEntryWidget->suggestionsVisible()) {
                const int input_width = std::max(1, cmd_input_box.x_max - cmd_input_box.x_min + 1);
                Element overlay_list = commandEntryWidget->renderOverlay()
                                        | xflex;

                const int dx = std::max(0, (cmd_input_box.x_min - cmd_panel_box.x_min));
                const int dy = std::max(0, (cmd_input_box.y_min - cmd_panel_box.y_max));

                Element positioned = dbox({
                    filler() | size(HEIGHT, EQUAL, dy),
                        overlay_list,
                });

                base = hbox({
                    base,       
                    positioned,
                });
            }

            return (base
                    | xflex
                    | border
                    | reflect(cmd_panel_box));
        });
        //store widgets
        _command_entry = commandEntryWidget;
        _command_component = cmd_panel;
        _command_element = cmd_panel->Render();

        
        //connection widget builder
        auto connWidget = std::make_shared<GUI::ConnectionWidget>();

        auto conn_panel = Renderer(connWidget->component(), [=] {
            int term_w = _screen.dimx();
            int term_h = _screen.dimy();
            return connWidget->component()->Render()
                   | size(WIDTH,  EQUAL, std::max(20, term_w / 2))
                   | size(HEIGHT, EQUAL, std::max(10, term_h / 2));
        });
        //store widgets
        _connection_renderer = connWidget;
        _connection_component = conn_panel;
        _connection_element = conn_panel->Render();

        //info widget builder
        auto infoWidget = std::make_shared<GUI::InfoWidget>();
        auto info_panel = Renderer(infoWidget->component(), [=] {
            int term_w = _screen.dimx();
            int term_h = _screen.dimy();
            infoWidget->resize(std::max(20, term_w / 2), std::max(10, term_h / 2));
            return infoWidget->component()->Render()
                   | size(WIDTH,  EQUAL, std::max(20, term_w / 2))
                   | size(HEIGHT, EQUAL, std::max(10, term_h / 2));
        });
        //store widgets
        _info_widget = infoWidget;
        _info_component = info_panel;
        _info_element = info_panel->Render();

        _widgets.push_back(_waveform_renderer);
        _widgets.push_back(_connection_renderer);
        _widgets.push_back(_info_widget);
    }


}