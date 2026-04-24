#include "gui/LVGL/SequencerUI.hpp"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GUI {

SequencerUI::SequencerUI() 
    : _container(nullptr), _bpm_label(nullptr), _playhead_line(nullptr), 
      _bpm(60), _play_position(0), _playhead_angle(0.0f), _target_angle(0.0f),
      _last_update_time(0), _is_playing(false) {
}

SequencerUI::~SequencerUI() {
}

void SequencerUI::init(lv_obj_t* parent) {
    std::cout << "[SequencerUI] init() started" << std::endl;
    
    // Create container
    _container = lv_obj_create(parent);
    std::cout << "[SequencerUI] Container created" << std::endl;
    
    lv_obj_set_size(_container, 800, 480);
    lv_obj_center(_container);
    lv_obj_set_style_bg_color(_container, lv_color_hex(0x0a1428), 0);
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_set_style_pad_all(_container, 0, 0);
    std::cout << "[SequencerUI] Container configured" << std::endl;
    
    // Initialize default categories (3 categories with 4 instruments each)
    // Category 1 (left) - center at (200, 240)
    InstrumentCategory cat1;
    cat1.center = {200, 240};
    cat1.tracks = {
        {"Clap",   lv_color_hex(0xff8c00), 140, std::vector<bool>(16, false), {}},
        {"Hi Hat", lv_color_hex(0x00ff00), 110, std::vector<bool>(16, false), {}},
        {"Snare",  lv_color_hex(0x0080ff), 80,  std::vector<bool>(16, false), {}},
        {"Kick",   lv_color_hex(0xffffff), 50,  std::vector<bool>(16, false), {}}
    };
    
    // Category 2 (center) - center at (400, 240)
    InstrumentCategory cat2;
    cat2.center = {400, 240};
    cat2.tracks = {
        {"Clap2",   lv_color_hex(0xff8c00), 140, std::vector<bool>(16, false), {}},
        {"Hi Hat2", lv_color_hex(0x00ff00), 110, std::vector<bool>(16, false), {}},
        {"Snare2",  lv_color_hex(0x0080ff), 80,  std::vector<bool>(16, false), {}},
        {"Kick2",   lv_color_hex(0xffffff), 50,  std::vector<bool>(16, false), {}}
    };
    
    // Category 3 (right) - center at (600, 240)
    InstrumentCategory cat3;
    cat3.center = {600, 240};
    cat3.tracks = {
        {"Clap3",   lv_color_hex(0xff8c00), 140, std::vector<bool>(16, false), {}},
        {"Hi Hat3", lv_color_hex(0x00ff00), 110, std::vector<bool>(16, false), {}},
        {"Snare3",  lv_color_hex(0x0080ff), 80,  std::vector<bool>(16, false), {}},
        {"Kick3",   lv_color_hex(0xffffff), 50,  std::vector<bool>(16, false), {}}
    };
    
    _categories.push_back(cat1);
    _categories.push_back(cat2);
    _categories.push_back(cat3);
    std::cout << "[SequencerUI] Categories added" << std::endl;
    
    // Set some default active steps for demonstration
    _categories[0].tracks[0].steps[0] = true;
    _categories[0].tracks[0].steps[4] = true;
    _categories[0].tracks[0].steps[8] = true;
    _categories[0].tracks[0].steps[12] = true;
    
    _categories[0].tracks[1].steps[2] = true;
    _categories[0].tracks[1].steps[6] = true;
    _categories[0].tracks[1].steps[10] = true;
    _categories[0].tracks[1].steps[14] = true;
    std::cout << "[SequencerUI] Default steps set" << std::endl;
    
    // Create BPM label at center
    _bpm_label = lv_label_create(_container);
    lv_label_set_text_fmt(_bpm_label, "%d", _bpm);
    lv_obj_set_style_text_color(_bpm_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_bpm_label, &lv_font_montserrat_20, 0);
    lv_obj_align(_bpm_label, LV_ALIGN_CENTER, 0, 0);
    std::cout << "[SequencerUI] BPM label created" << std::endl;
    
    std::cout << "[SequencerUI] Calling createUI()..." << std::endl;
    createUI();
    std::cout << "[SequencerUI] init() completed!" << std::endl;
}

void SequencerUI::setBPM(int bpm) {
    _bpm = bpm;
    if (_bpm_label) {
        lv_label_set_text_fmt(_bpm_label, "%d", _bpm);
    }
}

void SequencerUI::setPlayPosition(int step, size_t sampleCounter, size_t samplesPerStep) {
    _play_position = step % 16;
    
    // Calculate interpolated position as a float
    float interpolation = 0.0f;
    if (samplesPerStep > 0) {
        interpolation = static_cast<float>(sampleCounter) / static_cast<float>(samplesPerStep);
    }
    
    // Calculate target angle: each step is 22.5° (360° / 16 steps)
    float position = static_cast<float>(_play_position) + interpolation;
    _target_angle = position * 22.5f;  // 360° / 16 = 22.5° per step
}

void SequencerUI::setPlaying(bool playing) {
    _is_playing = playing;
    std::cout << "[SequencerUI] Playing state: " << (_is_playing ? "PLAYING" : "PAUSED") << std::endl;
}

void SequencerUI::addInstrument(const std::string& name, const std::vector<bool>& pattern) {
    if (!_container) return;
    
    // Maximum 4 instruments
    static int instrument_count = 0;
    if (instrument_count >= 4) {
        std::cout << "[SequencerUI] Maximum 4 instruments reached, ignoring: " << name << std::endl;
        return;
    }
    
    // Define colors and radii for up to 4 instruments
    lv_color_t colors[] = {
        lv_color_hex(0xff8c00),  // Orange
        lv_color_hex(0x00ff00),  // Green
        lv_color_hex(0x0080ff),  // Blue
        lv_color_hex(0xffffff)   // White
    };
    int radii[] = {140, 110, 80, 50};
    
    lv_point_t center = {400, 240};
    lv_color_t color = colors[instrument_count];
    int radius = radii[instrument_count];
    
    std::cout << "[SequencerUI] Adding instrument " << instrument_count << ": " << name 
              << " (color: 0x" << std::hex << color.red << color.green << color.blue 
              << ", radius: " << std::dec << radius << ")" << std::endl;
    
    // Create dashed circle
    lv_obj_t* arc = lv_arc_create(_container);
    int arc_size = radius * 2;
    lv_obj_set_size(arc, arc_size, arc_size);
    lv_obj_set_pos(arc, center.x - radius, center.y - radius);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_width(arc, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    
    // Create dots based on pattern
    int num_steps = pattern.size();
    for (int i = 0; i < num_steps; i++) {
        float angle = (i * 360.0f / num_steps) - 90.0f;
        lv_point_t pos = getPointOnCircle(center, radius, angle);
        
        lv_obj_t* dot = lv_obj_create(_container);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_pos(dot, pos.x - 5, pos.y - 5);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, color, 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        lv_obj_set_style_border_color(dot, color, 0);
        
        if (pattern[i]) {
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(dot, LV_OPA_70, 0);
        }
    }
    
    instrument_count++;
    std::cout << "[SequencerUI] Instrument added successfully (" << instrument_count << "/4)" << std::endl;
}

void SequencerUI::toggleStep(int category, int track, int step) {
    if (category >= 0 && category < (int)_categories.size() &&
        track >= 0 && track < (int)_categories[category].tracks.size() &&
        step >= 0 && step < (int)_categories[category].tracks[track].steps.size()) {
        
        _categories[category].tracks[track].steps[step] = 
            !_categories[category].tracks[track].steps[step];
        
        // Update visual
        auto& obj = _categories[category].tracks[track].step_objects[step];
        if (_categories[category].tracks[track].steps[step]) {
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(obj, LV_OPA_20, 0);
        }
    }
}

void SequencerUI::update() {
    updatePlayhead();
}

void SequencerUI::createUI() {
    std::cout << "[SequencerUI] createUI() started" << std::endl;
    
    // Les cercles seront ajoutés dynamiquement via addInstrument()
    // quand le moteur audio enverra les informations
    
    std::cout << "[SequencerUI] Waiting for instruments from audio engine..." << std::endl;
    
    // Créer la ligne de playhead
    _playhead_line = lv_line_create(_container);
    lv_obj_set_style_line_color(_playhead_line, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_line_width(_playhead_line, 3, 0);
    
    static lv_point_precise_t line_points[2] = {{400, 240}, {400, 90}};
    lv_line_set_points(_playhead_line, line_points, 2);
    
    std::cout << "[SequencerUI] Playhead created" << std::endl;
    std::cout << "[SequencerUI] createUI() completed" << std::endl;
}

void SequencerUI::updatePlayhead() {
    if (!_playhead_line) return;
    
    // Smoothly interpolate current angle towards target angle
    if (_is_playing) {
        // Handle wrap-around (e.g., from 350° to 10°)
        float diff = _target_angle - _playhead_angle;
        
        // Normalize difference to [-180, 180]
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        
        // Smooth interpolation with easing factor
        // Higher value = faster catch-up, but less smooth
        float easing = 0.3f;
        _playhead_angle += diff * easing;
        
        // Normalize angle to [0, 360)
        while (_playhead_angle >= 360.0f) _playhead_angle -= 360.0f;
        while (_playhead_angle < 0.0f) _playhead_angle += 360.0f;
    }
    
    // Draw playhead at center (400, 240)
    lv_point_t center = {400, 240};
    // Offset by -90° so it starts at the top
    lv_point_t end_point = getPointOnCircle(center, 150, _playhead_angle - 90.0f);
    
    static lv_point_precise_t line_points[2];
    line_points[0].x = center.x;
    line_points[0].y = center.y;
    line_points[1].x = end_point.x;
    line_points[1].y = end_point.y;
    
    lv_line_set_points(_playhead_line, line_points, 2);
}

lv_point_t SequencerUI::getPointOnCircle(const lv_point_t& center, int radius, float angle) {
    float rad = angle * M_PI / 180.0f;
    lv_point_t point;
    point.x = center.x + (int)(radius * cos(rad));
    point.y = center.y + (int)(radius * sin(rad));
    return point;
}

} // namespace GUI
