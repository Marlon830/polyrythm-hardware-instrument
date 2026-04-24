/*
@file App.hpp
@brief Declaration of the App class that manages the AudioEngine and TUI.
@ingroup app
*/

#ifndef APP_HPP
    #define APP_HPP

    #include "engine/AudioEngine.hpp"
    #include "gui/IGUI.hpp"
    #include "ipc/HardwareInputService.hpp"
    #include "common/SPSCQueue.hpp"

    /// @brief The App class that encapsulates the AudioEngine and TUI.
    /// @details This class initializes and manages the lifecycle of the audio engine
    /// and the text-based user interface (TUI). It sets up the necessary communication
    /// queues for commands and events between the engine and the UI.
    class App {
    public:
        /// @brief Constructs the App with initialized AudioEngine and TUI.
        App(std::unique_ptr<GUI::IGUI> gui);

        /// @brief Destroys the App and its components.
        ~App();

        /// @brief Starts the AudioEngine and TUI.
        void start();

        /// @brief Stops the AudioEngine and TUI.
        void stop();

        /// @brief The AudioEngine instance.
        std::unique_ptr<Engine::AudioEngine> _audioEngine;

        /// @brief The GUI instance.
        std::unique_ptr<GUI::IGUI> _gui;

        /// @brief Headless hardware input bridge (bootstrap).
        IPC::HardwareInputService _hardwareInputService;
    private:

        /// @brief The command queue for inter-component communication.
        Common::SPSCQueue<std::unique_ptr<Common::ICommand>, 1024> _commandQueue;

        /// @brief The event queue for inter-component communication.
        Common::SPSCQueue<std::unique_ptr<Common::IEvent>, 1024> _eventQueue;

    };

#endif // APP_HPP