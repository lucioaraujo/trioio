// Trioio — cópia de RASGO/REFERENCIAS/SAIDA_TECNICA/src/Dither.h (saida-ref 1.0,
// 16/11/2026, sha256 9e8a2c60d975e99f…), namespace trioio. Proveniência completa abaixo.
// Copyright (C) 2026 Lúcio de Araújo

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace trioio
{
// TPDF dither + rounding to 16- or 24-bit PCM.
//
// Provenance: extracted on 2026-11-16 from the Aquorbium recorder
// (PROJETOS_AUTORAIS/AQUORBIUM, commit ab1da6b, src/app/DiagnosticAudioCapture.h,
// uniform() and pack(), AGPL-3.0-or-later, same author), which follows Navalha 2
// (docs/DITHER_TPDF.md). Only the quantiser is taken; the file writing stays
// with each instrument. Adapted: a class of its own, and the uniform values
// are kept in double precision (the original rounded them to float).
//
// The rule from those sources: dither belongs to the integer encoding, never
// to the float bus. Two uniform values from a xorshift32 sequence are
// subtracted per sample (triangular, +-1 LSB); successive samples take
// successive values; non-finite samples become digital silence. Measure the
// float master, and, if wanted, the decoded integers (decode()).
class TpdfQuantiser final
{
public:
    explicit TpdfQuantiser(unsigned bits = 24, std::uint32_t seed = 0x9e3779b9u) noexcept
    {
        setBits(bits);
        state = seed != 0u ? seed : 0x9e3779b9u;
    }

    // 16 or 24 (anything else becomes 24)
    void setBits(unsigned value) noexcept
    {
        depth = value == 16u ? 16u : 24u;
        full = (std::int32_t { 1 } << (depth - 1u)) - 1;
    }
    [[nodiscard]] unsigned bits() const noexcept { return depth; }

    // one float sample (+-1.0 full scale) to the signed integer to write
    [[nodiscard]] std::int32_t quantise(float x) noexcept
    {
        if (!std::isfinite(x)) x = 0.0f;
        const auto dither = uniform() - uniform();
        return static_cast<std::int32_t>(std::clamp<long>(
            std::lround(static_cast<double>(x) * static_cast<double>(full) + dither),
            -static_cast<long>(full) - 1, static_cast<long>(full)));
    }
    // the written integer read back as float, for the encoded analysis
    [[nodiscard]] float decode(std::int32_t value) const noexcept
    {
        return static_cast<float>(value) / static_cast<float>(full);
    }

private:
    double uniform() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<double>(state) / 4294967296.0;
    }

    std::uint32_t state = 0x9e3779b9u;
    unsigned depth = 24u;
    std::int32_t full = 8388607;
};
}

namespace trioio
{
// The float to hand JUCE's 24-bit WAV writer so that it writes exactly the
// integer `value` (2026-11-16). JUCE 9.0.3 turns a float into
// round(x * (2^31 - 1)) and then keeps the top 24 bits (>> 8)
// (juce_AudioFormatWriter.cpp convertFloatsToInts; AudioData::Int24
// setAsInt32LE). Half an LSB of offset puts the rounding in the middle of
// the step, so neither the float precision nor the (2^31 - 1) scale can
// move the integer (tested over the whole 24-bit range).
[[nodiscard]] inline float floatForJuceInt24(std::int32_t value) noexcept
{
    return static_cast<float>((static_cast<double>(value) + 0.5) / 8388608.0);
}
}
