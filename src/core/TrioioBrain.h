#pragma once

#include <atomic>
#include <cstdint>

namespace trioio
{

struct BrainDecision
{
    float energy = 0.0f;
    float density = 0.0f;
    float harmonicity = 0.5f;
    float brightness = 0.5f;
    float transientRetention = 0.5f;
    float memory = 0.0f;
    float mutation = 0.0f;
    float spatialActivity = 0.0f;
    float autonomy = 1.0f;
    float confidence = 0.0f;
    std::uint64_t revision = 0;
};

[[nodiscard]] BrainDecision sanitizeDecision(BrainDecision decision) noexcept;

// Cross-thread contract: the analysis/strategy side publishes complete scalar
// snapshots; the audio side reads one coherent snapshot without touching
// mutable state. The sequence counter is a seqlock: scalar atomics avoid data
// races, while the reader retries when a publication overlaps its copy.
class EnergyControlBus final
{
public:
    void publish(BrainDecision decision) noexcept;
    [[nodiscard]] BrainDecision read() const noexcept;

private:
    std::atomic<std::uint64_t> sequence_ { 0 };
    std::atomic<float> energy_ { 0.0f };
    std::atomic<float> density_ { 0.0f };
    std::atomic<float> harmonicity_ { 0.5f };
    std::atomic<float> brightness_ { 0.5f };
    std::atomic<float> transientRetention_ { 0.5f };
    std::atomic<float> memory_ { 0.0f };
    std::atomic<float> mutation_ { 0.0f };
    std::atomic<float> spatialActivity_ { 0.0f };
    std::atomic<float> autonomy_ { 1.0f };
    std::atomic<float> confidence_ { 0.0f };
    std::atomic<std::uint64_t> revision_ { 0 };
};

} // namespace trioio
