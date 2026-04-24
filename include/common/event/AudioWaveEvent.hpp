#ifndef COMMON_EVENT_AUDIOWAVEEVENT_HPP
#define COMMON_EVENT_AUDIOWAVEEVENT_HPP

#include "common/event/IEvent.hpp"
#include "gui/IGUI.hpp"
#include <vector>
#include <iostream>

namespace Common {
/// @brief Event to represent an audio waveform.
/// @ingroup common_event
struct AudioWaveEvent : public IEvent {
    
    /// @brief Construct a new AudioWaveEvent
    /// @param samples Pointer to the array of audio samples.
    /// @param numSamples Number of samples in the array.
    AudioWaveEvent(const double* samples, size_t numSamples)
        : _samples(samples, samples + numSamples), _numSamples(numSamples) {}

    /// @brief Destroy the AudioWaveEvent
    ~AudioWaveEvent() override = default;

    /// @brief Dispatch the event to the given IGUI.
    /// @param gui The IGUI to dispatch the event to.
    void dispatch(GUI::IGUI& gui) override {
        gui.handleWaveformEvent(_samples.data(), _numSamples);
    }

    PCMBuffer _samples;
    size_t _numSamples;
};
}

#endif // COMMON_EVENT_AUDIOWAVEEVENT_HPP