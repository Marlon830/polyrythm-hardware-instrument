/*
 * @file Voice.hpp
 * @brief Voice class definition.
 * @ingroup engine_core
 * @details Defines the Voice class representing a single voice in the synthesizer.
*/

#ifndef VOICE_HPP
    #define VOICE_HPP

    #include "engine/AudioGraph/AudioGraph.hpp"


namespace Engine::Core {

    /// @brief Enumeration representing the state of a voice.
    /// @details Used to track whether a voice is active, released, or inactive.
    enum VoiceState {
        ACTIVE,
        RELEASED,
        INACTIVE
    };

    class Voice {
    public:
        /// @brief Constructs a new Voice object.
        Voice(AudioGraph audioGraph);

        /// @brief Destroys the Voice object.
        ~Voice();

        /// @brief Handles the event when a note is pressed.
        /// @param noteNumber The MIDI note number.
        /// @param velocity The velocity of the note press.
        void noteOn(int noteNumber, float velocity);

        /// @brief Handles the event when a note is released.
        /// @param noteNumber The MIDI note number.
        void noteOff(int noteNumber);

        /// @brief Checks if the voice is currently active.
        /// @return True if the voice is active, false otherwise.
        bool isActive() const;

        bool isReleased() const;

        bool isInactive() const;

        void setState(VoiceState state);

        /// @brief Gets the MIDI note number associated with the voice.
        /// @return The MIDI note number.
        int getNoteNumber() const { return _noteNumber; }

        /// @brief Gets the velocity of the note.
        /// @return The velocity value.
        float getVelocity() const { return _velocity; }

        /// @brief Gets the audio graph associated with the voice.
        /// @return Reference to the AudioGraph object.
        AudioGraph& getAudioGraph() { return _audioGraph; }

        void setAudioGraph(const AudioGraph& graph) { _audioGraph = graph; }
    private:

        /// @brief The current state of the voice.
        VoiceState _state;

        /// @brief The MIDI note number assigned to the voice.
        int _noteNumber;

        /// @brief The velocity of the note.
        float _velocity;

        /// @brief The audio graph associated with the voice.
        AudioGraph _audioGraph;
    };
}

#endif