/**
 * @file PolyRhythmModule.cpp
 * @brief Implementation of the polyrhythmic sequencer engine.
 *
 * @ingroup engine_module
 *
 * ── Scheduling algorithm (per buffer) ───────────────────────────────────────
 *
 *   φ_start = current phase
 *   φ_end   = φ_start + Δφ × bufferSize
 *
 *   For each timeline point p:
 *     if   φ_end < 1.0:  triggered ⟺ φ_start ≤ p < φ_end
 *     else (wrap):       triggered ⟺ p ≥ φ_start  OR  p < (φ_end mod 1.0)
 *
 *   Sample offset within buffer:
 *     δφ = (p ≥ φ_start) ? (p − φ_start) : (1.0 − φ_start + p)
 *     sampleOffset = δφ / Δφ
 *
 *   After scanning, advance:  φ ← φ_end mod 1.0
 *
 * ── Timeline rebuild ────────────────────────────────────────────────────────
 *
 *   Called only when shapes change (never in the audio callback).
 *   Complexity: O(Σnᵢ · log(Σnᵢ)) — negligible for typical subdivision counts.
 *
 * ── Tempo change handling ───────────────────────────────────────────────────
 *
 *   Because scheduling is phase-based, a tempo change only affects Δφ.
 *   The playhead maintains its current position — no drift, no reset.
 */

#include "engine/module/PolyRhythmModule.hpp"
#include "engine/port/OutputPort.hpp"
#include "engine/module/ModuleFactory.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>

namespace Engine {
namespace Module {

// ═══════════════════════════════════════════════════════════════════════════════
//  Utility implementations
// ═══════════════════════════════════════════════════════════════════════════════

uint32_t polyGcd(uint32_t a, uint32_t b) {
    while (b) { uint32_t t = b; b = a % b; a = t; }
    return a;
}

uint32_t polyLcm(uint32_t a, uint32_t b) {
    return (a && b) ? (a / polyGcd(a, b)) * b : 0;
}

/// Bjorklund's algorithm — distributes k onsets across n positions as evenly
/// as possible. Produces classic Euclidean rhythms (e.g. tresillo, rumba clave).
///
/// The algorithm works like Bresenham line-drawing: it interleaves groups
/// until only 0 or 1 remainder groups are left.
std::vector<bool> bjorklund(uint32_t hits, uint32_t steps) {
    if (steps == 0) return {};
    if (hits >= steps) return std::vector<bool>(steps, true);
    if (hits == 0)     return std::vector<bool>(steps, false);

    // Build groups: `hits` groups of [true], `steps-hits` groups of [false]
    std::vector<std::vector<bool>> groups;
    groups.reserve(steps);
    for (uint32_t i = 0; i < hits; ++i)         groups.push_back({true});
    for (uint32_t i = 0; i < steps - hits; ++i) groups.push_back({false});

    for (;;) {
        if (groups.size() <= 1) break;

        // Find where groups start differing from the first group
        size_t splitIdx = groups.size();
        for (size_t i = 1; i < groups.size(); ++i) {
            if (groups[i] != groups[0]) { splitIdx = i; break; }
        }
        if (splitIdx >= groups.size()) break;  // all identical

        size_t nFront     = splitIdx;
        size_t nRemainder = groups.size() - splitIdx;
        if (nRemainder <= 1) break;

        size_t nMerge = std::min(nFront, nRemainder);
        std::vector<std::vector<bool>> next;
        next.reserve(groups.size() - nMerge);

        // Merge front[i] + remainder[i]
        for (size_t i = 0; i < nMerge; ++i) {
            auto combined = groups[i];
            combined.insert(combined.end(),
                            groups[nFront + i].begin(),
                            groups[nFront + i].end());
            next.push_back(std::move(combined));
        }
        // Left-over front groups
        for (size_t i = nMerge; i < nFront; ++i)
            next.push_back(std::move(groups[i]));
        // Left-over remainder groups
        for (size_t i = nFront + nMerge; i < groups.size(); ++i)
            next.push_back(std::move(groups[i]));

        groups = std::move(next);
    }

    // Flatten
    std::vector<bool> result;
    result.reserve(steps);
    for (const auto& g : groups)
        result.insert(result.end(), g.begin(), g.end());
    return result;
}

/// Build the merged timeline from multiple shapes.
/// Each shape contributes its subdivision points; duplicates (within epsilon) are fused.
std::vector<TimelinePoint> buildTimeline(
    const std::vector<RhythmShape>& shapes,
    double epsilon)
{
    std::vector<TimelinePoint> points;

    for (size_t si = 0; si < shapes.size(); ++si) {
        const auto& shape = shapes[si];
        if (shape.subdivision == 0) continue;

        // Optional Euclidean mask: only generate points where Bjorklund says "hit"
        std::vector<bool> mask;
        if (shape.euclideanHits > 0) {
            mask = bjorklund(shape.euclideanHits, shape.subdivision);
        }

        for (uint32_t j = 0; j < shape.subdivision; ++j) {
            // Skip if Euclidean mask says rest
            if (!mask.empty() && !mask[j]) continue;

            double phase = std::fmod(
                static_cast<double>(j) / shape.subdivision + shape.offset, 1.0);
            if (phase < 0.0) phase += 1.0;

            TimelinePoint pt;
            pt.phase             = phase;
            pt.sourceShapeIndex  = static_cast<uint32_t>(si);
            pt.subdivisionIndex  = j;
            points.push_back(pt);
        }
    }

    // Sort by phase
    std::sort(points.begin(), points.end());

    // De-duplicate within epsilon
    if (points.size() > 1) {
        std::vector<TimelinePoint> unique;
        unique.reserve(points.size());
        unique.push_back(points[0]);
        for (size_t i = 1; i < points.size(); ++i) {
            if (points[i].phase - unique.back().phase > epsilon) {
                unique.push_back(points[i]);
            }
            // else: merge — drop duplicate (keep the earlier one)
        }
        points = std::move(unique);
    }

    return points;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  PolyRhythmModule — construction / destruction
// ═══════════════════════════════════════════════════════════════════════════════

PolyRhythmModule::PolyRhythmModule()
    : _bpm(std::make_shared<Param<float>>("BPM", 120.0f))
    , _beatsPerCycle(std::make_shared<Param<uint32_t>>("BeatsPerCycle", 4))
    , _phase(0.0)
    , _firstProc(true)
{
    _name = "PolyRhythmModule";
    _type = "poly_rhythm";

    // Single event output port (same convention as StepSequencerModule)
    auto output = std::make_shared<Port::OutputPort>(
        "PolyRhythm Event Output",
        Signal::SignalType::EVENT);
    _outputPorts.push_back(output);

    _parameters.push_back(std::static_pointer_cast<ParamBase>(_bpm));
    _parameters.push_back(std::static_pointer_cast<ParamBase>(_beatsPerCycle));

    std::cerr << "[PolyRhythmModule] Created — "
              << _bpm->get() << " BPM, "
              << _beatsPerCycle->get() << " beats/cycle\n";
}

PolyRhythmModule::PolyRhythmModule(std::string name)
    : _bpm(std::make_shared<Param<float>>("BPM", 120.0f))
    , _beatsPerCycle(std::make_shared<Param<uint32_t>>("BeatsPerCycle", 4))
    , _phase(0.0)
    , _firstProc(true)
{
    _name = std::move(name);
    _type = "poly_rhythm";

    auto output = std::make_shared<Port::OutputPort>(
        "PolyRhythm Event Output",
        Signal::SignalType::EVENT);
    _outputPorts.push_back(output);

    _parameters.push_back(std::static_pointer_cast<ParamBase>(_bpm));
    _parameters.push_back(std::static_pointer_cast<ParamBase>(_beatsPerCycle));

    std::cerr << "[PolyRhythmModule] Created '" << _name << "' — "
              << _bpm->get() << " BPM, "
              << _beatsPerCycle->get() << " beats/cycle\n";
}

PolyRhythmModule::~PolyRhythmModule() = default;

IModule* PolyRhythmModule::clone() const {
    return new PolyRhythmModule(*this);
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Real-time processing
// ═══════════════════════════════════════════════════════════════════════════════

double PolyRhythmModule::computePhaseIncrement(double sampleRate) const {
    // One cycle = beatsPerCycle beats at BPM tempo
    // Cycle duration (seconds) = beatsPerCycle * 60 / BPM
    // Phase increment per sample = 1 / (cycleDuration * sampleRate)
    //                             = BPM / (60 * beatsPerCycle * sampleRate)
    double bpc = static_cast<double>(_beatsPerCycle->get());
    if (bpc <= 0.0) bpc = 4.0;
    return static_cast<double>(_bpm->get()) / (60.0 * bpc * sampleRate);
}

void PolyRhythmModule::process(Core::AudioContext& context) {
    if (_timeline.empty() || _tracks.empty()) return;

    const double phaseInc   = computePhaseIncrement(static_cast<double>(context.sampleRate));
    const double bufferSpan = phaseInc * context.bufferSize;
    const double pStart     = _phase;
    const double pEnd       = _phase + bufferSpan;
    const bool   wraps      = (pEnd >= 1.0);
    const double pEndMod    = wraps ? std::fmod(pEnd, 1.0) : pEnd;

    // ── Scan timeline for triggered points ──────────────────────────────────
    //
    // Because the timeline is sorted by phase, we could binary-search for the
    // start of the window.  For typical timeline sizes (< 64 points) a linear
    // scan is faster due to cache locality and avoids branch misprediction.

    for (size_t i = 0; i < _timeline.size(); ++i) {
        const double p = _timeline[i].phase;

        const bool hit = wraps
            ? (p >= pStart || p < pEndMod)
            : (p >= pStart && p < pEnd);

        if (!hit) continue;

        // ── Compute sample-accurate offset within this buffer ───────────────
        const double dp = (p >= pStart)
            ? (p - pStart)
            : (1.0 - pStart + p);   // point wrapped past 1.0

        uint32_t sampleOffset = static_cast<uint32_t>(dp / phaseInc);
        if (sampleOffset >= static_cast<uint32_t>(context.bufferSize))
            sampleOffset = static_cast<uint32_t>(context.bufferSize - 1);

        // ── Fire for every track that has this point active ─────────────────
        for (const auto& track : _tracks) {
            if (i >= track.activePoints.size() || !track.activePoints[i])
                continue;

            // NOTE_ON — sample-accurate
            _outputPorts[0]->send(std::make_shared<Signal::EventSignal>(
                Signal::EventType::NOTE_ON,
                sampleOffset,
                static_cast<uint8_t>(track.noteNumber),
                static_cast<uint8_t>(track.velocity)));

            // NOTE_OFF — short hit (configurable length is a future TODO)
            _outputPorts[0]->send(std::make_shared<Signal::EventSignal>(
                Signal::EventType::NOTE_OFF,
                sampleOffset + static_cast<uint32_t>(context.bufferSize / 4),
                static_cast<uint8_t>(track.noteNumber),
                0));
        }
    }

    // ── Advance playhead ────────────────────────────────────────────────────
    _phase = wraps ? pEndMod : pEnd;

    _firstProc = false;
}

void PolyRhythmModule::reset() {
    _phase     = 0.0;
    _firstProc = true;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Shape management
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmModule::addShape(const RhythmShape& shape) {
    _shapes.push_back(shape);
    rebuildTimeline();
    std::cerr << "[PolyRhythmModule] addShape: n=" << shape.subdivision
              << " offset=" << shape.offset
              << " → timeline has " << _timeline.size() << " points\n";
}

void PolyRhythmModule::removeShape(size_t index) {
    if (index < _shapes.size()) {
        _shapes.erase(_shapes.begin() + static_cast<ptrdiff_t>(index));
        rebuildTimeline();
        std::cerr << "[PolyRhythmModule] removeShape[" << index
                  << "] → timeline has " << _timeline.size() << " points\n";
    }
}

void PolyRhythmModule::updateShape(size_t index, const RhythmShape& shape) {
    if (index < _shapes.size()) {
        _shapes[index] = shape;
        rebuildTimeline();
    }
}

const std::vector<RhythmShape>& PolyRhythmModule::getShapes() const {
    return _shapes;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Track management
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmModule::setTracks(const std::vector<PolyRhythmTrack>& tracks) {
    _tracks = tracks;
    // Ensure every track's activePoints matches the current timeline size
    for (auto& t : _tracks)
        t.activePoints.resize(_timeline.size(), false);
}

const std::vector<PolyRhythmTrack>& PolyRhythmModule::getTracks() const {
    return _tracks;
}

void PolyRhythmModule::togglePoint(size_t trackIndex, size_t pointIndex) {
    if (trackIndex < _tracks.size() && pointIndex < _tracks[trackIndex].activePoints.size()) {
        _tracks[trackIndex].activePoints[pointIndex] =
            !_tracks[trackIndex].activePoints[pointIndex];
    }
}

void PolyRhythmModule::applyEuclidean(size_t trackIndex, uint32_t hits) {
    if (trackIndex >= _tracks.size() || _timeline.empty()) return;

    auto pattern = bjorklund(hits, static_cast<uint32_t>(_timeline.size()));
    _tracks[trackIndex].activePoints = pattern;
}

void PolyRhythmModule::rotateTrack(size_t trackIndex, int amount) {
    if (trackIndex >= _tracks.size()) return;
    auto& ap = _tracks[trackIndex].activePoints;
    if (ap.empty()) return;

    int n = static_cast<int>(ap.size());
    int shift = ((amount % n) + n) % n;  // normalize to [0, n)
    std::rotate(ap.begin(), ap.begin() + shift, ap.end());
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Accessors
// ═══════════════════════════════════════════════════════════════════════════════

const std::vector<TimelinePoint>& PolyRhythmModule::getTimeline() const {
    return _timeline;
}

size_t PolyRhythmModule::getTimelineSize() const {
    return _timeline.size();
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Internal: rebuild timeline
// ═══════════════════════════════════════════════════════════════════════════════

void PolyRhythmModule::rebuildTimeline() {
    size_t oldSize = _timeline.size();

    _timeline = buildTimeline(_shapes);

    size_t newSize = _timeline.size();

    // Resize all track masks.
    // Strategy: if the timeline grew, new points default to inactive.
    // If it shrank, excess points are silently dropped.
    // For a smarter strategy (preserving toggled points across shape changes),
    // one would need to map old phase → new phase, which is a UI concern.
    for (auto& track : _tracks) {
        track.activePoints.resize(newSize, false);
    }

    if (oldSize != newSize) {
        std::cerr << "[PolyRhythmModule] rebuildTimeline: "
                  << oldSize << " → " << newSize << " points\n";
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Auto-register in ModuleFactory
// ═══════════════════════════════════════════════════════════════════════════════

static AutoRegister polyRhythmModuleReg{
    "poly_rhythm",
    [](std::string name) -> std::shared_ptr<IModule> {
        return std::make_shared<PolyRhythmModule>(std::move(name));
    }
};

} // namespace Module
} // namespace Engine
