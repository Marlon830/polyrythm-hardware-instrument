/**
 * @file PolyRhythmCarousel.cpp
 * @brief Circular carousel implementation for sets of polyrhythmic instruments.
 *
 * @ingroup gui
 */

#include "gui/LVGL/PolyRhythmCarousel.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace GUI {

namespace {
    static int radiusForSlot(size_t ringCount, size_t localRingIdx, int fallback) {
        static constexpr int R1[1] = {132};
        static constexpr int R2[2] = {138, 98};
        static constexpr int R3[3] = {144, 108, 72};
        static constexpr int R4[4] = {148, 116, 84, 52};

        if (ringCount == 1 && localRingIdx < 1) return R1[localRingIdx];
        if (ringCount == 2 && localRingIdx < 2) return R2[localRingIdx];
        if (ringCount == 3 && localRingIdx < 3) return R3[localRingIdx];
        if (ringCount >= 4 && localRingIdx < 4) return R4[localRingIdx];
        return fallback;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Construction / destruction
// ═══════════════════════════════════════════════════════════════════════════════

PolyRhythmCarousel::PolyRhythmCarousel()  = default;

PolyRhythmCarousel::~PolyRhythmCarousel() {
    _sets.clear();  // unique_ptr<PolyRhythmUI> destructors clean up LVGL objects
    if (_indicator)  { lv_obj_del(_indicator);  _indicator  = nullptr; }
    if (_background) { lv_obj_del(_background); _background = nullptr; }
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Initialization
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::init(lv_obj_t* parent) {
    _parent = parent;

    // ── Shared dark background (drawn behind all set containers) ─────────────
    _background = lv_obj_create(parent);
    lv_obj_set_size(_background, 800, 480);
    lv_obj_align(_background, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(_background, lv_color_hex(0x0a1428), 0);
    lv_obj_set_style_border_width(_background, 0, 0);
    lv_obj_clear_flag(_background, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(_background, LV_OBJ_FLAG_SCROLLABLE);

    // ── Set indicator ("1/3") at top center ──────────────────────────────────
    _indicator = lv_label_create(parent);
    lv_label_set_text(_indicator, "");
    lv_obj_set_style_text_color(_indicator, lv_color_hex(0x667799), 0);
    lv_obj_set_style_text_font(_indicator, &lv_font_montserrat_14, 0);
    lv_obj_align(_indicator, LV_ALIGN_TOP_MID, 0, 4);

    std::cout << "[Carousel] Initialized\n";
}


// ═══════════════════════════════════════════════════════════════════════════════
//  IPC data staging (accumulate before rebuildFromData)
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::addRing(const std::string& name, lv_color_t color, int radius) {
    StagedRing sr;
    sr.name   = name;
    sr.color  = color;
    sr.radius = radius;
    _staged.push_back(std::move(sr));
}

void PolyRhythmCarousel::addRingShape(size_t globalIdx, const ShapeInfo& info) {
    if (globalIdx < _staged.size()) {
        _staged[globalIdx].shapes.push_back(info);
    }
}

void PolyRhythmCarousel::setRingTimeline(size_t globalIdx, const std::vector<double>& phases) {
    if (globalIdx < _staged.size()) {
        _staged[globalIdx].timelinePhases = phases;
    }
}

void PolyRhythmCarousel::setRingActivePoints(size_t globalIdx, const std::vector<bool>& active) {
    if (globalIdx < _staged.size()) {
        _staged[globalIdx].activePoints = active;
    }
}

void PolyRhythmCarousel::clearAllRings() {
    _staged.clear();
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Rebuild sets from staged data
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::rebuildFromData() {
    const size_t numRings = _staged.size();
    size_t numSets = (numRings == 0) ? 0
                   : ((numRings + RINGS_PER_SET - 1) / RINGS_PER_SET);
    if (numSets > MAX_SETS) numSets = MAX_SETS;

    // ── Remove excess sets (unique_ptr handles LVGL cleanup) ─────────────────
    while (_sets.size() > numSets) {
        _sets.pop_back();
    }

    // ── Create new sets ──────────────────────────────────────────────────────
    while (_sets.size() < numSets) {
        SetSlot slot;
        slot.ui = std::make_unique<PolyRhythmUI>();
        slot.ui->init(_parent, 300, 240, false);  // transparent container
        _sets.push_back(std::move(slot));
    }

    // ── Populate each set with ring data ─────────────────────────────────────
    for (size_t s = 0; s < numSets; ++s) {
        _sets[s].firstRing = s * RINGS_PER_SET;
        _sets[s].ringCount = std::min(RINGS_PER_SET, numRings - _sets[s].firstRing);
        const float scale = (s == _currentSet)
                          ? 1.0f
                          : static_cast<float>(SIDE_SCALE_PCT) / 100.0f;
        populateSet(s, scale);
    }

    // Validate current set
    if (_currentSet >= _sets.size() && !_sets.empty())
        _currentSet = 0;

    // Restore pending selection from engine if provided
    if (_hasPendingSelectedRing && !_sets.empty()) {
        auto [setIdx, localIdx] = globalToLocal(_pendingSelectedGlobalRing);
        if (setIdx < _sets.size() && localIdx < _sets[setIdx].ringCount) {
            _currentSet = setIdx;
            _sets[setIdx].selectedLocalRing = localIdx;
        }
    }

    // Re-apply per-set selection after rebuild
    for (size_t s = 0; s < _sets.size(); ++s) {
        if (_sets[s].ringCount == 0) continue;
        if (_sets[s].selectedLocalRing >= _sets[s].ringCount) {
            _sets[s].selectedLocalRing = _sets[s].ringCount - 1;
        }
        _sets[s].ui->selectRing(_sets[s].selectedLocalRing);
    }

    applyLayout(false);
    updateIndicator();

    std::cout << "[Carousel] rebuildFromData: " << numRings << " rings → "
              << numSets << " sets\n";
}

void PolyRhythmCarousel::populateSet(size_t setIdx, float radiusScale) {
    if (setIdx >= _sets.size()) return;
    auto& set = _sets[setIdx];
    auto& ui  = *set.ui;

    ui.clearAllRings();

    for (size_t i = 0; i < set.ringCount; ++i) {
        const size_t gi = set.firstRing + i;
        if (gi >= _staged.size()) break;
        const auto& sr = _staged[gi];
        const int displayRadius = static_cast<int>(
            radiusForSlot(set.ringCount, i, sr.radius) * radiusScale);

        ui.addRing(sr.name, sr.color, displayRadius);
        for (const auto& shape : sr.shapes) {
            ui.addRingShape(i, shape);
        }
        ui.setRingTimeline(i, sr.timelinePhases);
        ui.setRingActivePoints(i, sr.activePoints);
    }

    ui.rebuildFromData();
    ui.setHardwareMode(_hardwareMode);

    // ── Wire callbacks: translate local ring index → global ──────────────────
    const size_t offset = set.firstRing;

    ui.setOnPointToggle([this, offset](size_t localRing, size_t pt) {
        if (_onPointToggle) _onPointToggle(offset + localRing, pt);
    });
    ui.setOnShapeAdd([this, offset](size_t localRing, uint32_t subdiv, double off) {
        if (_onShapeAdd) _onShapeAdd(offset + localRing, subdiv, off);
    });
    ui.setOnShapeRemove([this, offset](size_t localRing, size_t shapeIdx) {
        if (_onShapeRemove) _onShapeRemove(offset + localRing, shapeIdx);
    });
    ui.setOnShapeModify([this, offset](size_t localRing, size_t shapeIdx,
                                       uint32_t ns, double no) {
        if (_onShapeModify) _onShapeModify(offset + localRing, shapeIdx, ns, no);
    });
    ui.setOnRotate([this, offset](size_t localRing, int amount) {
        if (_onRotate) _onRotate(offset + localRing, amount);
    });
    ui.setOnRingSelect([this, offset, setIdx](size_t localRing) {
        if (setIdx < _sets.size()) {
            _sets[setIdx].selectedLocalRing = localRing;
        }
        if (_onRingSelect) _onRingSelect(offset + localRing);
    });
    ui.setOnModeChange([this](const std::string& mode) {
        if (_onModeChange) {
            _onModeChange(mode);
        }
    });
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Forwarding to sets (phase, BPM, playing, select, toggle)
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::setPlayheadPhase(double phase) {
    for (auto& set : _sets) set.ui->setPlayheadPhase(phase);
}

void PolyRhythmCarousel::setBPM(int bpm) {
    for (auto& set : _sets) set.ui->setBPM(bpm);
}

void PolyRhythmCarousel::setPlaying(bool playing) {
    for (auto& set : _sets) set.ui->setPlaying(playing);
}

void PolyRhythmCarousel::setHardwareMode(const std::string& mode) {
    _hardwareMode = mode;
    for (auto& set : _sets) {
        set.ui->setHardwareMode(mode);
    }
}

void PolyRhythmCarousel::selectRing(size_t globalIdx) {
    _pendingSelectedGlobalRing = globalIdx;
    _hasPendingSelectedRing = true;

    auto [setIdx, localIdx] = globalToLocal(globalIdx);
    if (setIdx < _sets.size()) {
        // Navigate to the correct set if needed
        if (setIdx != _currentSet) {
            _currentSet = setIdx;
            applyLayout(false);
            updateIndicator();
        }
        _sets[setIdx].selectedLocalRing = localIdx;
        _sets[setIdx].ui->selectRing(localIdx);
    }
}

void PolyRhythmCarousel::togglePointLocal(size_t globalIdx, size_t pointIdx) {
    auto [setIdx, localIdx] = globalToLocal(globalIdx);
    if (setIdx < _sets.size()) {
        _sets[setIdx].ui->togglePointLocal(localIdx, pointIdx);
    }
}

void PolyRhythmCarousel::setSelectedPoint(size_t globalIdx, size_t pointIdx) {
    auto [setIdx, localIdx] = globalToLocal(globalIdx);
    if (setIdx < _sets.size()) {
        _sets[setIdx].ui->setSelectedPoint(localIdx, pointIdx);
    }
}

size_t PolyRhythmCarousel::getCurrentSelectedGlobalRing() const {
    if (_sets.empty() || _currentSet >= _sets.size()) {
        return 0;
    }

    const auto& set = _sets[_currentSet];
    if (set.ringCount == 0) {
        return set.firstRing;
    }

    size_t local = set.selectedLocalRing;
    if (local >= set.ringCount) {
        local = set.ringCount - 1;
    }
    return set.firstRing + local;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Navigation
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::navigateLeft() {
    if (_isAnimating || _sets.size() <= 1) return;

    const size_t n = _sets.size();
    const size_t oldCenter = _currentSet;
    const size_t newCenter = (_currentSet + n - 1) % n;

    // If the incoming set isn't already visible as a preview,
    // pre-position it at the left edge for correct entry direction.
    auto* cont = _sets[newCenter].ui->getContainer();
    const bool wasHidden = lv_obj_has_flag(cont, LV_OBJ_FLAG_HIDDEN);
    if (wasHidden) {
        // Was hidden → unhide, snap to left edge (transparent), will fade in
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_x(cont, LEFT_XOFF);
    } else {
        // Was visible (e.g., on the right side for n==2) →
        // snap to left edge so animation direction matches nav direction
        int curX = lv_obj_get_x(cont);
        if (curX > LEFT_XOFF) {
            lv_anim_del(cont, animXCb);
            lv_anim_del(cont, animOpaCb);
            lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
            lv_obj_set_x(cont, LEFT_XOFF);
        }
    }

    _currentSet = newCenter;

    // Sync live active-point state back to _staged before re-populating,
    // so toggles made by the user are preserved across set role changes.
    syncSetToStaged(oldCenter);
    syncSetToStaged(newCenter);

    applyLayout(true);
    updateIndicator();

    // Re-populate sets that changed role so their ring sizes match their new position.
    // Old center → side (shrink), new center → center (full size).
    const float sideScale = static_cast<float>(SIDE_SCALE_PCT) / 100.0f;
    populateSet(oldCenter, sideScale);
    populateSet(newCenter, 1.0f);

    // populateSet rebuilds LVGL objects (labels etc.) after applyLayout has
    // already set visibility flags, so re-apply them for the re-populated sets.
    _sets[oldCenter].ui->setRingLabelsVisible(false);
    _sets[oldCenter].ui->setShapePanelVisible(false);
    _sets[oldCenter].ui->setBpmLabelVisible(false);
    _sets[oldCenter].ui->setInteractive(false);

    _sets[newCenter].ui->setRingLabelsVisible(true);
    _sets[newCenter].ui->setShapePanelVisible(true);
    _sets[newCenter].ui->setBpmLabelVisible(true);
    _sets[newCenter].ui->setInteractive(true);
    _sets[newCenter].ui->selectRing(_sets[newCenter].selectedLocalRing);
    if (_onRingSelect) {
        _onRingSelect(_sets[newCenter].firstRing + _sets[newCenter].selectedLocalRing);
    }

    std::cout << "[Carousel] Navigate left → set " << _currentSet << "\n";
}

void PolyRhythmCarousel::navigateRight() {
    if (_isAnimating || _sets.size() <= 1) return;

    const size_t n = _sets.size();
    const size_t oldCenter = _currentSet;
    const size_t newCenter = (_currentSet + 1) % n;

    // Pre-position entering set at the right edge if not already visible there.
    auto* cont = _sets[newCenter].ui->getContainer();
    const bool wasHidden = lv_obj_has_flag(cont, LV_OBJ_FLAG_HIDDEN);
    if (wasHidden) {
        // Was hidden → unhide, snap to right edge transparent, will fade in
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_x(cont, RIGHT_XOFF);
    }
    // If already visible as right preview, animation handles it naturally.

    _currentSet = newCenter;

    // Sync live active-point state back to _staged before re-populating.
    syncSetToStaged(oldCenter);
    syncSetToStaged(newCenter);

    applyLayout(true);
    updateIndicator();

    // Re-populate sets that changed role so their ring sizes match their new position.
    const float sideScale = static_cast<float>(SIDE_SCALE_PCT) / 100.0f;
    populateSet(oldCenter, sideScale);
    populateSet(newCenter, 1.0f);

    // Re-apply visibility flags destroyed by populateSet's rebuild.
    _sets[oldCenter].ui->setRingLabelsVisible(false);
    _sets[oldCenter].ui->setShapePanelVisible(false);
    _sets[oldCenter].ui->setBpmLabelVisible(false);
    _sets[oldCenter].ui->setInteractive(false);

    _sets[newCenter].ui->setRingLabelsVisible(true);
    _sets[newCenter].ui->setShapePanelVisible(true);
    _sets[newCenter].ui->setBpmLabelVisible(true);
    _sets[newCenter].ui->setInteractive(true);
    _sets[newCenter].ui->selectRing(_sets[newCenter].selectedLocalRing);
    if (_onRingSelect) {
        _onRingSelect(_sets[newCenter].firstRing + _sets[newCenter].selectedLocalRing);
    }

    std::cout << "[Carousel] Navigate right → set " << _currentSet << "\n";
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Layout — position, opacity, z-order, interaction
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::applyLayout(bool animate) {
    const size_t n = _sets.size();
    if (n == 0) return;

    // Track selected local ring from current active set before changing visibility.
    if (_currentSet < _sets.size() && _sets[_currentSet].ui) {
        _sets[_currentSet].selectedLocalRing = _sets[_currentSet].ui->getSelectedRing();
    }

    // Determine prev/next indices (circular).
    const size_t prev = (n > 1) ? ((_currentSet + n - 1) % n) : SIZE_MAX;
    size_t next = (n > 1) ? ((_currentSet + 1) % n) : SIZE_MAX;

    // For n == 2, prev == next.  Show the other set as "next" (right side only).
    const bool prevEqualsNext = (prev == next);

    for (size_t s = 0; s < n; ++s) {
        auto* cont = _sets[s].ui->getContainer();
        int      targetX;
        int      targetY;
        lv_opa_t targetOpa;
        uint16_t targetScale;
        bool     isCenter;

        if (s == _currentSet) {
            targetX   = CENTER_XOFF;
            targetY   = CENTER_YOFF;
            targetOpa = LV_OPA_COVER;
            targetScale = CENTER_SCALE_PCT;
            isCenter  = true;
        } else if (s == prev && !prevEqualsNext) {
            targetX   = LEFT_XOFF;
            targetY   = SIDE_YOFF;
            targetOpa = SIDE_OPA;
            targetScale = SIDE_SCALE_PCT;
            isCenter  = false;
        } else if (s == next) {
            targetX   = RIGHT_XOFF;
            targetY   = SIDE_YOFF;
            targetOpa = SIDE_OPA;
            targetScale = SIDE_SCALE_PCT;
            isCenter  = false;
        } else {
            targetX   = HIDDEN_XOFF;
            targetY   = CENTER_YOFF;
            targetOpa = LV_OPA_TRANSP;
            targetScale = SIDE_SCALE_PCT;
            isCenter  = false;
        }

        // Interaction & panel visibility
        _sets[s].ui->setInteractive(isCenter);
        _sets[s].ui->setShapePanelVisible(isCenter);
        _sets[s].ui->setBpmLabelVisible(isCenter);
        _sets[s].ui->setRingLabelsVisible(isCenter);
        // Note: setContainerScalePct intentionally not called.
        // LVGL software transform on 800×480 containers is too expensive.

        const bool hidden = (targetOpa == LV_OPA_TRANSP);

        if (animate) {
            // During animation always keep container visible so the transition is seen.
            // Hidden flag is only applied in the static (non-animated) path.
            lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);

            const int curX = lv_obj_get_x(cont);
            const int curY = lv_obj_get_y(cont);
            const lv_opa_t curOpa = lv_obj_get_style_opa(cont, LV_PART_MAIN);

            // Cancel any running animation on this container
            lv_anim_del(cont, animXCb);
            lv_anim_del(cont, animYCb);
            lv_anim_del(cont, animOpaCb);

            if (curX != targetX) {
                lv_anim_t ax;
                lv_anim_init(&ax);
                lv_anim_set_var(&ax, cont);
                lv_anim_set_values(&ax, curX, targetX);
                lv_anim_set_duration(&ax, ANIM_DURATION_MS);
                lv_anim_set_exec_cb(&ax, animXCb);
                lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
                lv_anim_start(&ax);
            }

            if (curY != targetY) {
                lv_anim_t ay;
                lv_anim_init(&ay);
                lv_anim_set_var(&ay, cont);
                lv_anim_set_values(&ay, curY, targetY);
                lv_anim_set_duration(&ay, ANIM_DURATION_MS);
                lv_anim_set_exec_cb(&ay, animYCb);
                lv_anim_set_path_cb(&ay, lv_anim_path_ease_out);
                lv_anim_start(&ay);
            }

            if (curOpa != targetOpa) {
                lv_anim_t ao;
                lv_anim_init(&ao);
                lv_anim_set_var(&ao, cont);
                lv_anim_set_values(&ao, static_cast<int32_t>(curOpa),
                                        static_cast<int32_t>(targetOpa));
                lv_anim_set_duration(&ao, ANIM_DURATION_MS);
                lv_anim_set_exec_cb(&ao, animOpaCb);
                lv_anim_set_path_cb(&ao, lv_anim_path_ease_out);
                lv_anim_start(&ao);
            }

            _isAnimating = true;
            _animStart   = std::chrono::steady_clock::now();
        } else {
            if (hidden) {
                // Use LVGL's hidden flag so the container is excluded from layout
                // calculations and cannot cause scroll overflow on the root screen.
                lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_x(cont, CENTER_XOFF);          // park at 0 (safe)
                lv_obj_set_y(cont, CENTER_YOFF);          // park at neutral Y
                lv_obj_set_style_opa(cont, LV_OPA_TRANSP, 0);
            } else {
                lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_x(cont, targetX);
                lv_obj_set_y(cont, targetY);
                lv_obj_set_style_opa(cont, targetOpa, 0);
            }
        }

        // Z-order: center container on top of side containers
        if (isCenter) {
            lv_obj_move_foreground(cont);
        }
    }

    // Indicator always on very top
    if (_indicator) lv_obj_move_foreground(_indicator);
}

void PolyRhythmCarousel::updateIndicator() {
    if (!_indicator) return;

    if (_sets.empty()) {
        lv_label_set_text(_indicator, "");
        return;
    }

    if (_sets.size() == 1) {
        // Single set — no indicator needed
        lv_label_set_text(_indicator, "");
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%zu/%zu", _currentSet + 1, _sets.size());
    lv_label_set_text(_indicator, buf);
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Frame update
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::update() {
    // Check animation completion
    if (_isAnimating) {
        auto elapsed = std::chrono::steady_clock::now() - _animStart;
        if (elapsed >= std::chrono::milliseconds(ANIM_DURATION_MS + 50)) {
            _isAnimating = false;
        }
    }

    const size_t n = _sets.size();
    if (n == 0) return;

    const size_t prev = (n > 1) ? ((_currentSet + n - 1) % n) : SIZE_MAX;
    const size_t next = (n > 1) ? ((_currentSet + 1) % n) : SIZE_MAX;

    for (size_t s = 0; s < n; ++s) {
        const bool visible = (s == _currentSet)
                          || (s == prev && prev != next)
                          || (s == next);
        if (visible && _sets[s].ui) {
            _sets[s].ui->update();
        }
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmCarousel::syncSetToStaged(size_t setIdx) {
    if (setIdx >= _sets.size()) return;
    const auto& set = _sets[setIdx];
    for (size_t i = 0; i < set.ringCount; ++i) {
        const size_t gi = set.firstRing + i;
        if (gi >= _staged.size()) break;
        _staged[gi].activePoints = set.ui->getRingActivePoints(i);
    }
}

std::pair<size_t, size_t> PolyRhythmCarousel::globalToLocal(size_t globalIdx) const {
    return { globalIdx / RINGS_PER_SET, globalIdx % RINGS_PER_SET };
}

void PolyRhythmCarousel::animXCb(void* var, int32_t v) {
    lv_obj_set_x(static_cast<lv_obj_t*>(var), v);
}

void PolyRhythmCarousel::animYCb(void* var, int32_t v) {
    lv_obj_set_y(static_cast<lv_obj_t*>(var), v);
}

void PolyRhythmCarousel::animOpaCb(void* var, int32_t v) {
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(var), static_cast<lv_opa_t>(v), 0);
}

} // namespace GUI