#include "TrioioBrain.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <thread>

int main()
{
    trioio::BrainDecision input;
    input.energy = std::numeric_limits<float>::quiet_NaN();
    input.density = 2.0f;
    input.harmonicity = -1.0f;
    input.autonomy = std::numeric_limits<float>::infinity();
    input.revision = 7;

    const auto safe = trioio::sanitizeDecision(input);
    assert(safe.energy == 0.0f);
    assert(safe.density == 1.0f);
    assert(safe.harmonicity == 0.0f);
    assert(safe.autonomy == 1.0f);

    trioio::EnergyControlBus bus;
    bus.publish(input);
    const auto snapshot = bus.read();
    assert(snapshot.energy == 0.0f);
    assert(snapshot.density == 1.0f);
    assert(snapshot.revision == 7);
    assert(std::isfinite(snapshot.confidence));

    // Every published decision uses one marker for all fields. A concurrent
    // reader must observe a complete marker, never a mixture of two writes.
    trioio::EnergyControlBus concurrentBus;
    std::atomic<bool> writing { true };
    std::thread writer([&]
    {
        for (std::uint64_t revision = 1; revision <= 20000; ++revision)
        {
            const auto marker = static_cast<float>(revision & 1u);
            trioio::BrainDecision decision;
            decision.energy = marker;
            decision.density = marker;
            decision.harmonicity = marker;
            decision.brightness = marker;
            decision.transientRetention = marker;
            decision.memory = marker;
            decision.mutation = marker;
            decision.spatialActivity = marker;
            decision.autonomy = marker;
            decision.confidence = marker;
            decision.revision = revision;
            concurrentBus.publish(decision);
            // Model the analysis thread yielding between decision frames. It
            // also makes the test exercise the reader's retry path without
            // turning a unit test into an artificial writer starvation case.
            if ((revision & 63u) == 0u)
                std::this_thread::yield();
        }
        writing.store(false, std::memory_order_release);
    });

    while (writing.load(std::memory_order_acquire))
    {
        const auto observed = concurrentBus.read();
        if (observed.revision == 0)
            continue; // bounded seqlock fallback: coherent baseline
        const auto marker = static_cast<float>(observed.revision & 1u);
        assert(observed.energy == marker);
        assert(observed.density == marker);
        assert(observed.harmonicity == marker);
        assert(observed.brightness == marker);
        assert(observed.confidence == marker);
    }
    writer.join();
    const auto finalSnapshot = concurrentBus.read();
    assert(finalSnapshot.revision == 20000);
    assert(finalSnapshot.energy == 0.0f);
    assert(finalSnapshot.harmonicity == 0.0f);
    return 0;
}
