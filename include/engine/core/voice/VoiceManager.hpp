/*
    * @file VoiceManager.hpp
    * @brief VoiceManager class definition.
    * @ingroup engine_core
    * @details Defines the VoiceManager class responsible for managing multiple voices
    * in a polyphonic synthesizer. In the graph, only the instrument module interacts with
    * the VoiceManager to handle note on/off events and voice allocation.
*/

#ifndef VOICEMANAGER_HPP
    #define VOICEMANAGER_HPP

    #include <vector>
    #include <memory>
    #include "engine/core/voice/Voice.hpp"
    #include "engine/core/mix/IMixStrategy.hpp"
    #include "engine/core/mix/AditiveMix.hpp"
    #include "engine/AudioGraph/AudioGraph.hpp"

namespace Engine::Core {
    /// @brief Class managing multiple voices for polyphonic synthesis.
    /// @details The VoiceManager handles voice allocation, note on/off events,
    /// and maintains a pool of Voice objects. It supports different mixing strategies
    /// for combining audio signals from active voices.
    class VoiceManager {
    public:

        /// @brief Constructs a VoiceManager with a maximum number of voices.
        /// @param maxVoices The maximum number of voices to manage.
        VoiceManager(size_t maxVoices);

        /// @brief Destroys the VoiceManager and its voices.
        ~VoiceManager();

        /// @brief Handles a note on event, allocating a voice.
        /// @param noteNumber The MIDI note number.
        /// @param velocity The velocity of the note on event.
        std::shared_ptr<Voice> noteOn(int noteNumber, float velocity, AudioGraph& graph);

        /// @brief Handles a note off event, releasing the corresponding voice.
        /// @param noteNumber The MIDI note number.
        void noteOff(int noteNumber);

        void removeInactiveVoices();

        /// @brief Gets the list of all voices managed by the VoiceManager.
        /// @return A vector of shared pointers to all Voice objects.
        std::vector<std::shared_ptr<Voice>>& getVoices() { return _voices; }

        /// @brief Gets the list of currently active voices.
        /// @return A vector of shared pointers to active Voice objects.
        std::vector<std::shared_ptr<Voice>> getActiveVoices() const { return _activeVoices; }

        /// @brief Sets the mixing strategy for combining audio signals.
        /// @param mixStrategy A shared pointer to the mixing strategy to use.
        void setMixStrategy(std::shared_ptr<IMixStrategy> mixStrategy) { _mixStrategy = mixStrategy; }

        /// @brief Gets the current mixing strategy.
        /// @return A shared pointer to the current mixing strategy.
        std::shared_ptr<IMixStrategy> getMixStrategy() const { return _mixStrategy; }

    private:
        /// @brief The maximum number of voices.
        size_t _maxVoices;

        /// @brief The pool of all voices managed by the VoiceManager.
        std::vector<std::shared_ptr<Voice>> _voices;

        /// @brief Finds a free voice for allocation.
        /// @return A shared pointer to a free Voice, or nullptr if none are available.
        std::shared_ptr<Voice> findFreeVoice();

        /// @brief The list of currently active voices.
        std::vector<std::shared_ptr<Voice>> _activeVoices;

        /// @brief The mixing strategy used to combine audio signals from voices.
        std::shared_ptr<IMixStrategy> _mixStrategy;
    };
} // namespace Engine::Core

#endif // VOICEMANAGER_HPP