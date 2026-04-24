#include "App.hpp"

App::App(std::unique_ptr<GUI::IGUI> gui)
    : _audioEngine(std::make_unique<Engine::AudioEngine>(_commandQueue,
                                                         _eventQueue)),
      _gui(std::move(gui))
{
}

App::~App() = default;

void App::start()
{
    _audioEngine->initialize();
    _hardwareInputService.start();
    _gui->start();
    _audioEngine->start();
}

void App::stop()
{
    // Implementation for stopping the app
    _hardwareInputService.stop();
    _gui->stop();
    _audioEngine->stop();
}