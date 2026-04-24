#ifndef ENGINE_AUDIOENGINE_HPP
#define ENGINE_AUDIOENGINE_HPP

#include "engine/AudioGraph/AudioGraph.hpp"
#include "common/Consumer.hpp"
#include "common/Producer.hpp"
#include "common/SPSCQueue.hpp"
#include "common/command/ICommand.hpp"
#include "common/event/IEvent.hpp"
#include "engine/core/IEventEmitter.hpp"
#include <thread>
#include <memory>
#include <iostream>
#include <vector> 

namespace Engine {
    namespace Module {
        class StepSequencerModule;   // Forward declaration
        class PolyRhythmModule;      // Forward declaration
    }
    
    /// @brief Main audio engine class managing the audio graph and processing.
    /// @details The AudioEngine class encapsulates the AudioGraph and provides
    /// methods to initialize, start, stop, and process audio data. It serves
    /// as the central component for audio processing in the engine.
    /// @ingroup engine_audioengine
    class AudioEngine : 
        public Common::Consumer<std::unique_ptr<Common::ICommand>>,
        public Common::Producer<std::unique_ptr<Common::IEvent>>,
        public Core::IEventEmitter {
    public:
        /// @brief Constructs a new AudioEngine.
        AudioEngine(Common::SPSCQueue<std::unique_ptr<Common::ICommand>, 1024>& commandQueue,
                    Common::SPSCQueue<std::unique_ptr<Common::IEvent>, 1024>& eventQueue);

        /// @brief Destroys the AudioEngine and releases resources.
        ~AudioEngine();

        /// @brief Initializes the audio engine.
        /// @return True if initialization was successful, false otherwise.
        bool initialize();

        /// @brief Starts audio processing.
        void start();

        /// @brief Stops audio processing.
        void stop();

        /// @brief Processes audio data through the audio graph.
        void process();


    template<typename T>
    bool setParameter(int id, const std::string& name, T value) {
        auto modules = _audioGraph.getAllParams();
        for (const auto& [moduleName, params] : modules) {
            if (moduleName.find(std::to_string(id)) != std::string::npos) {

                for (const auto& param : params.parameters) {
                    if (param->getName() == name) {
                        // Assuming ParamBase has a setValue method
                        if (auto floatParam = std::dynamic_pointer_cast<Module::Param<T>>(param)) {
                            floatParam->set(value);
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
        
    void emit(std::unique_ptr<Common::IEvent> event) override {
        push(std::move(event));
    }

    AudioGraph& getAudioGraph() {
        return _audioGraph;
    }

    /// @brief Gets the audio graph for a specific scope.
    /// @param scope The scope identifier.
    /// @return Reference to the AudioGraph for the given scope.
    AudioGraph& getScopedAudioGraph(unsigned int scope);

    void createConnection(const std::string& srcModuleId, const std::string& srcPort,
                          const std::string& dstModuleId, const std::string& dstPort, unsigned int scope = 0);

    void removeConnection(const std::string& srcModuleId, const std::string& srcPort,
                          const std::string& dstModuleId, const std::string& dstPort, unsigned int scope = 0);

    protected:
        void handle(std::unique_ptr<Common::ICommand>& cmd) override;

    public:
        /// @brief Per-instrument polyrhythmic config — each has its own module, shapes, and timeline.
        struct PolyInstrument {
            std::shared_ptr<Module::PolyRhythmModule> module;
            std::string name;
            uint32_t    color  = 0xffffff;
            int         radius = 100;
            uint32_t    shapeCounter = 1;  ///< Persistent counter for unique shape names
        };

        enum class HardwareMode {
            TogglePoint,
            AddRemovePoint,
            Rotate
        };

    private:
        std::atomic<bool> _isRunning; ///< Flag indicating if the audio engine is running.
        std::atomic<bool> _isPlaying; ///< Flag indicating if audio is currently playing (not paused).
        AudioGraph _audioGraph; ///< The audio graph managed by the engine.
        std::shared_ptr<Module::StepSequencerModule> _mainStepSequencer; ///< Main step sequencer for playhead tracking.

        std::vector<PolyInstrument> _polyInstruments;
        size_t _selectedRing = 0;
        std::vector<size_t> _selectedPoints;
        HardwareMode _hardwareMode = HardwareMode::Rotate;
    };
} // namespace Engine

#endif // ENGINE_AUDIOENGINE_HPP