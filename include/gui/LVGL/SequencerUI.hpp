#ifndef SEQUENCER_UI_HPP
#define SEQUENCER_UI_HPP

#include "lvgl/lvgl.h"
#include <vector>
#include <string>

namespace GUI {

/// @brief Configuration for a single instrument track
struct InstrumentTrack {
    std::string name;
    lv_color_t color;
    int radius;  // Distance from center
    std::vector<bool> steps;  // true = step active, false = step inactive
    std::vector<lv_obj_t*> step_objects;  // Visual objects for steps
};

/// @brief Configuration for a category (group of instruments)
struct InstrumentCategory {
    std::vector<InstrumentTrack> tracks;
    lv_point_t center;  // Center position of this category
};

/// @brief Main sequencer UI class
class SequencerUI {
public:
    SequencerUI();
    ~SequencerUI();
    
    /// @brief Initialize the sequencer UI
    /// @param parent Parent LVGL object
    void init(lv_obj_t* parent);
    
    /// @brief Set the BPM value
    void setBPM(int bpm);
    
    /// @brief Set the current playback position with interpolation
    /// @param step Current step (0-15)
    /// @param sampleCounter Samples elapsed in current step
    /// @param samplesPerStep Total samples per step
    void setPlayPosition(int step, size_t sampleCounter, size_t samplesPerStep);
    
    /// @brief Set playing state (true = playing, false = paused)
    void setPlaying(bool playing);
    
    /// @brief Add an instrument with its pattern dynamically
    /// @param name Name of the instrument
    /// @param pattern Pattern of steps
    void addInstrument(const std::string& name, const std::vector<bool>& pattern);
    
    /// @brief Toggle a step for a specific category, track, and step index
    void toggleStep(int category, int track, int step);
    
    /// @brief Update the display
    void update();
    
private:
    lv_obj_t* _container;
    lv_obj_t* _bpm_label;
    lv_obj_t* _playhead_line;
    
    std::vector<InstrumentCategory> _categories;
    int _bpm;
    int _play_position;
    float _playhead_angle;  // Current playhead angle in degrees
    float _target_angle;    // Target angle from audio engine
    uint32_t _last_update_time;  // For smooth animation timing
    bool _is_playing;  // Whether the sequencer is currently playing
    
    // Drawing helpers
    void createUI();
    void updatePlayhead();
    
    // Helper to calculate point on circle
    lv_point_t getPointOnCircle(const lv_point_t& center, int radius, float angle);
};

} // namespace GUI

#endif // SEQUENCER_UI_HPP
