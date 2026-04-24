/**
 * @file PolyRhythmCarousel.hpp
 * @brief Circular carousel of PolyRhythmUI sets for the polyrhythmic sequencer.
 *
 * @ingroup gui
 *
 * ── Carousel concept ────────────────────────────────────────────────────────
 *
 *   Instruments are grouped into "sets" of up to RINGS_PER_SET (4) rings.
 *   Up to MAX_SETS (5) sets can exist (supporting up to 20 instruments).
 *   Sets form a circular loop: 1 → 2 → 3 → … → N → 1.
 *
 *   Three sets are visible simultaneously:
 *     • center — fully opaque, interactive (shape panel + BPM shown)
 *     • left   — dimmed preview, read-only, playhead animated
 *     • right  — dimmed preview, read-only, playhead animated
 *
 *   Keyboard left/right arrows navigate through sets with smooth
 *   lv_anim_t position + opacity transitions.
 *
 * ── IPC routing ─────────────────────────────────────────────────────────────
 *
 *   All methods accept *global* ring indices (0-based across all instruments).
 *   The carousel transparently maps them to (setIndex, localRingIndex) and
 *   routes to the correct PolyRhythmUI instance.
 */

#ifndef POLYRHYTHM_CAROUSEL_HPP
#define POLYRHYTHM_CAROUSEL_HPP

#include "gui/LVGL/PolyRhythmUI.hpp"

#include <vector>
#include <memory>
#include <chrono>
#include <cstddef>
#include <string>

namespace GUI {

class PolyRhythmCarousel {
public:
    static constexpr size_t RINGS_PER_SET = 4;
    static constexpr size_t MAX_SETS      = 5;
    static constexpr size_t MAX_RINGS     = RINGS_PER_SET * MAX_SETS;  // 20

    PolyRhythmCarousel();
    ~PolyRhythmCarousel();

    /// @brief Initialize carousel within the given LVGL parent (typically lv_screen_active()).
    void init(lv_obj_t* parent);

    // ─── IPC data routing (global ring indices) ──────────────────────────────

    void addRing(const std::string& name, lv_color_t color, int radius);
    void addRingShape(size_t globalRingIdx, const ShapeInfo& info);
    void setRingTimeline(size_t globalRingIdx, const std::vector<double>& phases);
    void setRingActivePoints(size_t globalRingIdx, const std::vector<bool>& active);
    void clearAllRings();
    void rebuildFromData();

    void setPlayheadPhase(double phase);
    void setBPM(int bpm);
    void setPlaying(bool playing);
    void setHardwareMode(const std::string& mode);
    void selectRing(size_t globalRingIdx);
    void setSelectedPoint(size_t globalRingIdx, size_t pointIdx);
    void togglePointLocal(size_t globalRingIdx, size_t pointIdx);
    size_t getCurrentSelectedGlobalRing() const;

    // ─── Callbacks (fire with global ring indices) ───────────────────────────

    void setOnPointToggle(PointToggleCallback cb)   { _onPointToggle = std::move(cb); }
    void setOnShapeAdd(ShapeAddCallback cb)         { _onShapeAdd = std::move(cb); }
    void setOnShapeRemove(ShapeRemoveCallback cb)   { _onShapeRemove = std::move(cb); }
    void setOnShapeModify(ShapeModifyCallback cb)   { _onShapeModify = std::move(cb); }
    void setOnRotate(RotateCallback cb)             { _onRotate = std::move(cb); }
    void setOnRingSelect(RingSelectCallback cb)     { _onRingSelect = std::move(cb); }
    void setOnModeChange(ModeChangeCallback cb)     { _onModeChange = std::move(cb); }

    // ─── Navigation ──────────────────────────────────────────────────────────

    void navigateLeft();
    void navigateRight();

    // ─── Frame update ────────────────────────────────────────────────────────

    void update();

    size_t getCurrentSet() const { return _currentSet; }
    size_t getSetCount()  const { return _sets.size(); }

private:
    /// @brief Map global ring index → (setIndex, localRingIndex).
    std::pair<size_t, size_t> globalToLocal(size_t globalIdx) const;

    /// @brief Populate one set's PolyRhythmUI from staged ring data.
    /// @param radiusScale Multiply all ring radii by this factor (1.0 = center, <1 = side preview).
    void populateSet(size_t setIdx, float radiusScale = 1.0f);

    /// @brief Save live active-point state from a set's UI back to _staged.
    void syncSetToStaged(size_t setIdx);

    /// @brief Apply visual layout: position, opacity, z-order, and interaction flags.
    void applyLayout(bool animate);

    /// @brief Update the "1/3" indicator label.
    void updateIndicator();

    // LVGL animation callbacks
    static void animXCb(void* var, int32_t v);
    static void animYCb(void* var, int32_t v);
    static void animOpaCb(void* var, int32_t v);

    // ── Staged ring data (accumulated before rebuildFromData) ─────────────────

    struct StagedRing {
        std::string             name;
        lv_color_t              color;
        int                     radius;
        std::vector<ShapeInfo>  shapes;
        std::vector<double>     timelinePhases;
        std::vector<bool>       activePoints;
    };

    // ── Per-set slot ─────────────────────────────────────────────────────────

    struct SetSlot {
        std::unique_ptr<PolyRhythmUI> ui;
        size_t firstRing = 0;   ///< Global index of first ring in this set
        size_t ringCount = 0;   ///< Number of rings in this set
        size_t selectedLocalRing = 0;
    };

    lv_obj_t* _parent     = nullptr;
    lv_obj_t* _background = nullptr;    ///< Full-screen dark background
    lv_obj_t* _indicator  = nullptr;    ///< "Set 1/3" label

    std::vector<StagedRing> _staged;
    std::vector<SetSlot>    _sets;
    size_t _currentSet = 0;
    size_t _pendingSelectedGlobalRing = 0;
    bool   _hasPendingSelectedRing = false;

    bool _isAnimating = false;
    std::chrono::steady_clock::time_point _animStart;
    std::string _hardwareMode = "rotate";

    // Layout constants for 800×480 screen
    static constexpr int ANIM_DURATION_MS = 250;
    static constexpr int LEFT_XOFF   = -210;
    static constexpr int CENTER_XOFF = 0;
    static constexpr int RIGHT_XOFF  = 210;
    static constexpr int HIDDEN_XOFF = 900;
    static constexpr int CENTER_YOFF = 0;       ///< Center set: no vertical offset
    static constexpr int SIDE_YOFF   = -55;     ///< Side sets: shift up for depth effect
    static constexpr lv_opa_t SIDE_OPA = 40;   ///< ~22% opacity for side previews (less cluttered)
    static constexpr uint16_t CENTER_SCALE_PCT = 100;
    static constexpr uint16_t SIDE_SCALE_PCT   = 80;

    // External callbacks
    PointToggleCallback  _onPointToggle;
    ShapeAddCallback     _onShapeAdd;
    ShapeRemoveCallback  _onShapeRemove;
    ShapeModifyCallback  _onShapeModify;
    RotateCallback       _onRotate;
    RingSelectCallback   _onRingSelect;
    ModeChangeCallback   _onModeChange;
};

} // namespace GUI

#endif // POLYRHYTHM_CAROUSEL_HPP
