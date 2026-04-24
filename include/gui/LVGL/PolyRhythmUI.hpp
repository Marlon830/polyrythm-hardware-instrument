/**
 * @file PolyRhythmUI.hpp
 * @brief LVGL-based circular UI for the polyrhythmic sequencer.
 *
 * @ingroup gui
 *
 * ── Visual design ───────────────────────────────────────────────────────────
 *
 *   Each instrument is a concentric ring (circle) with its OWN shapes and
 *   timeline. Points are placed at angle = phase × 360° starting from top (−90°).
 *   Because shapes have different subdivisions, points are NOT uniformly spaced.
 *
 *   Active point  → filled circle (full opacity)
 *   Inactive point → hollow ring  (border only, low opacity)
 *
 *   Clicking on a ring's name label selects that ring. The shape panel on the
 *   right shows shapes for the selected ring only, with controls to add, delete,
 *   and modify (change subdivision, rotate) each shape.
 *
 * ── IPC protocol (forked child process) ─────────────────────────────────────
 *
 *   Audio → GUI:
 *     POLY_RING:idx|name|color_hex|radius
 *     POLY_RING_SHAPE:ringIdx|name|subdivision|offset|euclideanHits
 *     POLY_RING_TIMELINE:ringIdx|p0,p1,...
 *     POLY_RING_ACTIVE:ringIdx|0,1,0,...
 *     POLY_SELECT:ringIdx (restore selected ring after rebuild)
 *     POLY_PLAYHEAD:phase|bpm
 *     POLY_READY
 *
 *   GUI → Audio:
 *     POLY_TOGGLE:ringIdx|pointIdx
 *     POLY_ADD_SHAPE:ringIdx|subdivision|offset
 *     POLY_REMOVE_SHAPE:ringIdx|shapeIdx
 *     POLY_MOD_SHAPE:ringIdx|shapeIdx|newSubdiv|newOffset
 *     POLY_ROTATE:ringIdx|amount
 */

#ifndef POLYRHYTHM_UI_HPP
#define POLYRHYTHM_UI_HPP

#include "lvgl/lvgl.h"

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <cstddef>
#include <chrono>

namespace GUI {

/// @brief Shape summary for display in control panel.
struct ShapeInfo {
    std::string name;
    uint32_t    subdivision   = 4;
    double      offset        = 0.0;
    uint32_t    euclideanHits = 0;
};

/// @brief Per-ring data — each ring has its own timeline, shapes, and active points.
struct PolyRingData {
    std::string              name;
    lv_color_t               color;
    int                      radius;           ///< Distance from center in px
    std::vector<double>      timelinePhases;   ///< Own merged timeline
    std::vector<bool>        activePoints;     ///< One bool per timeline point
    std::vector<ShapeInfo>   shapes;           ///< Own shape list
    std::vector<lv_obj_t*>   dotObjects;       ///< LVGL widgets for dots
    lv_obj_t*                labelObj = nullptr;///< Ring name label (clickable)
};

/// @brief Callback types — sent back to audio engine via IPC.
/// All callbacks include ringIdx to identify which instrument.
using PointToggleCallback  = std::function<void(size_t ringIdx, size_t pointIdx)>;
using ShapeAddCallback     = std::function<void(size_t ringIdx, uint32_t subdivision, double offset)>;
using ShapeRemoveCallback  = std::function<void(size_t ringIdx, size_t shapeIdx)>;
using ShapeModifyCallback  = std::function<void(size_t ringIdx, size_t shapeIdx, uint32_t newSubdiv, double newOffset)>;
using RotateCallback       = std::function<void(size_t ringIdx, int amount)>;
using RingSelectCallback   = std::function<void(size_t ringIdx)>;
using ModeChangeCallback   = std::function<void(const std::string& mode)>;

/// @brief LVGL-based circular polyrhythmic sequencer UI.
///
/// Each ring = one instrument with its own PolyRhythmModule, own shapes, own timeline.
/// The shape panel shows controls for the currently *selected* ring.
class PolyRhythmUI {
public:
    PolyRhythmUI();
    ~PolyRhythmUI();

    /// @brief Initialize the UI container, playhead, and shape panel.
    /// @param drawBackground If true, container has dark background; false for transparent (carousel side sets).
    void init(lv_obj_t* parent, int centerX = 300, int centerY = 240, bool drawBackground = true);

    // ─── IPC data setters ────────────────────────────────────────────────────

    /// @brief Declare a ring (instrument). Called once per instrument at setup.
    void addRing(const std::string& name, lv_color_t color, int radius);

    /// @brief Set shapes for a given ring (replaces prior shapes).
    void setRingShapes(size_t ringIdx, const std::vector<ShapeInfo>& shapes);

    /// @brief Add a single shape to a ring.
    void addRingShape(size_t ringIdx, const ShapeInfo& info);

    /// @brief Set the merged timeline for a ring.
    void setRingTimeline(size_t ringIdx, const std::vector<double>& phases);

    /// @brief Set the active points for a ring.
    void setRingActivePoints(size_t ringIdx, const std::vector<bool>& active);

    /// @brief Clear all rings (before full rebuild).
    void clearAllRings();

    /// @brief Rebuild all visual elements from local ring data.
    void rebuildFromData();

    /// @brief Set playhead phase from IPC [0.0, 1.0).
    void setPlayheadPhase(double phase);

    /// @brief Toggle a point locally (immediate visual feedback).
    void togglePointLocal(size_t ringIdx, size_t pointIdx);

    /// @brief Set the currently selected point for one ring.
    void setSelectedPoint(size_t ringIdx, size_t pointIdx);

    /// @brief Trigger a blink animation on a dot.
    /// Safe to call multiple times per frame — resets countdown if already blinking.
    void triggerBlink(size_t ringIdx, size_t pointIdx);

    // ─── UI state ────────────────────────────────────────────────────────────

    void update();
    void setBPM(int bpm);
    void setPlaying(bool playing);
    void setHardwareMode(const std::string& mode);

    // ─── Carousel display control ─────────────────────────────────────────

    /// @brief Enable/disable user interaction (clicks on dots, labels, shape panel).
    void setInteractive(bool interactive);
    bool isInteractive() const { return _interactive; }

    /// @brief Set the X offset of this set's container for carousel positioning.
    void setContainerXOffset(int xOffset);

    /// @brief Set container opacity (dims all children). For side set previews.
    void setContainerOpacity(lv_opa_t opa);

    /// @brief Set container visual scale in percent (100 = normal size).
    void setContainerScalePct(uint16_t scalePct);

    /// @brief Show/hide the shape control panel (hidden on side sets).
    void setShapePanelVisible(bool visible);

    /// @brief Show/hide the BPM label at the center (hidden on side sets).
    void setBpmLabelVisible(bool visible);

    /// @brief Show/hide ring labels (instrument names).
    void setRingLabelsVisible(bool visible);

    /// @brief Get the LVGL container for z-order management.
    lv_obj_t* getContainer() const { return _container; }

    /// @brief Current selected ring index in this set.
    size_t getSelectedRing() const { return _selectedRing; }

    /// @brief Number of rings in this UI instance.
    size_t getRingCount() const { return _rings.size(); }

    /// @brief Current selected point index for a ring.
    size_t getSelectedPoint(size_t ringIdx) const;

    /// @brief Read back active point state for a ring (used by carousel to sync staged data).
    const std::vector<bool>& getRingActivePoints(size_t ringIdx) const;

    /// @brief Select a ring for the shape panel.
    void selectRing(size_t ringIdx);

    // ─── Callbacks ───────────────────────────────────────────────────────────

    void setOnPointToggle(PointToggleCallback cb)   { _onPointToggle = std::move(cb); }
    void setOnShapeAdd(ShapeAddCallback cb)         { _onShapeAdd = std::move(cb); }
    void setOnShapeRemove(ShapeRemoveCallback cb)   { _onShapeRemove = std::move(cb); }
    void setOnShapeModify(ShapeModifyCallback cb)   { _onShapeModify = std::move(cb); }
    void setOnRotate(RotateCallback cb)             { _onRotate = std::move(cb); }
    void setOnRingSelect(RingSelectCallback cb)     { _onRingSelect = std::move(cb); }
    void setOnModeChange(ModeChangeCallback cb)     { _onModeChange = std::move(cb); }

private:
    // ── Drawing ──────────────────────────────────────────────────────────────
    void createArc(PolyRingData& ring);
    void createDotsForRing(size_t ringIdx);
    void createPlayhead();
    void createShapePanel();
    void rebuildShapePanel();
    void updatePlayhead();
    void updateDots();
    void updateBlinks(uint32_t dtMs);   ///< Process blink timers and apply visual styles
    void refreshModeButtons();
    void clearDotsForRing(size_t ringIdx);
    void clearAllVisuals();

    lv_point_t pointOnCircle(int radius, double angleDeg) const;

    // ── LVGL callbacks ───────────────────────────────────────────────────────
    static void onDotClicked(lv_event_t* e);
    static void onRingLabelClicked(lv_event_t* e);
    static void onAddShape(lv_event_t* e);
    static void onRemoveShape(lv_event_t* e);
    static void onSubdivUp(lv_event_t* e);
    static void onSubdivDown(lv_event_t* e);
    static void onRotateLeft(lv_event_t* e);
    static void onRotateRight(lv_event_t* e);
    static void onModeToggle(lv_event_t* e);
    static void onModePoints(lv_event_t* e);
    static void onModeRotate(lv_event_t* e);

    // ── State ────────────────────────────────────────────────────────────────
    lv_obj_t* _parent     = nullptr;
    lv_obj_t* _container  = nullptr;
    lv_obj_t* _bpmLabel   = nullptr;
    lv_obj_t* _playhead   = nullptr;
    lv_obj_t* _shapePanel = nullptr;
    lv_obj_t* _shapePanelTitle = nullptr;
    lv_obj_t* _modeToggleBtn = nullptr;
    lv_obj_t* _modePointsBtn = nullptr;
    lv_obj_t* _modeRotateBtn = nullptr;
    lv_point_precise_t _playheadPoints[2]{};

    std::vector<PolyRingData>  _rings;
    std::vector<lv_obj_t*>     _shapePanelWidgets;  ///< Dynamic widgets in shape panel
    std::vector<lv_obj_t*>     _arcObjects;         ///< Arc LVGL objects
    std::vector<size_t>        _selectedPoints;     ///< Selected point per ring

    size_t _selectedRing  = 0;
    int    _centerX       = 300;
    int    _centerY       = 240;
    int    _bpm           = 120;
    float  _playheadAngle = 0.0f;
    float  _prevPlayheadAngle = 0.0f;
    float  _targetAngle   = 0.0f;
    bool   _isPlaying     = false;
    bool   _interactive   = true;
    bool   _hasPrevPlayhead = false;
    bool   _hasLastBlinkTick = false;
    std::string _hardwareMode = "rotate";

    std::chrono::steady_clock::time_point _lastBlinkTick;

    /// Blink animation: per-ring, per-dot remaining time in ms.
    /// 0 = no blink active. >0 = blink time remaining.
    /// Outer vector indexed by ring, inner by point.
    std::vector<std::vector<uint16_t>> _blinkCountdown;

    /// Total blink duration in ms.
    static constexpr uint16_t BLINK_DURATION_MS = 120;
    /// Peak flash duration in ms (full white, max size).
    static constexpr uint16_t BLINK_PEAK_MS = 45;

    PointToggleCallback  _onPointToggle;
    ShapeAddCallback     _onShapeAdd;
    ShapeRemoveCallback  _onShapeRemove;
    ShapeModifyCallback  _onShapeModify;
    RotateCallback       _onRotate;
    RingSelectCallback   _onRingSelect;
    ModeChangeCallback   _onModeChange;
};

} // namespace GUI

#endif // POLYRHYTHM_UI_HPP
