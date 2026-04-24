/*
@file TUI.hpp
@brief Declaration of the TUI class for text-based user interface.
@ingroup gui

TUI based implementation of the IGUI interface using FTXUI library.
*/

#ifndef GUI_TUI_TUI_HPP
#define GUI_TUI_TUI_HPP

#include "gui/IGUI.hpp"
#include "gui/TUI/widget/IWidget.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>

#include "common/Producer.hpp"
#include "common/Consumer.hpp"
#include "common/SPSCQueue.hpp"
#include "common/command/ICommand.hpp"
#include "common/event/IEvent.hpp"

#include "gui/TUI/CommandHandler.hpp"

#include <memory>
namespace GUI {
    /// @brief TUI based implementation of the IGUI interface using FTXUI library.
    /// @details This class provides a text-based user interface (TUI) implementation
    /// of the IGUI interface, utilizing the FTXUI library for rendering and interaction.
    /// It depends on producer-consumer queues for command and event handling.
    class TUI : public IGUI,
                public Common::Producer<std::unique_ptr<Common::ICommand>>,
                public Common::Consumer<std::unique_ptr<Common::IEvent>> {
    public:
        /// @brief Constructs the TUI with command and event queues.
        TUI(Common::SPSCQueue<std::unique_ptr<Common::ICommand>, 1024>& q,
               Common::SPSCQueue<std::unique_ptr<Common::IEvent>, 1024>& e);

        /// @brief Adds a widget to the TUI.
        void addWidget(std::shared_ptr<IWidget> widget);

        /// @brief Initializes the TUI Widgets.
        void init();

        /// @brief Starts the TUI event loop in a separate thread.
        void start() override;

        /// @brief Stops the TUI event loop and joins the thread.
        void stop() override;

        /// @brief Processes pending events from the event queue.
        void process() override {}; // not used, we use processEvents instead

        /// @brief Processes all pending events from the event queue.
        void processEvents() {
            processAll();
        }

        /// @brief Handles waveform events.
        void handleWaveformEvent(const double* samples, size_t numSamples) override;

        /// @brief Handles module list events.
        void handleModuleListEvent(const std::map<std::string, Common::ModuleInfo>& modules) override;

        /// @brief Handles connection list events.
        void handleConnectionListEvent(const std::vector<std::shared_ptr<Engine::ConnectionInfo>>& connections,
                                        const std::vector<std::string>& modules) override;

        /// @brief Handles message events.
        void handleMessageEvent(Common::logType type, const std::string& message) override;

        /// @brief Pushes a command to the command queue. this is a helper method
        void pushCommand(std::unique_ptr<Common::ICommand> cmd) {
            this->push(std::move(cmd));
        }

        /// @brief Container component for the TUI.
        ftxui::Component _container;

        /// @brief Root component for the TUI.
        ftxui::Component _root;

    protected:
        /// @brief Handles an event by dispatching it. This overrides the Consumer's handle method.
        void handle(std::unique_ptr<Common::IEvent>& event) override {
            event->dispatch(*this);
        }
    private:
        /// @brief Handles command input from the user.
        void commandHandler(std::string command);

        /// @brief Builds the user interface components.
        void buildUI();

        ftxui::ScreenInteractive _screen;std::vector<std::shared_ptr<IWidget>> _widgets;

        CommandHandler router;

        // Scope management
        std::vector<unsigned int> _scopesList;
        std::vector<std::string> _scopePaths;

        /// @brief Waveform widget and component
        std::shared_ptr<IWidget> _waveform_renderer;
        ftxui::Element _waveform_element;
        ftxui::Component _waveform_component; 

        /// @brief Connection widget and component
        std::shared_ptr<IWidget> _connection_renderer;
        ftxui::Component _connection_component;
        ftxui::Element _connection_element;

        /// @brief Command entry widget and component
        std::shared_ptr<IWidget> _command_entry;
        ftxui::Component _command_component;
        ftxui::Element _command_element;

        /// @brief Info widget and component
        std::shared_ptr<IWidget> _info_widget;
        ftxui::Component _info_component;
        ftxui::Element _info_element;

        /// @brief Command panel box and input box
        ftxui::Box cmd_panel_box;
        ftxui::Box cmd_input_box;    

        /// @brief Stored module parameters
        std::map<std::string, Common::ModuleInfo> _moduleParams;

        /// @brief Running flag and thread for the TUI event loop
        std::atomic<bool> _running = false;
        std::thread _thread;
    };
} 
#endif // GUI_TUI_TUI_HPP