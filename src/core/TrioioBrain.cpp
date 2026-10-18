#include "TrioioBrain.h"

#include <algorithm>
#include <cmath>

namespace trioio
{
namespace
{
float finiteUnit(float value, float fallback) noexcept
{
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : fallback;
}
}

BrainDecision sanitizeDecision(BrainDecision decision) noexcept
{
    decision.energy = finiteUnit(decision.energy, 0.0f);
    decision.density = finiteUnit(decision.density, 0.0f);
    decision.harmonicity = finiteUnit(decision.harmonicity, 0.5f);
    decision.brightness = finiteUnit(decision.brightness, 0.5f);
    decision.transientRetention = finiteUnit(decision.transientRetention, 0.5f);
    decision.memory = finiteUnit(decision.memory, 0.0f);
    decision.mutation = finiteUnit(decision.mutation, 0.0f);
    decision.spatialActivity = finiteUnit(decision.spatialActivity, 0.0f);
    decision.autonomy = finiteUnit(decision.autonomy, 1.0f);
    decision.confidence = finiteUnit(decision.confidence, 0.0f);
    return decision;
}

void EnergyControlBus::publish(BrainDecision decision) noexcept
{
    decision = sanitizeDecision(decision);
    // A single analysis/strategy writer is expected. Odd means that a write
    // is in progress; the release increment publishes the complete snapshot.
    sequence_.fetch_add(1, std::memory_order_relaxed);
    // the release fence keeps the data stores below AFTER the odd count: on a
    // weakly ordered CPU (ARM, the Apple Silicon CI runner, 2026-10-19) they
    // could otherwise become visible before it and a reader would accept a
    // torn snapshot. H.-J. Boehm, "Can Seqlocks Get Along with Programming
    // Language Memory Models?", MSPC 2012.
    std::atomic_thread_fence(std::memory_order_release);
    energy_.store(decision.energy, std::memory_order_relaxed);
    density_.store(decision.density, std::memory_order_relaxed);
    harmonicity_.store(decision.harmonicity, std::memory_order_relaxed);
    brightness_.store(decision.brightness, std::memory_order_relaxed);
    transientRetention_.store(decision.transientRetention, std::memory_order_relaxed);
    memory_.store(decision.memory, std::memory_order_relaxed);
    mutation_.store(decision.mutation, std::memory_order_relaxed);
    spatialActivity_.store(decision.spatialActivity, std::memory_order_relaxed);
    autonomy_.store(decision.autonomy, std::memory_order_relaxed);
    confidence_.store(decision.confidence, std::memory_order_relaxed);
    revision_.store(decision.revision, std::memory_order_release);
    sequence_.fetch_add(1, std::memory_order_release);
}

BrainDecision EnergyControlBus::read() const noexcept
{
    BrainDecision last;

    // Keep the retry bound finite: this function may run on the audio thread.
    // Under ordinary operation the first attempt succeeds. If a writer remains
    // active for the whole bounded window, return a known coherent baseline;
    // returning the partially copied fields would be musically and logically
    // worse than a short, explicit loss of the newest decision.
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const auto before = sequence_.load(std::memory_order_acquire);
        if ((before & 1u) != 0u)
            continue;

        BrainDecision decision;
        decision.energy = energy_.load(std::memory_order_relaxed);
        decision.density = density_.load(std::memory_order_relaxed);
        decision.harmonicity = harmonicity_.load(std::memory_order_relaxed);
        decision.brightness = brightness_.load(std::memory_order_relaxed);
        decision.transientRetention = transientRetention_.load(std::memory_order_relaxed);
        decision.memory = memory_.load(std::memory_order_relaxed);
        decision.mutation = mutation_.load(std::memory_order_relaxed);
        decision.spatialActivity = spatialActivity_.load(std::memory_order_relaxed);
        decision.autonomy = autonomy_.load(std::memory_order_relaxed);
        decision.confidence = confidence_.load(std::memory_order_relaxed);
        decision.revision = revision_.load(std::memory_order_relaxed);

        // the acquire fence keeps the data loads above BEFORE the second
        // count: an acquire load alone does not stop earlier relaxed loads
        // from moving after it (Boehm 2012)
        std::atomic_thread_fence(std::memory_order_acquire);
        const auto after = sequence_.load(std::memory_order_relaxed);
        last = decision;
        if (before == after && (after & 1u) == 0u)
            return sanitizeDecision(decision);
    }

    (void) last;
    return BrainDecision {};
}

} // namespace trioio
