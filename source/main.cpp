#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <memory>
#include <chrono>
#include <iostream>
#include <cmath>
#include <cmath>
#include <cstdlib>
#include <string>

#include "engine/module/ModuleFactory.hpp"
#include "engine/module/OscModule.hpp"
#include "engine/module/DebugModule.hpp"
#include "engine/module/AudioOutModule.hpp"
#include "engine/module/FilterModule.hpp"
#include "engine/module/Instrument.hpp"
#include "engine/module/EventEmitter.hpp"
#include "engine/module/StepSequencerModule.hpp"
#include "engine/module/SamplerModule.hpp"

#include "engine/AudioGraph/AudioGraph.hpp"
#include "engine/signal/AudioSignal.hpp"
#include "engine/port/InputPort.hpp"
#include "engine/core/voice/VoiceManager.hpp"
#include "engine/parameter/Param.hpp"

#include "common/event/AudioWaveEvent.hpp"
#include "common/command/GetModuleList.hpp"
#include "engine/module/StepSequencerModule.hpp"

#include "engine/utils/WavLoader.hpp"

#include "gui/LVGL/LVGL.hpp"
#include "gui/NullGUI.hpp"

#include "App.hpp"

#include <sndfile.h>

int main()
{
    const char* headlessEnv = std::getenv("TINYFW_HEADLESS");
    const bool headless = headlessEnv != nullptr && std::string(headlessEnv) == "1";

    std::unique_ptr<GUI::IGUI> gui;
    if (headless) {
        std::cout << "[Main] Starting in headless mode (NullGUI)" << std::endl;
        gui = std::make_unique<GUI::NullGUI>();
    } else {
        std::cout << "[Main] Starting with LVGL GUI" << std::endl;
        gui = std::make_unique<GUI::LVGL>();
    }

    App app(std::move(gui));
    app.start();

    // Engine::Util::WavData kickSample = Engine::Util::WavLoader::load("samples/kick.wav");
    // Engine::Util::WavData snareSample = Engine::Util::WavLoader::load("samples/snare.wav");
    // Engine::Util::WavData hihatSample = Engine::Util::WavLoader::load("samples/hihat.wav");

    // std::map<int, Engine::Util::WavData> sampleMap{
    //     {36, kickSample},
    //     {38, snareSample},
    //     {42, hihatSample}
    // };

    // app._guiControl->setParameter(0, "Step Count", static_cast<size_t>(32));

    // app._guiControl->setParameter(1, "Samples", sampleMap);

    // app._guiControl->setParameter(0, "BPM", 140.0f);
    

    // Engine::Module::SequencerTrack kickTrack(36, 127, 32);
    // kickTrack.pattern[0] = true;
    // kickTrack.pattern[2] = true;
    // kickTrack.pattern[7] = true;
    // kickTrack.pattern[10] = true;

    // Engine::Module::SequencerTrack snareTrack(38, 100, 32);
    // snareTrack.pattern[4] = true;
    // snareTrack.pattern[12] = true;

    // Engine::Module::SequencerTrack hihatTrack(42, 40, 32);
    // hihatTrack.pattern[0] = true;
    // hihatTrack.pattern[2] = true;
    // hihatTrack.pattern[4] = true;
    // hihatTrack.pattern[6] = true;
    // hihatTrack.pattern[7] = true;
    // hihatTrack.pattern[8] = true;
    // hihatTrack.pattern[10] = true;
    // hihatTrack.pattern[12] = true;
    // hihatTrack.pattern[14] = true;
    // hihatTrack.pattern[15] = true;

    // app._tui->setParameter(0, "Tracks", std::vector<Engine::Module::SequencerTrack>{
    //     kickTrack,
    //     snareTrack,
    //     hihatTrack
    // });

    // app._guiControl->setParameter(1, "Waveform", Engine::Module::SINE);
    // app._guiControl->setParameter(6, "Time Interval", 200.0f);

    // app._guiControl->setParameter(1, "Cutoff Frequency", 300.0f);
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // app._guiControl->pushCommand(std::make_unique<Common::GetModuleListCommand>());

    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // app._guiControl->process();

    std::this_thread::sleep_for(std::chrono::seconds(10000));
    app.stop();
}

// int main() {
//         // Create the drum box
//     auto drumBox = std::make_shared<Engine::Module::StepSequencerModule>(16, 120.0f);

//     // Create the tracks
//     Engine::Module::SequencerTrack kickTrack(36, 127, 16);
//     kickTrack.pattern[0] = true;
//     kickTrack.pattern[4] = true;
//     kickTrack.pattern[8] = true;
//     kickTrack.pattern[12] = true;
//     drumBox->addTrack(kickTrack);

//     Engine::Module::SequencerTrack snareTrack(38, 100, 16);
//     snareTrack.pattern[4] = true;
//     snareTrack.pattern[12] = true;
//     drumBox->addTrack(snareTrack);

//     Engine::Module::SequencerTrack hihatTrack(42, 80, 16);
//     for (size_t i = 0; i < 16; i += 2) {
//         hihatTrack.pattern[i] = true;
//     }
//     drumBox->addTrack(hihatTrack);

//     // Create a SAMPLER
//     auto drumSampler = std::make_shared<Engine::Module::SamplerModule>(16);

//     // Load WAV samples for each sound
//     drumSampler->loadSample(36, "samples/kick.wav");    // Kick
//     drumSampler->loadSample(38, "samples/snare.wav");   // Snare
//     drumSampler->loadSample(42, "samples/hihat.wav");   // Hi-hat

//     // Create the audio output
//     auto audioOut = std::make_shared<Engine::Module::AudioOutModule>();

//     // Connect everything together
//     audioGraph.addModule(drumBox);
//     audioGraph.addModule(drumSampler);
//     audioGraph.addModule(audioOut);

//     audioGraph.connect(
//         drumBox->getOutputPorts()[0],
//         drumSampler->getInputPorts()[0]
//     );
//     audioGraph.connect(
//         drumSampler->getOutputPorts()[0],
//         audioOut->getInputPorts()[0]
//     );

//     // Main loop
//     while (true) {
//         Engine::Core::AudioContext context{512, 48000, 440.0f};
//         audioGraph.setContext(context);
//         audioGraph.process();
//     }

//     return 0;
// }
