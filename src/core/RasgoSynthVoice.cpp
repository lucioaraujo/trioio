#include "RasgoSynthVoice.h"

#include "AY8910Chip.hpp"
#include "ChoirEnsemble.hpp"
#include "FmFluteVoice.hpp"
#include "GaloVoice.hpp"
#include "GendynOscillator.hpp"
#include "GranularCloud.hpp"
#include "Oscillator.hpp"
#include "Rungler.hpp"
#include "ScatVoice.hpp"
#include "SpectralObject.hpp"
#include "ThereminVoice.hpp"
#include "VosimOscillator.hpp"
#include "WalshOscillator.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

namespace trioio
{
using composition::Note;
using composition::Timbre;

bool isRasgoSynthTimbre(Timbre t) noexcept
{
    switch (t)
    {
        case Timbre::gendyn:
        case Timbre::vosim:
        case Timbre::walsh:
        case Timbre::theremin:
        case Timbre::flute:
        case Timbre::scat:
        case Timbre::chip:
        case Timbre::galo:
        case Timbre::choir:
        case Timbre::rungler:
        case Timbre::granular:
        case Timbre::spectralObject:
            return true;
        default:
            return false;
    }
}

namespace
{
double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

double unit(std::uint64_t& s)
{
    s += 0x9E3779B97F4A7C15ull;
    std::uint64_t x = s;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
}

// Laço comum: `hzAt(t)` dá a frequência da nota em t (com glissando),
// `tick(hz)` produz uma amostra. Envelope: ataque curto e release na
// cauda; as vozes com envelope próprio (flauta, scat, galo) pedem `own`.
std::vector<float> run(const Note& n, double sr, double tail, bool own,
                       const std::function<float(double hz, double t)>& tick)
{
    const double total = n.duration + tail;
    const auto frames = static_cast<std::size_t>(total * sr);
    std::vector<float> out(frames, 0.0f);
    double dcIn = 0.0, dcOut = 0.0;  // bloqueador de DC (VOSIM é unipolar)
    for (std::size_t i = 0; i < frames; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = midiHz(n.pitchAt(t));
        const double x = tick(hz, t);
        dcOut = x - dcIn + 0.9995 * dcOut;
        dcIn = x;
        double env = std::min(1.0, (total - t) / 0.03);
        if (!own)
            env *= std::min(1.0, t / 0.01) * (t > n.duration ? std::max(0.0, 1.0 - (t - n.duration) / tail) : 1.0);
        out[i] = std::isfinite(dcOut) ? static_cast<float>(dcOut * env) : 0.0f;
    }
    return out;
}
} // namespace

std::vector<float> renderRasgoSynthNote(const Note& n, double sr, std::uint64_t seed)
{
    if (!isRasgoSynthTimbre(n.timbre) || n.duration <= 0.0)
        return {};
    std::uint64_t rs = seed;
    const auto s32 = static_cast<unsigned>(unit(rs) * 4294967295.0);
    switch (n.timbre)
    {
        case Timbre::gendyn:
        {
            // GENDYN (Xenakis): a forma de onda é um passeio aleatório de
            // pontos; a soma das durações dá o período. Para soar na nota,
            // a duração média de cada ponto é 1 / (f × pontos), com a faixa
            // aberta ±25 %: a altura vagueia, como na técnica original.
            rasgo::GendynOscillator g;
            g.setSampleRate(sr);
            g.seed(s32);
            const int points = 6 + static_cast<int>(unit(rs) * 6.0);
            const double avg = 1.0 / (midiHz(n.pitch) * points);
            g.configure(points, avg * 0.75, avg * 1.25);
            g.setStepScale(static_cast<float>(0.05 + 0.15 * unit(rs)), static_cast<float>(0.02 + 0.08 * unit(rs)));
            return run(n, sr, 0.15, false, [&](double, double) { return g.tick(); });
        }
        case Timbre::vosim:
        {
            rasgo::VosimOscillator v;
            v.setSampleRate(sr);
            v.setFormant(500.0 + 1500.0 * unit(rs));
            v.setPulseCount(2 + static_cast<int>(unit(rs) * 4.0));
            v.setDecay(0.6 + 0.3 * unit(rs));
            return run(n, sr, 0.12, false, [&](double hz, double) { v.setFrequency(hz); return v.tick(); });
        }
        case Timbre::walsh:
        {
            rasgo::WalshOscillator w;
            w.setSampleRate(sr);
            std::vector<double> weights(12);
            for (std::size_t k = 0; k < weights.size(); ++k)
                weights[k] = (unit(rs) * 2.0 - 1.0) / (1.0 + 0.5 * static_cast<double>(k));
            w.configure(weights);
            return run(n, sr, 0.12, false, [&](double hz, double) { w.setFrequency(hz); return w.tick(); });
        }
        case Timbre::theremin:
        {
            // o teremim nunca salta: desliza até a nota (glide próprio)
            rasgo::ThereminVoice th;
            th.setSampleRate(sr);
            th.configure(0.04 + 0.12 * unit(rs), 4.5 + 2.0 * unit(rs), static_cast<float>(0.15 + 0.25 * unit(rs)), s32);
            th.setGain(1.0f);
            return run(n, sr, 0.25, false, [&](double hz, double) { th.setTargetFrequency(hz); return th.tick(); });
        }
        case Timbre::flute:
        {
            rasgo::FmFluteVoice f;
            f.setSampleRate(sr);
            f.configure(0.99 + 0.02 * unit(rs), 2.5 + 2.0 * unit(rs), 0.4 + 0.4 * unit(rs), 0.04, 0.18);
            f.setGain(1.0f);
            f.noteOn();
            bool off = false;
            return run(n, sr, 0.3, true, [&](double hz, double t) {
                if (!off && t >= n.duration) { f.noteOff(); off = true; }
                f.setFrequency(hz);
                return f.tick();
            });
        }
        case Timbre::scat:
        {
            rasgo::ScatVoice sc;
            sc.setSampleRate(sr);
            sc.seed(s32);
            sc.beginSyllable(midiHz(n.pitch), n.duration, static_cast<int>(unit(rs) * 5.0));
            return run(n, sr, 0.1, true, [&](double, double) { return sc.tick(); });
        }
        case Timbre::chip:
        {
            // AY-3-8910: o tom alterna a cada ciclo da sua frequência, então a
            // onda quadrada sai em freq/2 — pede-se 2 × a nota. Canal 2 uma
            // oitava acima, levemente desafinado; canal 3 desligado (volume 0).
            rasgo::AY8910Chip c;
            c.setSampleRate(sr);
            c.setVolume(0, 0.8f);
            c.setVolume(1, static_cast<float>(0.25 + 0.3 * unit(rs)));
            c.setVolume(2, 0.0f);
            c.setNoiseEnabled(0, false);
            c.setNoiseEnabled(1, false);
            c.setNoiseEnabled(2, false);
            const double detune = 1.0 + 0.004 * unit(rs);
            return run(n, sr, 0.06, false, [&](double hz, double) {
                c.setToneFrequency(0, 2.0 * hz);
                c.setToneFrequency(1, 4.0 * hz * detune);
                return c.tick();
            });
        }
        case Timbre::galo:
        {
            // corpo modal soprado com o contorno de chamado do próprio Galo
            rasgo::GaloVoice g;
            g.setSampleRate(sr);
            std::array<double, rasgo::ModalResonator::kNumModes> ratios {}, decays {}, gains {};
            const double stretch = 0.02 + 0.08 * unit(rs);
            for (std::size_t k = 0; k < ratios.size(); ++k)
            {
                const double h = static_cast<double>(k + 1);
                ratios[k] = h * std::sqrt(1.0 + stretch * h * h);
                decays[k] = 0.6 / (1.0 + 0.4 * h);
                gains[k] = 1.0 / h;
            }
            g.configure(s32, ratios, decays, gains);
            g.trigger(midiHz(n.pitch), n.duration);
            return run(n, sr, 0.35, true, [&](double, double) { return g.tick(); });
        }
        case Timbre::choir:
        {
            // quatro vozes de vogal num acorde em volta da nota (o acorde é
            // sorteado entre quatro cores); cada voz entra com um atraso
            // próprio, como num coro de verdade
            rasgo::ChoirEnsemble c;
            c.setSampleRate(sr);
            c.seed(s32);
            static const std::array<std::array<int, 4>, 4> chords { { { 0, 4, 7, 11 }, { 0, 3, 7, 10 },
                                                                       { 0, 5, 7, 10 }, { 0, 5, 10, 15 } } };
            c.triggerPhrase(static_cast<int>(std::lround(n.pitch)), chords[static_cast<std::size_t>(unit(rs) * 3.999)],
                            static_cast<int>(unit(rs) * 5.0), n.duration);
            return run(n, sr, 0.15, true, [&](double, double) { return c.tick(); });
        }
        case Timbre::rungler:
        {
            // o Rungler (registrador de deslocamento caótico, Rob Hordijk)
            // faz a altura saltar em degraus em volta da nota: a nota é o
            // centro, não a melodia
            rasgo::Rungler rg;
            rg.setSampleRate(sr);
            rg.setRates(4.0 + 6.0 * unit(rs), 6.0 + 9.0 * unit(rs));
            rg.setFeedbackDepth(0.2 + 0.5 * unit(rs));
            rg.reset(static_cast<std::uint8_t>(1 + unit(rs) * 254.0));
            rasgo::Oscillator o;
            o.setSampleRate(sr);
            o.setWaveform(unit(rs) < 0.5 ? rasgo::Oscillator::Waveform::Saw : rasgo::Oscillator::Waveform::Pulse);
            const double span = 3.0 + 9.0 * unit(rs);  // semitons para cada lado
            return run(n, sr, 0.1, false, [&](double hz, double) {
                const double step = std::round(rg.next() * span);
                o.setFrequency(hz * std::pow(2.0, step / 12.0));
                return o.tick();
            });
        }
        case Timbre::granular:
        {
            // o tom da nota (senoide + quinta suave) alimenta a nuvem: os grãos
            // relêem o passado recente com desvio de altura
            rasgo::GranularCloud g;
            g.setSampleRate(sr, 1.0);
            g.seed(s32);
            g.setDensity(18.0 + 40.0 * unit(rs));
            g.setGrainDuration(0.02, 0.06 + 0.1 * unit(rs));
            g.setSpread(0.05 + 0.3 * unit(rs));
            g.setPitchJitter(0.1 + 1.5 * unit(rs));
            g.setMix(1.0f);
            double ph1 = 0.0, ph2 = 0.0;
            return run(n, sr, 0.4, false, [&](double hz, double t) {
                ph1 += hz / sr;
                ph2 += 1.5 * hz / sr;
                const double feed = t < n.duration ? std::sin(6.283185307 * ph1) + 0.3 * std::sin(6.283185307 * ph2) : 0.0;
                return g.process(static_cast<float>(0.5 * feed));
            });
        }
        case Timbre::spectralObject:
        {
            // uma só proporção numérica vira altura, ritmo e timbre: cada
            // parcial (admitido por um crivo de Xenakis) pulsa num período
            // proporcional ao seu índice
            rasgo::SpectralObject o;
            o.setSampleRate(sr);
            o.configure(static_cast<rasgo::SpectralObject::PartialSet>(static_cast<int>(unit(rs) * 2.999)),
                        midiHz(n.pitch), s32);
            o.setInharmonicity(0.0005 * unit(rs));
            o.setBasePeriodSamples(static_cast<std::size_t>(sr * (0.12 + 0.3 * unit(rs))));
            o.setGain(1.0f);
            return run(n, sr, 0.3, false, [&](double, double) { return o.tick(); });
        }
        default:
            break;
    }
    return {};
}

} // namespace trioio
