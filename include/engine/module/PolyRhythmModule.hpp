/**
 * @file PolyRhythmModule.hpp
 * @brief Polyrhythmic sequencer module based on subdivision shape combination.
 *
 * @ingroup engine_module
 *
 * Unlike a classic step sequencer (uniform grid of N steps), the PolyRhythm module
 * builds a non-uniform timeline from the union of multiple "rhythmic shapes", each
 * with its own subdivision count.
 *
 * ── Mathematical model ──────────────────────────────────────────────────────────
 *
 *   One cycle = one loop / bar. Time is represented as normalized phase φ ∈ [0, 1).
 *
 *   A RhythmShape S with subdivision n and offset θ produces n points:
 *       P_S = { (j/n + θ) mod 1.0  |  j ∈ [0, n) }
 *
 *   The merged timeline T is the sorted, de-duplicated union:
 *       T = sort( ⋃ᵢ P_Sᵢ )   with |p₁ - p₂| < ε  → merge
 *
 *   Example:
 *       Shape A  n=3  →  {0.000, 0.333, 0.667}
 *       Shape B  n=4  →  {0.000, 0.250, 0.500, 0.750}
 *       Merged T       →  {0.000, 0.250, 0.333, 0.500, 0.667, 0.750}  (6 points)
 *
 *   Each PolyRhythmTrack has a bitmask over T. The user toggles individual points.
 *   Points on the circle UI are placed at angle = phase × 360° (non-uniform spacing).
 *
 * ── Euclidean mode ──────────────────────────────────────────────────────────────
 *
 *   Bjorklund(k, n): distribute k hits across n positions as evenly as possible.
 *   Can be applied per-shape or per-track over the full timeline.
 *
 * ── Real-time scheduling ────────────────────────────────────────────────────────
 *
 *   Phase increment per sample:
 *       Δφ = BPM / (60 · beatsPerCycle · sampleRate)
 *
 *   Each process() call:
 *       1. Compute φ_start, φ_end = φ_start + Δφ × bufferSize
 *       2. Find all timeline points in [φ_start, φ_end) (handle wrap)
 *       3. For each hit point, emit NOTE_ON/OFF with sample-accurate offset
 *       4. Advance φ
 *
 *   Timeline is rebuilt only when shapes change → O(1) per audio buffer.
 *
 * ── Thread safety notes ─────────────────────────────────────────────────────────
 *
 *   Shape/track mutations come from the UI thread via command queue.
 *   rebuildTimeline() produces a new vector; swap into audio thread atomically.
 *   In the current codebase's single-producer/single-consumer model, this is safe
 *   because commands are processed between audio callbacks.
 */

#ifndef POLYRHYTHM_MODULE_HPP
#define POLYRHYTHM_MODULE_HPP

#include "engine/module/BaseModule.hpp"
#include "engine/signal/EventSignal.hpp"
#include "engine/parameter/Parameters.hpp"
#include "engine/parameter/Param.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace Engine {
namespace Module {

// ═══════════════════════════════════════════════════════════════════════════════
//  Data structures
// ═══════════════════════════════════════════════════════════════════════════════

/// @brief A rhythmic shape: one subdivision layer in the polyrhythmic timeline.
/// @details Divides one cycle into `subdivision` equal parts, optionally shifted
/// by `offset`. Multiple shapes are merged to produce a non-uniform timeline.
struct RhythmShape {
    /// Number of equal divisions of the cycle (3 = triplets, 5 = quintuplets, …)
    uint32_t subdivision = 4;

    /// Phase offset ∈ [0.0, 1.0) — shifts all points of this shape
    double offset = 0.0;

    /// Human-readable label ("triplets", "clave", …)
    std::string name = "Shape";

    /// Euclidean mode: k hits distributed over `subdivision` positions (Bjorklund).
    /// 0 = manual mode (all positions available for the user to toggle).
    uint32_t euclideanHits = 0;
};

/// @brief A single point on the merged polyrhythmic timeline.
struct TimelinePoint {
    /// Normalized phase position ∈ [0.0, 1.0)
    double phase = 0.0;

    /// Index of the RhythmShape that generated this point
    uint32_t sourceShapeIndex = 0;

    /// Subdivision index within the source shape
    uint32_t subdivisionIndex = 0;

    bool operator<(const TimelinePoint& other) const { return phase < other.phase; }
};

/// @brief An instrument track mapping active points on the polyrhythmic timeline.
struct PolyRhythmTrack {
    /// MIDI note number
    int noteNumber = 36;

    /// MIDI velocity (0–127)
    int velocity = 100;

    /// One bool per timeline point (true = this track fires here)
    std::vector<bool> activePoints;

    /// Human-readable label
    std::string name = "Track";
};


// ═══════════════════════════════════════════════════════════════════════════════
//  Utility functions (usable outside the module)
// ═══════════════════════════════════════════════════════════════════════════════

/// @brief Greatest common divisor.
uint32_t polyGcd(uint32_t a, uint32_t b);

/// @brief Least common multiple.
uint32_t polyLcm(uint32_t a, uint32_t b);

/// @brief Bjorklund's algorithm — Euclidean rhythm generator.
/// Distributes `hits` onsets as evenly as possible across `steps` positions.
/// @return Boolean vector of size `steps` (true = onset).
std::vector<bool> bjorklund(uint32_t hits, uint32_t steps);

/// @brief Build a merged, sorted, de-duplicated timeline from a set of shapes.
/// @param shapes The rhythmic shapes to merge.
/// @param epsilon Minimum phase distance to consider two points distinct.
/// @return Sorted vector of TimelinePoints.
std::vector<TimelinePoint> buildTimeline(
    const std::vector<RhythmShape>& shapes,
    double epsilon = 1e-9);


// ═══════════════════════════════════════════════════════════════════════════════
//  PolyRhythmModule
// ═══════════════════════════════════════════════════════════════════════════════

/// @brief Polyrhythmic sequencer module.
///
/// Pipeline:
///   UI → addShape/removeShape/togglePoint → rebuildTimeline()
///     → process() reads timeline + phase accumulator → emits NOTE_ON/OFF
///       → output port → connected instrument / sampler
///
/// Comparison with StepSequencerModule:
///  ┌──────────────────────┬────────────────────────────┐
///  │ StepSequencer        │ PolyRhythm                 │
///  ├──────────────────────┼────────────────────────────┤
///  │ Uniform 16th grid    │ Non-uniform merged timeline│
///  │ Fixed step count     │ Dynamic point count        │
///  │ sampleCounter-based  │ Phase accumulator          │
///  │ Cannot mix subdiv    │ Arbitrary shape stacking   │
///  │ No Euclidean support │ Built-in Bjorklund         │
///  └──────────────────────┴────────────────────────────┘
class PolyRhythmModule : public BaseModule, public Parameters {
public:
    PolyRhythmModule();
    explicit PolyRhythmModule(std::string name);
    ~PolyRhythmModule() override;

    IModule* clone() const override;

    /// @brief Real-time audio processing.
    /// Advances phase accumulator — emits events for triggered timeline points.
    void process(Core::AudioContext& context) override;

    /// @brief Reset playhead to phase 0.
    void reset();

    // ─── Shape management (UI / command thread) ──────────────────────────────

    /// @brief Add a rhythmic shape and rebuild the timeline.
    void addShape(const RhythmShape& shape);

    /// @brief Remove a shape by index and rebuild the timeline.
    void removeShape(size_t index);

    /// @brief Replace a shape at index and rebuild the timeline.
    void updateShape(size_t index, const RhythmShape& shape);

    /// @brief Get current shapes (read-only).
    const std::vector<RhythmShape>& getShapes() const;

    // ─── Track management ────────────────────────────────────────────────────

    /// @brief Set all tracks. Resizes activePoints to match current timeline.
    void setTracks(const std::vector<PolyRhythmTrack>& tracks);

    /// @brief Get current tracks (read-only).
    const std::vector<PolyRhythmTrack>& getTracks() const;

    /// @brief Toggle one point for a given track.
    void togglePoint(size_t trackIndex, size_t pointIndex);

    /// @brief Apply Euclidean pattern (k hits) over the full merged timeline for a track.
    void applyEuclidean(size_t trackIndex, uint32_t hits);

    /// @brief Rotate a track's active points by `amount` positions (positive = clockwise).
    void rotateTrack(size_t trackIndex, int amount);

    // ─── Accessors ───────────────────────────────────────────────────────────

    const std::vector<TimelinePoint>& getTimeline() const;
    size_t getTimelineSize() const;

    double getCurrentPhase() const { return _phase; }
    float  getBPM()          const { return _bpm->get(); }
    uint32_t getBeatsPerCycle() const { return _beatsPerCycle->get(); }

    template<typename T>
    T getParameter(const std::string& name) const {
        if (name == "bpm")            return static_cast<T>(_bpm->get());
        if (name == "beatsPerCycle")  return static_cast<T>(_beatsPerCycle->get());
        return T();
    }

private:
    /// @brief Rebuild the merged timeline from _shapes and resize every track's mask.
    void rebuildTimeline();

    /// @brief Phase increment per sample: BPM / (60 · beatsPerCycle · sampleRate).
    double computePhaseIncrement(double sampleRate) const;

    // ─── Parameters ──────────────────────────────────────────────────────────
    std::shared_ptr<Param<float>>    _bpm;            ///< Tempo
    std::shared_ptr<Param<uint32_t>> _beatsPerCycle;  ///< Beats in one loop (default 4 = 1 bar)

    // ─── State ───────────────────────────────────────────────────────────────
    std::vector<RhythmShape>    _shapes;
    std::vector<TimelinePoint>  _timeline;
    std::vector<PolyRhythmTrack> _tracks;

    double _phase     = 0.0;   ///< Playhead phase ∈ [0.0, 1.0)
    bool   _firstProc = true;  ///< Flag for first process() call
};

} // namespace Module
} // namespace Engine

#endif // POLYRHYTHM_MODULE_HPP
