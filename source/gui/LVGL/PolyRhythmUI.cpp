/**
 * @file PolyRhythmUI.cpp
 * @brief LVGL circular UI implementation for the polyrhythmic sequencer.
 *
 * @ingroup gui
 *
 * Each concentric ring represents one instrument with its OWN shapes &
 * timeline.  Clicking a ring label selects it; the shape panel on the right
 * shows controls (add / delete / +−subdiv / rotate) for the selected ring.
 */

#include "gui/LVGL/PolyRhythmUI.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GUI {

namespace {
    static float normalizeAngle(float a) {
        while (a >= 360.0f) a -= 360.0f;
        while (a < 0.0f)    a += 360.0f;
        return a;
    }

    // Clockwise distance from `from` to `to` in [0, 360)
    static float cwDistance(float from, float to) {
        float d = to - from;
        while (d < 0.0f)   d += 360.0f;
        while (d >= 360.0f) d -= 360.0f;
        return d;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Construction
// ═══════════════════════════════════════════════════════════════════════════════

PolyRhythmUI::PolyRhythmUI()  = default;

PolyRhythmUI::~PolyRhythmUI() {
    if (_container) {
        lv_obj_del(_container);  // Deletes container and ALL children
        _container = nullptr;
    }
    _playhead = nullptr;
    _bpmLabel = nullptr;
    _shapePanel = nullptr;
    _shapePanelTitle = nullptr;
    _modeToggleBtn = nullptr;
    _modePointsBtn = nullptr;
    _modeRotateBtn = nullptr;
    for (auto& ring : _rings) {
        ring.dotObjects.clear();
        ring.labelObj = nullptr;
    }
    _arcObjects.clear();
    _shapePanelWidgets.clear();
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Initialization
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::init(lv_obj_t* parent, int centerX, int centerY, bool drawBackground) {
    _parent  = parent;
    _centerX = centerX;
    _centerY = centerY;

    // Main container
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, 800, 480);
    lv_obj_align(_container, LV_ALIGN_TOP_LEFT, 0, 0);
    if (drawBackground) {
        lv_obj_set_style_bg_color(_container, lv_color_hex(0x0a1428), 0);
    } else {
        lv_obj_set_style_bg_opa(_container, LV_OPA_TRANSP, 0);
    }
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_set_style_pad_all(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // BPM label at circle center
    _bpmLabel = lv_label_create(_container);
    lv_label_set_text_fmt(_bpmLabel, "%d", _bpm);
    lv_obj_set_style_text_color(_bpmLabel, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(_bpmLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(_bpmLabel, _centerX - 15, _centerY - 12);

    createPlayhead();
    createShapePanel();

    std::cout << "[PolyRhythmUI] Initialized at center ("
              << _centerX << ", " << _centerY << ")\n";
}


// ═══════════════════════════════════════════════════════════════════════════════
//  IPC data setters
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::addRing(const std::string& name, lv_color_t color, int radius) {
    PolyRingData ring;
    ring.name   = name;
    ring.color  = color;
    ring.radius = radius;
    _rings.push_back(std::move(ring));
    _selectedPoints.push_back(0);
    _blinkCountdown.emplace_back();   // empty vector, will be sized in rebuildFromData
    std::cout << "[PolyRhythmUI] addRing: " << name << " r=" << radius << "\n";
}

void PolyRhythmUI::setRingShapes(size_t ringIdx, const std::vector<ShapeInfo>& shapes) {
    if (ringIdx < _rings.size()) {
        _rings[ringIdx].shapes = shapes;
    }
}

void PolyRhythmUI::addRingShape(size_t ringIdx, const ShapeInfo& info) {
    if (ringIdx < _rings.size()) {
        _rings[ringIdx].shapes.push_back(info);
    }
}

void PolyRhythmUI::setRingTimeline(size_t ringIdx, const std::vector<double>& phases) {
    if (ringIdx < _rings.size()) {
        _rings[ringIdx].timelinePhases = phases;
        if (ringIdx < _selectedPoints.size()) {
            if (phases.empty()) {
                _selectedPoints[ringIdx] = 0;
            } else if (_selectedPoints[ringIdx] >= phases.size()) {
                _selectedPoints[ringIdx] = phases.size() - 1;
            }
        }
        std::cout << "[PolyRhythmUI] setRingTimeline[" << ringIdx << "]: "
                  << phases.size() << " points\n";
    }
}

void PolyRhythmUI::setRingActivePoints(size_t ringIdx, const std::vector<bool>& active) {
    if (ringIdx < _rings.size()) {
        _rings[ringIdx].activePoints = active;
        // Ensure size matches timeline
        _rings[ringIdx].activePoints.resize(_rings[ringIdx].timelinePhases.size(), false);
        if (ringIdx < _selectedPoints.size()) {
            const size_t pointCount = _rings[ringIdx].activePoints.size();
            if (pointCount == 0) {
                _selectedPoints[ringIdx] = 0;
            } else if (_selectedPoints[ringIdx] >= pointCount) {
                _selectedPoints[ringIdx] = pointCount - 1;
            }
        }
    }
}

void PolyRhythmUI::clearAllRings() {
    clearAllVisuals();
    _rings.clear();
    _selectedPoints.clear();
    _blinkCountdown.clear();
}

void PolyRhythmUI::setPlayheadPhase(double phase) {
    _targetAngle = static_cast<float>(phase * 360.0);
}

void PolyRhythmUI::togglePointLocal(size_t ringIdx, size_t pointIdx) {
    if (ringIdx < _rings.size() && pointIdx < _rings[ringIdx].activePoints.size()) {
        _rings[ringIdx].activePoints[pointIdx] =
            !_rings[ringIdx].activePoints[pointIdx];

        // Immediate visual feedback
        if (pointIdx < _rings[ringIdx].dotObjects.size()) {
            lv_obj_t* dot = _rings[ringIdx].dotObjects[pointIdx];
            if (_rings[ringIdx].activePoints[pointIdx]) {
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
            }
        }

        // Clear any active blink on this dot to avoid visual conflict
        if (ringIdx < _blinkCountdown.size() && pointIdx < _blinkCountdown[ringIdx].size()) {
            _blinkCountdown[ringIdx][pointIdx] = 0;
        }
    }
}

void PolyRhythmUI::setSelectedPoint(size_t ringIdx, size_t pointIdx) {
    if (ringIdx >= _rings.size() || ringIdx >= _selectedPoints.size()) {
        return;
    }

    const size_t pointCount = _rings[ringIdx].timelinePhases.size();
    if (pointCount == 0) {
        _selectedPoints[ringIdx] = 0;
        return;
    }

    _selectedPoints[ringIdx] = std::min(pointIdx, pointCount - 1);
}

size_t PolyRhythmUI::getSelectedPoint(size_t ringIdx) const {
    if (ringIdx < _selectedPoints.size()) {
        return _selectedPoints[ringIdx];
    }
    return 0;
}

void PolyRhythmUI::selectRing(size_t ringIdx) {
    if (ringIdx >= _rings.size()) return;
    
    // Deselect old label highlight
    if (_selectedRing < _rings.size() && _rings[_selectedRing].labelObj) {
        lv_obj_set_style_bg_opa(_rings[_selectedRing].labelObj, LV_OPA_TRANSP, 0);
    }
    
    _selectedRing = ringIdx;
    if (_selectedRing < _selectedPoints.size()) {
        const size_t pointCount = _rings[_selectedRing].timelinePhases.size();
        if (pointCount == 0) {
            _selectedPoints[_selectedRing] = 0;
        } else if (_selectedPoints[_selectedRing] >= pointCount) {
            _selectedPoints[_selectedRing] = pointCount - 1;
        }
    }
    
    // Highlight selected label
    if (_rings[_selectedRing].labelObj) {
        lv_obj_set_style_bg_opa(_rings[_selectedRing].labelObj, LV_OPA_30, 0);
        lv_obj_set_style_bg_color(_rings[_selectedRing].labelObj, _rings[_selectedRing].color, 0);
    }
    
    rebuildShapePanel();
    std::cout << "[PolyRhythmUI] Selected ring: " << _rings[ringIdx].name << "\n";
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Rebuild from local data
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::rebuildFromData() {
    if (!_container) return;

    clearAllVisuals();

    if (_selectedPoints.size() < _rings.size()) {
        _selectedPoints.resize(_rings.size(), 0);
    }

    // Resize blink countdown arrays to match ring/timeline sizes
    _blinkCountdown.resize(_rings.size());

    // Create arcs + dots for each ring
    for (size_t i = 0; i < _rings.size(); ++i) {
        createArc(_rings[i]);
        createDotsForRing(i);
        // Size blink array to match dot count, zero-initialized (no active blinks)
        _blinkCountdown[i].assign(_rings[i].timelinePhases.size(), 0);

        const size_t pointCount = _rings[i].timelinePhases.size();
        if (pointCount == 0) {
            _selectedPoints[i] = 0;
        } else if (_selectedPoints[i] >= pointCount) {
            _selectedPoints[i] = pointCount - 1;
        }
    }

    // Ensure selection is valid
    if (_selectedRing >= _rings.size() && !_rings.empty())
        _selectedRing = 0;

    // Highlight selected ring label
    if (_selectedRing < _rings.size() && _rings[_selectedRing].labelObj) {
        lv_obj_set_style_bg_opa(_rings[_selectedRing].labelObj, LV_OPA_30, 0);
        lv_obj_set_style_bg_color(_rings[_selectedRing].labelObj, _rings[_selectedRing].color, 0);
    }

    rebuildShapePanel();

    std::cout << "[PolyRhythmUI] rebuildFromData: " << _rings.size() << " rings\n";
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Arc (circle ring)
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::createArc(PolyRingData& ring) {
    if (!_container) return;

    lv_obj_t* arc = lv_arc_create(_container);
    int arcSize = ring.radius * 2;
    lv_obj_set_size(arc, arcSize, arcSize);
    lv_obj_set_pos(arc, _centerX - ring.radius, _centerY - ring.radius);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_width(arc, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, ring.color, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);  // let container opa handle dimming
    lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);             // prevent default widget bg fill
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    _arcObjects.push_back(arc);

    // Clickable name label — positioned in top-left, arranged by ring index
    lv_obj_t* label = lv_label_create(_container);
    lv_label_set_text(label, ring.name.c_str());
    lv_obj_set_style_text_color(label, ring.color, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    // Position will be set after we know ring index
    lv_obj_set_style_pad_all(label, 4, 0);
    lv_obj_set_style_radius(label, 4, 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);

    // Store ring index in user_data
    size_t ringIdx = _arcObjects.size() - 1;  // index = number of arcs created so far - 1
    // Find ring index by name
    for (size_t i = 0; i < _rings.size(); ++i) {
        if (&_rings[i] == &ring) { ringIdx = i; break; }
    }
    
    // Position label in top-left corner, stacked vertically by ring order
    // Rings are ordered largest to smallest radius, so ring 0 is at top
    int labelY = 10 + static_cast<int>(ringIdx) * 25;
    lv_obj_set_pos(label, 10, labelY);
    
    lv_obj_set_user_data(label, reinterpret_cast<void*>(static_cast<uintptr_t>(ringIdx)));
    lv_obj_add_event_cb(label, onRingLabelClicked, LV_EVENT_CLICKED, this);

    ring.labelObj = label;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Dots — placed at angles from per-ring timeline phases
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::createDotsForRing(size_t ringIdx) {
    if (!_container || ringIdx >= _rings.size()) return;

    auto& ring = _rings[ringIdx];
    ring.dotObjects.clear();
    if (ring.timelinePhases.empty()) return;

    ring.dotObjects.reserve(ring.timelinePhases.size());

    for (size_t pi = 0; pi < ring.timelinePhases.size(); ++pi) {
        double angleDeg = ring.timelinePhases[pi] * 360.0 - 90.0;
        lv_point_t pos  = pointOnCircle(ring.radius, angleDeg);

        lv_obj_t* dot = lv_obj_create(_container);
        lv_obj_set_size(dot, 14, 14);
        lv_obj_set_pos(dot, pos.x - 7, pos.y - 7);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, ring.color, 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        lv_obj_set_style_border_color(dot, ring.color, 0);
        lv_obj_set_style_outline_width(dot, 0, 0);
        lv_obj_set_style_outline_opa(dot, LV_OPA_TRANSP, 0);

        bool active = (pi < ring.activePoints.size()) && ring.activePoints[pi];
        if (active) {
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
        }

        // Pack ringIdx + pointIdx
        lv_obj_add_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        uintptr_t userData = (static_cast<uintptr_t>(ringIdx) << 16) | (pi & 0xFFFF);
        lv_obj_set_user_data(dot, reinterpret_cast<void*>(userData));
        lv_obj_add_event_cb(dot, onDotClicked, LV_EVENT_CLICKED, this);

        ring.dotObjects.push_back(dot);
    }
}

void PolyRhythmUI::clearDotsForRing(size_t ringIdx) {
    if (ringIdx >= _rings.size()) return;
    for (auto* obj : _rings[ringIdx].dotObjects) {
        if (obj) lv_obj_del(obj);
    }
    _rings[ringIdx].dotObjects.clear();
}

void PolyRhythmUI::clearAllVisuals() {
    // Delete dots
    for (size_t i = 0; i < _rings.size(); ++i) {
        for (auto* obj : _rings[i].dotObjects) {
            if (obj) lv_obj_del(obj);
        }
        _rings[i].dotObjects.clear();
        // Delete label
        if (_rings[i].labelObj) {
            lv_obj_del(_rings[i].labelObj);
            _rings[i].labelObj = nullptr;
        }
    }
    // Delete arcs
    for (auto* arc : _arcObjects) {
        if (arc) lv_obj_del(arc);
    }
    _arcObjects.clear();
    // Clear shape panel widgets
    for (auto* w : _shapePanelWidgets) {
        if (w) lv_obj_del(w);
    }
    _shapePanelWidgets.clear();
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Playhead
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::createPlayhead() {
    _playhead = lv_line_create(_container);
    lv_obj_set_style_line_color(_playhead, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_line_width(_playhead, 3, 0);
    lv_obj_set_style_line_opa(_playhead, LV_OPA_80, 0);

    _playheadPoints[0].x = static_cast<lv_value_precise_t>(_centerX);
    _playheadPoints[0].y = static_cast<lv_value_precise_t>(_centerY);
    _playheadPoints[1].x = static_cast<lv_value_precise_t>(_centerX);
    _playheadPoints[1].y = static_cast<lv_value_precise_t>(_centerY - 150);
    lv_line_set_points(_playhead, _playheadPoints, 2);
}

void PolyRhythmUI::updatePlayhead() {
    if (!_playhead) return;
    float forwardStepDeg = 0.0f;

    if (_isPlaying) {
        float diff = _targetAngle - _playheadAngle;
        while (diff >  180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        forwardStepDeg = diff * 0.3f;
        _playheadAngle = normalizeAngle(_playheadAngle + forwardStepDeg);
    }

    int maxRadius = 150;
    for (const auto& r : _rings) {
        if (r.radius > maxRadius) maxRadius = r.radius;
    }
    maxRadius += 15;

    lv_point_t endPt = pointOnCircle(maxRadius, static_cast<double>(_playheadAngle) - 90.0);

    _playheadPoints[0].x = static_cast<lv_value_precise_t>(_centerX);
    _playheadPoints[0].y = static_cast<lv_value_precise_t>(_centerY);
    _playheadPoints[1].x = static_cast<lv_value_precise_t>(endPt.x);
    _playheadPoints[1].y = static_cast<lv_value_precise_t>(endPt.y);
    lv_line_set_points(_playhead, _playheadPoints, 2);

    // Trigger blink when the rendered playhead crosses an active point.
    // This keeps blink strictly tied to UI needle position (not audio trigger timing).
    if (!_isPlaying || !_hasPrevPlayhead) {
        _prevPlayheadAngle = _playheadAngle;
        _hasPrevPlayhead = true;
        return;
    }

    // Ignore backward or near-zero movement (can happen on jitter / pause transitions)
    if (forwardStepDeg <= 0.001f) {
        _prevPlayheadAngle = _playheadAngle;
        return;
    }

    // Guard against large jumps (e.g. clock hiccup) to avoid mass false triggers.
    float traveled = cwDistance(_prevPlayheadAngle, _playheadAngle);
    if (traveled > 90.0f) {
        _prevPlayheadAngle = _playheadAngle;
        return;
    }

    constexpr float CROSS_EPS_DEG = 0.8f;
    for (size_t ringIdx = 0; ringIdx < _rings.size(); ++ringIdx) {
        const auto& ring = _rings[ringIdx];
        const size_t n = std::min(ring.timelinePhases.size(), ring.activePoints.size());

        for (size_t pointIdx = 0; pointIdx < n; ++pointIdx) {
            if (!ring.activePoints[pointIdx]) continue;

            float pointAngle = normalizeAngle(static_cast<float>(ring.timelinePhases[pointIdx] * 360.0));
            float offset = cwDistance(_prevPlayheadAngle, pointAngle);

            // Trigger once when point falls inside the segment swept this frame.
            if (offset > 0.0f && offset <= (traveled + CROSS_EPS_DEG)) {
                triggerBlink(ringIdx, pointIdx);
            }
        }
    }

    _prevPlayheadAngle = _playheadAngle;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Shape control panel (right side)
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::createShapePanel() {
    if (!_container) return;

    _shapePanel = lv_obj_create(_container);
    lv_obj_set_size(_shapePanel, 200, 440);
    lv_obj_set_pos(_shapePanel, 590, 20);
    lv_obj_set_style_bg_color(_shapePanel, lv_color_hex(0x1a2a44), 0);
    lv_obj_set_style_border_width(_shapePanel, 1, 0);
    lv_obj_set_style_border_color(_shapePanel, lv_color_hex(0x2a4a6a), 0);
    lv_obj_set_style_pad_all(_shapePanel, 6, 0);
    lv_obj_set_flex_flow(_shapePanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_shapePanel, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(_shapePanel, 4, 0);

    // Title — shows selected ring name
    _shapePanelTitle = lv_label_create(_shapePanel);
    lv_label_set_text(_shapePanelTitle, "SHAPES");
    lv_obj_set_style_text_color(_shapePanelTitle, lv_color_hex(0xccddff), 0);
    lv_obj_set_style_text_font(_shapePanelTitle, &lv_font_montserrat_14, 0);

    // Hardware mode selector (right section)
    lv_obj_t* modeTitle = lv_label_create(_shapePanel);
    lv_label_set_text(modeTitle, "HW MODE");
    lv_obj_set_style_text_color(modeTitle, lv_color_hex(0x88aacc), 0);
    lv_obj_set_style_text_font(modeTitle, &lv_font_montserrat_14, 0);

    lv_obj_t* modeRow = lv_obj_create(_shapePanel);
    lv_obj_set_size(modeRow, 180, 30);
    lv_obj_set_style_bg_opa(modeRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(modeRow, 0, 0);
    lv_obj_set_style_pad_all(modeRow, 0, 0);
    lv_obj_set_flex_flow(modeRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(modeRow, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _modeToggleBtn = lv_btn_create(modeRow);
    lv_obj_set_size(_modeToggleBtn, 56, 26);
    lv_obj_add_event_cb(_modeToggleBtn, onModeToggle, LV_EVENT_CLICKED, this);
    lv_obj_t* modeToggleLbl = lv_label_create(_modeToggleBtn);
    lv_label_set_text(modeToggleLbl, "TGL");
    lv_obj_center(modeToggleLbl);
    lv_obj_set_style_text_color(modeToggleLbl, lv_color_hex(0xffffff), 0);

    _modePointsBtn = lv_btn_create(modeRow);
    lv_obj_set_size(_modePointsBtn, 56, 26);
    lv_obj_add_event_cb(_modePointsBtn, onModePoints, LV_EVENT_CLICKED, this);
    lv_obj_t* modePointsLbl = lv_label_create(_modePointsBtn);
    lv_label_set_text(modePointsLbl, "PTS");
    lv_obj_center(modePointsLbl);
    lv_obj_set_style_text_color(modePointsLbl, lv_color_hex(0xffffff), 0);

    _modeRotateBtn = lv_btn_create(modeRow);
    lv_obj_set_size(_modeRotateBtn, 56, 26);
    lv_obj_add_event_cb(_modeRotateBtn, onModeRotate, LV_EVENT_CLICKED, this);
    lv_obj_t* modeRotateLbl = lv_label_create(_modeRotateBtn);
    lv_label_set_text(modeRotateLbl, "ROT");
    lv_obj_center(modeRotateLbl);
    lv_obj_set_style_text_color(modeRotateLbl, lv_color_hex(0xffffff), 0);

    refreshModeButtons();

    // "Add Shape" button
    lv_obj_t* btnAdd = lv_btn_create(_shapePanel);
    lv_obj_set_size(btnAdd, 180, 30);
    lv_obj_set_style_bg_color(btnAdd, lv_color_hex(0x2266aa), 0);
    lv_obj_add_event_cb(btnAdd, onAddShape, LV_EVENT_CLICKED, this);

    lv_obj_t* btnLabel = lv_label_create(btnAdd);
    lv_label_set_text(btnLabel, "+ Add Shape");
    lv_obj_center(btnLabel);
    lv_obj_set_style_text_color(btnLabel, lv_color_hex(0xffffff), 0);

    // Rotate buttons row
    lv_obj_t* rotRow = lv_obj_create(_shapePanel);
    lv_obj_set_size(rotRow, 180, 30);
    lv_obj_set_style_bg_opa(rotRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rotRow, 0, 0);
    lv_obj_set_style_pad_all(rotRow, 0, 0);
    lv_obj_set_flex_flow(rotRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rotRow, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btnRotL = lv_btn_create(rotRow);
    lv_obj_set_size(btnRotL, 80, 26);
    lv_obj_set_style_bg_color(btnRotL, lv_color_hex(0x335577), 0);
    lv_obj_add_event_cb(btnRotL, onRotateLeft, LV_EVENT_CLICKED, this);
    lv_obj_t* rlbl = lv_label_create(btnRotL);
    lv_label_set_text(rlbl, "Rot <");
    lv_obj_center(rlbl);
    lv_obj_set_style_text_color(rlbl, lv_color_hex(0xffffff), 0);

    lv_obj_t* btnRotR = lv_btn_create(rotRow);
    lv_obj_set_size(btnRotR, 80, 26);
    lv_obj_set_style_bg_color(btnRotR, lv_color_hex(0x335577), 0);
    lv_obj_add_event_cb(btnRotR, onRotateRight, LV_EVENT_CLICKED, this);
    lv_obj_t* rrlbl = lv_label_create(btnRotR);
    lv_label_set_text(rrlbl, "Rot >");
    lv_obj_center(rrlbl);
    lv_obj_set_style_text_color(rrlbl, lv_color_hex(0xffffff), 0);
}

void PolyRhythmUI::rebuildShapePanel() {
    // Remove old dynamic shape widgets
    for (auto* w : _shapePanelWidgets) {
        if (w) lv_obj_del(w);
    }
    _shapePanelWidgets.clear();

    if (!_shapePanel || _rings.empty()) return;

    // Update panel title
    if (_selectedRing < _rings.size() && _shapePanelTitle) {
        std::string title = "SHAPES: " + _rings[_selectedRing].name;
        lv_label_set_text(_shapePanelTitle, title.c_str());
    }

    // Show shapes for the selected ring
    if (_selectedRing >= _rings.size()) return;
    const auto& shapes = _rings[_selectedRing].shapes;

    for (size_t i = 0; i < shapes.size(); ++i) {
        const auto& info = shapes[i];

        // Row container for this shape
        lv_obj_t* row = lv_obj_create(_shapePanel);
        lv_obj_set_size(row, 180, 55);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x223355), 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 3, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(row, 2, 0);
        _shapePanelWidgets.push_back(row);

        // Shape info label
        lv_obj_t* label = lv_label_create(row);
        std::string text = info.name + "  n=" + std::to_string(info.subdivision);
        if (info.euclideanHits > 0) {
            text += "  E(" + std::to_string(info.euclideanHits) + ")";
        }
        lv_label_set_text(label, text.c_str());
        lv_obj_set_style_text_color(label, lv_color_hex(0xaaccff), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

        // Button row: [ − ] [ + ]  [ DEL ]
        lv_obj_t* btnRow = lv_obj_create(row);
        lv_obj_set_size(btnRow, 170, 24);
        lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btnRow, 0, 0);
        lv_obj_set_style_pad_all(btnRow, 0, 0);
        lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_EVENLY,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Pack shapeIdx in user_data for each button
        uintptr_t shapeIdx = static_cast<uintptr_t>(i);

        // "−" subdivision button
        lv_obj_t* btnMinus = lv_btn_create(btnRow);
        lv_obj_set_size(btnMinus, 40, 22);
        lv_obj_set_style_bg_color(btnMinus, lv_color_hex(0x444466), 0);
        lv_obj_set_user_data(btnMinus, reinterpret_cast<void*>(shapeIdx));
        lv_obj_add_event_cb(btnMinus, onSubdivDown, LV_EVENT_CLICKED, this);
        lv_obj_t* mLbl = lv_label_create(btnMinus);
        lv_label_set_text(mLbl, "n-");
        lv_obj_center(mLbl);
        lv_obj_set_style_text_color(mLbl, lv_color_hex(0xffffff), 0);

        // "+" subdivision button
        lv_obj_t* btnPlus = lv_btn_create(btnRow);
        lv_obj_set_size(btnPlus, 40, 22);
        lv_obj_set_style_bg_color(btnPlus, lv_color_hex(0x444466), 0);
        lv_obj_set_user_data(btnPlus, reinterpret_cast<void*>(shapeIdx));
        lv_obj_add_event_cb(btnPlus, onSubdivUp, LV_EVENT_CLICKED, this);
        lv_obj_t* pLbl = lv_label_create(btnPlus);
        lv_label_set_text(pLbl, "n+");
        lv_obj_center(pLbl);
        lv_obj_set_style_text_color(pLbl, lv_color_hex(0xffffff), 0);

        // "DEL" button
        lv_obj_t* btnDel = lv_btn_create(btnRow);
        lv_obj_set_size(btnDel, 50, 22);
        lv_obj_set_style_bg_color(btnDel, lv_color_hex(0x882222), 0);
        lv_obj_set_user_data(btnDel, reinterpret_cast<void*>(shapeIdx));
        lv_obj_add_event_cb(btnDel, onRemoveShape, LV_EVENT_CLICKED, this);
        lv_obj_t* dLbl = lv_label_create(btnDel);
        lv_label_set_text(dLbl, "DEL");
        lv_obj_center(dLbl);
        lv_obj_set_style_text_color(dLbl, lv_color_hex(0xffffff), 0);
    }

    // Timeline summary for this ring
    lv_obj_t* summary = lv_label_create(_shapePanel);
    size_t nPts = (_selectedRing < _rings.size()) ? _rings[_selectedRing].timelinePhases.size() : 0;
    std::string sumText = "Timeline: " + std::to_string(nPts) + " pts";
    lv_label_set_text(summary, sumText.c_str());
    lv_obj_set_style_text_color(summary, lv_color_hex(0x88aacc), 0);
    lv_obj_set_style_text_font(summary, &lv_font_montserrat_14, 0);
    _shapePanelWidgets.push_back(summary);
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Update (called each LVGL frame)
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::update() {
    const auto now = std::chrono::steady_clock::now();
    uint32_t dtMs = 16; // default fallback (~60 FPS)
    if (_hasLastBlinkTick) {
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastBlinkTick).count();
        if (dt < 0) dt = 0;
        if (dt > 100) dt = 100; // clamp large spikes to avoid visual jumps
        dtMs = static_cast<uint32_t>(dt);
    } else {
        _hasLastBlinkTick = true;
    }
    _lastBlinkTick = now;

    updatePlayhead();
    updateBlinks(dtMs);
    updateDots();
}

void PolyRhythmUI::updateDots() {
    for (size_t ri = 0; ri < _rings.size(); ++ri) {
        const auto& ring = _rings[ri];
        for (size_t pi = 0; pi < ring.dotObjects.size() && pi < ring.activePoints.size(); ++pi) {
            const bool isSelected = (ri == _selectedRing)
                                 && (ri < _selectedPoints.size())
                                 && (pi == _selectedPoints[ri]);

            if (isSelected) {
                lv_obj_set_style_outline_width(ring.dotObjects[pi], 3, 0);
                lv_obj_set_style_outline_color(ring.dotObjects[pi], lv_color_hex(0xffffff), 0);
                lv_obj_set_style_outline_opa(ring.dotObjects[pi], LV_OPA_90, 0);
                lv_obj_set_style_border_width(ring.dotObjects[pi], 3, 0);
            } else {
                lv_obj_set_style_outline_width(ring.dotObjects[pi], 0, 0);
                lv_obj_set_style_outline_opa(ring.dotObjects[pi], LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(ring.dotObjects[pi], 2, 0);
                lv_obj_set_style_border_color(ring.dotObjects[pi], ring.color, 0);
            }

            // Skip dots that are currently blinking — blink takes priority
            if (ri < _blinkCountdown.size() && pi < _blinkCountdown[ri].size()
                && _blinkCountdown[ri][pi] > 0) {
                continue;
            }
            if (ring.activePoints[pi]) {
                lv_obj_set_style_bg_opa(ring.dotObjects[pi], LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_opa(ring.dotObjects[pi], LV_OPA_TRANSP, 0);
            }
        }
    }
}

void PolyRhythmUI::triggerBlink(size_t ringIdx, size_t pointIdx) {
    if (ringIdx >= _blinkCountdown.size()) return;
    if (pointIdx >= _blinkCountdown[ringIdx].size()) return;
    // Reset timer — if already blinking, restart full effect
    _blinkCountdown[ringIdx][pointIdx] = BLINK_DURATION_MS;
}

void PolyRhythmUI::updateBlinks(uint32_t dtMs) {
    for (size_t ri = 0; ri < _blinkCountdown.size() && ri < _rings.size(); ++ri) {
        const auto& ring = _rings[ri];
        auto& countdowns = _blinkCountdown[ri];

        for (size_t pi = 0; pi < countdowns.size(); ++pi) {
            if (countdowns[pi] == 0) continue;
            if (pi >= ring.dotObjects.size()) continue;

            lv_obj_t* dot = ring.dotObjects[pi];
            uint16_t remaining = countdowns[pi];
            uint16_t elapsed = (BLINK_DURATION_MS > remaining) ? (BLINK_DURATION_MS - remaining) : 0;

            // Blink phases:
            //   [0 .. BLINK_PEAK_MS)              -> peak flash (white, enlarged)
            //   [BLINK_PEAK_MS .. BLINK_DURATION) -> fade back (ring color, normal size)
            if (elapsed < BLINK_PEAK_MS) {
                // ── Peak phase: white flash + slight enlarge ────────────
                lv_obj_set_style_bg_color(dot, lv_color_hex(0xffffff), 0);
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                lv_obj_set_style_border_color(dot, lv_color_hex(0xffffff), 0);
                lv_obj_set_style_transform_width(dot, 3, 0);   // +3px each side
                lv_obj_set_style_transform_height(dot, 3, 0);
            } else {
                // ── Fade phase: transition back to ring color ───────────
                // Compute blend factor: 1.0 (start of fade) → 0.0 (end)
                const float denom = static_cast<float>(BLINK_DURATION_MS - BLINK_PEAK_MS);
                const float fadeElapsed = static_cast<float>(elapsed - BLINK_PEAK_MS);
                float t = 1.0f - (fadeElapsed / denom);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                // Blend white → ring color
                uint8_t r = static_cast<uint8_t>(255 * t + ring.color.red * (1.0f - t));
                uint8_t g = static_cast<uint8_t>(255 * t + ring.color.green * (1.0f - t));
                uint8_t b = static_cast<uint8_t>(255 * t + ring.color.blue * (1.0f - t));
                lv_obj_set_style_bg_color(dot, lv_color_make(r, g, b), 0);
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                lv_obj_set_style_border_color(dot, ring.color, 0);

                // Shrink back: transform proportional to t
                int grow = static_cast<int>(3.0f * t);
                lv_obj_set_style_transform_width(dot, grow, 0);
                lv_obj_set_style_transform_height(dot, grow, 0);
            }

            if (remaining <= dtMs) {
                countdowns[pi] = 0;
            } else {
                countdowns[pi] = static_cast<uint16_t>(remaining - dtMs);
            }

            // ── Blink finished: restore original style ──────────────────
            if (countdowns[pi] == 0) {
                lv_obj_set_style_bg_color(dot, ring.color, 0);
                lv_obj_set_style_border_color(dot, ring.color, 0);
                lv_obj_set_style_transform_width(dot, 0, 0);
                lv_obj_set_style_transform_height(dot, 0, 0);
                // Restore correct opacity based on active state
                if (pi < ring.activePoints.size() && ring.activePoints[pi]) {
                    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                } else {
                    lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
                }
            }
        }
    }
}

void PolyRhythmUI::setBPM(int bpm) {
    _bpm = bpm;
    if (_bpmLabel) lv_label_set_text_fmt(_bpmLabel, "%d", bpm);
}

void PolyRhythmUI::setPlaying(bool playing) {
    _isPlaying = playing;
    _hasPrevPlayhead = false;
    _hasLastBlinkTick = false;
    std::cout << "[PolyRhythmUI] Playing: " << (playing ? "YES" : "NO") << "\n";
}

void PolyRhythmUI::setHardwareMode(const std::string& mode) {
    std::string normalized = mode;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized == "toggle" || normalized == "toggle_point" || normalized == "activation") {
        _hardwareMode = "toggle";
    } else if (normalized == "points" || normalized == "add_remove" || normalized == "addremove") {
        _hardwareMode = "points";
    } else {
        _hardwareMode = "rotate";
    }

    refreshModeButtons();
}

void PolyRhythmUI::refreshModeButtons() {
    auto styleBtn = [](lv_obj_t* btn, bool active) {
        if (!btn) return;
        lv_obj_set_style_bg_color(btn,
            active ? lv_color_hex(0x2f8f4f) : lv_color_hex(0x334a66), 0);
        lv_obj_set_style_border_width(btn, active ? 2 : 1, 0);
        lv_obj_set_style_border_color(btn,
            active ? lv_color_hex(0x9ad6aa) : lv_color_hex(0x56708f), 0);
    };

    styleBtn(_modeToggleBtn, _hardwareMode == "toggle");
    styleBtn(_modePointsBtn, _hardwareMode == "points");
    styleBtn(_modeRotateBtn, _hardwareMode == "rotate");
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Carousel display control
// ═══════════════════════════════════════════════════════════════════════════════

const std::vector<bool>& PolyRhythmUI::getRingActivePoints(size_t ringIdx) const {
    static const std::vector<bool> empty;
    if (ringIdx < _rings.size()) return _rings[ringIdx].activePoints;
    return empty;
}

void PolyRhythmUI::setInteractive(bool interactive) {
    _interactive = interactive;
}

void PolyRhythmUI::setContainerXOffset(int xOffset) {
    if (_container) {
        lv_obj_set_x(_container, xOffset);
    }
}

void PolyRhythmUI::setContainerOpacity(lv_opa_t opa) {
    if (_container) {
        lv_obj_set_style_opa(_container, opa, 0);
    }
}

void PolyRhythmUI::setContainerScalePct(uint16_t /*scalePct*/) {
    // Intentionally a no-op.
    // Applying lv_obj_set_style_transform_zoom to an 800×480 container forces
    // LVGL to software-render every child through a per-pixel bilinear transform.
    // On the SDL software backend this freezes the renderer completely.
    // Visual depth is already conveyed through opacity + X-offset.
}

void PolyRhythmUI::setShapePanelVisible(bool visible) {
    if (_shapePanel) {
        if (visible) lv_obj_clear_flag(_shapePanel, LV_OBJ_FLAG_HIDDEN);
        else         lv_obj_add_flag(_shapePanel, LV_OBJ_FLAG_HIDDEN);
    }
}

void PolyRhythmUI::setBpmLabelVisible(bool visible) {
    if (_bpmLabel) {
        if (visible) lv_obj_clear_flag(_bpmLabel, LV_OBJ_FLAG_HIDDEN);
        else         lv_obj_add_flag(_bpmLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void PolyRhythmUI::setRingLabelsVisible(bool visible) {
    for (auto& ring : _rings) {
        if (!ring.labelObj) continue;
        if (visible) lv_obj_clear_flag(ring.labelObj, LV_OBJ_FLAG_HIDDEN);
        else         lv_obj_add_flag(ring.labelObj, LV_OBJ_FLAG_HIDDEN);
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Geometry
// ═══════════════════════════════════════════════════════════════════════════════

lv_point_t PolyRhythmUI::pointOnCircle(int radius, double angleDeg) const {
    double rad = angleDeg * M_PI / 180.0;
    lv_point_t pt;
    pt.x = _centerX + static_cast<int>(radius * cos(rad));
    pt.y = _centerY + static_cast<int>(radius * sin(rad));
    return pt;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  LVGL callbacks
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmUI::onDotClicked(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    lv_obj_t* dot = static_cast<lv_obj_t*>(lv_event_get_target(e));
    uintptr_t packed = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(dot));
    size_t ringIdx  = (packed >> 16) & 0xFFFF;
    size_t pointIdx = packed & 0xFFFF;

    self->setSelectedPoint(ringIdx, pointIdx);
    self->togglePointLocal(ringIdx, pointIdx);

    if (self->_onPointToggle)
        self->_onPointToggle(ringIdx, pointIdx);

    std::cout << "[PolyRhythmUI] Dot clicked: ring=" << ringIdx
              << " point=" << pointIdx << "\n";
}

void PolyRhythmUI::onRingLabelClicked(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    lv_obj_t* label = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    uintptr_t idx = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(label));
    const size_t ringIdx = static_cast<size_t>(idx);
    self->selectRing(ringIdx);
    if (self->_onRingSelect) {
        self->_onRingSelect(ringIdx);
    }
}

void PolyRhythmUI::onAddShape(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    // Add a shape with default subdivision=4 to the selected ring
    if (self->_onShapeAdd)
        self->_onShapeAdd(self->_selectedRing, 4, 0.0);

    std::cout << "[PolyRhythmUI] Add shape to ring " << self->_selectedRing << "\n";
}

void PolyRhythmUI::onRemoveShape(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    // Use current_target to get the button (not a child label)
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    uintptr_t shapeIdx = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn));

    if (self->_onShapeRemove)
        self->_onShapeRemove(self->_selectedRing, static_cast<size_t>(shapeIdx));

    std::cout << "[PolyRhythmUI] Remove shape " << shapeIdx
              << " from ring " << self->_selectedRing << "\n";
}

void PolyRhythmUI::onSubdivUp(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    uintptr_t shapeIdx = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn));
    size_t si = static_cast<size_t>(shapeIdx);

    if (self->_selectedRing < self->_rings.size() &&
        si < self->_rings[self->_selectedRing].shapes.size()) {
        auto& shape = self->_rings[self->_selectedRing].shapes[si];
        uint32_t newSubdiv = shape.subdivision + 1;
        if (newSubdiv > 32) newSubdiv = 32;  // cap

        if (self->_onShapeModify)
            self->_onShapeModify(self->_selectedRing, si, newSubdiv, shape.offset);
    }
}

void PolyRhythmUI::onSubdivDown(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    uintptr_t shapeIdx = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn));
    size_t si = static_cast<size_t>(shapeIdx);

    if (self->_selectedRing < self->_rings.size() &&
        si < self->_rings[self->_selectedRing].shapes.size()) {
        auto& shape = self->_rings[self->_selectedRing].shapes[si];
        uint32_t newSubdiv = (shape.subdivision > 2) ? shape.subdivision - 1 : 2;

        if (self->_onShapeModify)
            self->_onShapeModify(self->_selectedRing, si, newSubdiv, shape.offset);
    }
}

void PolyRhythmUI::onRotateLeft(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    // Rot < rotates counter-clockwise → positive rotation
    if (self->_onRotate)
        self->_onRotate(self->_selectedRing, 1);
}

void PolyRhythmUI::onRotateRight(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    // Rot > rotates clockwise → negative rotation
    if (self->_onRotate)
        self->_onRotate(self->_selectedRing, -1);
}

void PolyRhythmUI::onModeToggle(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    self->setHardwareMode("toggle");
    if (self->_onModeChange) {
        self->_onModeChange("toggle");
    }
}

void PolyRhythmUI::onModePoints(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    self->setHardwareMode("points");
    if (self->_onModeChange) {
        self->_onModeChange("points");
    }
}

void PolyRhythmUI::onModeRotate(lv_event_t* e) {
    auto* self = static_cast<PolyRhythmUI*>(lv_event_get_user_data(e));
    if (!self || !self->_interactive) return;

    self->setHardwareMode("rotate");
    if (self->_onModeChange) {
        self->_onModeChange("rotate");
    }
}

} // namespace GUI