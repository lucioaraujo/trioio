#include "ModularVoice.h"

#include <algorithm>
#include <cmath>

#ifdef TRIOIO_HAS_MODULAR
#include "core/SignalGraph.hpp"
#include "dsp/Drum.hpp"
#include "dsp/Formant.hpp"
#include "dsp/Hall.hpp"
#include "dsp/Lpg.hpp"
#include "dsp/Matter.hpp"
#include "dsp/Operator.hpp"
#include "dsp/Oscillator.hpp"
#include "dsp/Pulsar.hpp"
#include "dsp/Resonator.hpp"
#include "dsp/Shape.hpp"
#include "dsp/Space.hpp"
#include "dsp/StringVoice.hpp"
#include "dsp/Wavetable.hpp"

#include "NoiseFields.h"  // Antitotem: o sopro do ressonador

#include <memory>
#include <string>
#endif

namespace trioio
{
using composition::Note;
using composition::Timbre;

bool isModularTimbre(Timbre t) noexcept
{
    switch (t)
    {
        case Timbre::wavetable:
        case Timbre::pulsar:
        case Timbre::modal:
        case Timbre::waveguide:
        case Timbre::drum:
        case Timbre::vocal:
        case Timbre::folded:
        case Timbre::resonator:
        case Timbre::operatorFm:
        case Timbre::lpg:
            return true;
        default:
            return false;
    }
}

#ifndef TRIOIO_HAS_MODULAR

bool modularAvailable() noexcept { return false; }
std::vector<float> renderModularNote(const Note&, double, std::uint64_t) { return {}; }
bool applyModularRoom(composition::Room, double, std::vector<float>&, std::vector<float>&, std::uint64_t) { return false; }

#else

bool modularAvailable() noexcept { return true; }

namespace
{
using namespace rasgo::modular;

constexpr std::size_t kBlock = 256;  // AudioBlock::maxFrames

double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

// um número 0..1 estável por nota (seed), para variar a matéria de nota a nota
double unit(std::uint64_t& s)
{
    s += 0x9E3779B97F4A7C15ull;
    std::uint64_t x = s;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
}

// Um módulo com os seus buffers: entradas (nullptr = desconectada) e saídas.
struct Stage
{
    std::unique_ptr<Signal> module;
    std::vector<AudioBlock> in;          // um por porta de entrada
    std::vector<const AudioBlock*> ptrs; // o que vai para process()
    std::vector<AudioBlock> out;

    Stage(std::unique_ptr<Signal> m, float sr) : module(std::move(m))
    {
        module->prepare(sr, kBlock);
        in.assign(module->inputCount(), AudioBlock(sr, 1, kBlock));
        ptrs.assign(module->inputCount(), nullptr);
        out.assign(module->outputCount(), AudioBlock(sr, 1, kBlock));
    }
    int port(const char* name) const
    {
        for (std::size_t i = 0; i < module->inputCount(); ++i)
            if (module->inputDescriptor(i).name == name)
                return static_cast<int>(i);
        return -1;
    }
    void set(const char* id, float v) { module->setParameter(id, v); }
    void run() { module->process(ptrs, out); }
};

// O que cada família é, em módulos do Rasgo Modular.
struct Body
{
    explicit Body(Stage s) : src(std::move(s)) {}
    Stage src;
    std::unique_ptr<Stage> proc;  // processador opcional (FORMANT, SHAPE)
    int srcOut = 0;
    int trigPort = -1;            // golpe/pluck/gate no início da nota
    int breathPort = -1;          // entrada de áudio soprada por ruído (ressonador)
    int procStrike = -1;          // golpe no processador (LPG)
    const char* freqParam = "freq";
    bool percussive = false;
    double tail = 0.15;           // quanto soa depois do fim da nota
};

Body makeBody(Timbre t, const Note& n, float sr, std::uint64_t& rs)
{
    const float vel = n.velocity;
    switch (t)
    {
        case Timbre::wavetable:
        {
            Body b { Stage(std::make_unique<Wavetable>(), sr) };
            b.src.set("pos", static_cast<float>(unit(rs)));
            b.src.set("warp", static_cast<float>(0.15 + 0.4 * unit(rs)));
            b.src.set("drift", 0.15f);
            b.tail = 0.18;
            return b;
        }
        case Timbre::pulsar:
        {
            Body b { Stage(std::make_unique<Pulsar>(), sr) };
            b.src.set("formant", static_cast<float>(1.5 + 3.0 * unit(rs)));
            b.src.set("window", 0.45f);
            b.src.set("jitter", 0.06f);
            b.src.set("spread", 0.2f);
            b.src.set("level", 0.8f);
            b.tail = 0.12;
            return b;
        }
        case Timbre::modal:
        {
            Body b { Stage(std::make_unique<Matter>(), sr) };
            b.src.set("structure", static_cast<float>(0.15 + 0.7 * unit(rs)));
            b.src.set("brightness", 0.35f + 0.45f * vel);
            b.src.set("damping", static_cast<float>(0.25 + 0.25 * unit(rs)));
            b.src.set("position", static_cast<float>(0.08 + 0.3 * unit(rs)));
            b.trigPort = b.src.port("strike");
            b.percussive = true;
            b.tail = 2.5;
            return b;
        }
        case Timbre::waveguide:
        {
            Body b { Stage(std::make_unique<StringVoice>(), sr) };
            b.src.set("decay", 0.88f);
            b.src.set("damping", static_cast<float>(0.25 + 0.3 * unit(rs)));
            b.src.set("position", static_cast<float>(0.06 + 0.3 * unit(rs)));
            b.src.set("exciter", 0.5f);
            b.src.set("drive", 0.12f);
            b.trigPort = b.src.port("pluck");
            b.percussive = true;
            b.tail = 2.0;
            return b;
        }
        case Timbre::drum:
        {
            Body b { Stage(std::make_unique<Drum>(), sr) };
            b.freqParam = "tone";
            b.src.set("bend", static_cast<float>(0.3 + 0.5 * unit(rs)));
            b.src.set("decay", 0.35f + 0.3f * vel);
            b.src.set("snap", static_cast<float>(0.15 + 0.4 * unit(rs)));
            b.trigPort = b.src.port("gate");
            b.percussive = true;
            b.tail = 1.2;
            return b;
        }
        case Timbre::vocal:
        {
            Body b { Stage(std::make_unique<Oscillator>(), sr) };
            b.srcOut = 2;  // dente de serra: rico para as formantes
            b.proc = std::make_unique<Stage>(std::make_unique<Formant>(), sr);
            b.proc->set("res", 0.55f);
            b.tail = 0.15;
            return b;
        }
        case Timbre::folded:
        {
            Body b { Stage(std::make_unique<Oscillator>(), sr) };
            b.srcOut = 0;  // senoide: a dobra é que cria o espectro
            b.proc = std::make_unique<Stage>(std::make_unique<Shape>(), sr);
            b.proc->set("symmetry", static_cast<float>(-0.4 + 0.8 * unit(rs)));
            b.proc->set("sat", 0.2f);
            b.tail = 0.15;
            return b;
        }
        case Timbre::resonator:
        {
            // o banco de modos do Rasgo Modular arqueado pelo ruído rosa do
            // Antitotem: um corpo que soa enquanto é soprado
            Body b { Stage(std::make_unique<Resonator>(), sr) };
            // estrutura pouco esticada: com mais, o modo mais forte deixa de
            // ser a fundamental e a nota some (medido: 8 semitons acima)
            b.src.set("structure", static_cast<float>(0.03 + 0.2 * unit(rs)));
            b.src.set("partials", static_cast<float>(8 + static_cast<int>(unit(rs) * 12.0)));
            b.src.set("decay", 0.75f);
            b.src.set("damp", static_cast<float>(0.2 + 0.4 * unit(rs)));
            b.src.set("position", static_cast<float>(0.15 + 0.3 * unit(rs)));
            b.srcOut = 0;  // low: a região da fundamental
            b.trigPort = b.src.port("strike");
            b.breathPort = b.src.port("in");
            b.tail = 0.6;
            return b;
        }
        case Timbre::operatorFm:
        {
            // FM de 4 operadores: algoritmo e razões sorteados por nota,
            // índice pela dinâmica (forte = mais brilho)
            Body b { Stage(std::make_unique<Operator>(), sr) };
            b.src.set("algo", static_cast<float>(static_cast<int>(unit(rs) * 7.999)));
            static const float ratios[] = { 0.5f, 1.0f, 1.41f, 2.0f, 3.0f, 3.5f, 5.0f, 7.0f };
            b.src.set("ratio_b", ratios[static_cast<std::size_t>(unit(rs) * 7.999)]);
            b.src.set("ratio_c", ratios[static_cast<std::size_t>(unit(rs) * 7.999)]);
            b.src.set("ratio_d", ratios[static_cast<std::size_t>(unit(rs) * 7.999)]);
            b.src.set("index", 0.15f + 0.45f * vel);
            b.src.set("feedback", static_cast<float>(0.25 * unit(rs)));
            b.tail = 0.15;
            return b;
        }
        case Timbre::lpg:
        {
            // dente de serra ou triângulo num LPG golpeado: o "bongô" de
            // vactrol (Buchla 292), brilho e volume caindo juntos
            Body b { Stage(std::make_unique<Oscillator>(), sr) };
            b.srcOut = unit(rs) < 0.5 ? 2 : 1;  // serra ou triângulo
            b.proc = std::make_unique<Stage>(std::make_unique<Lpg>(), sr);
            b.proc->set("mode", 0.5f);
            b.proc->set("response", static_cast<float>(0.25 + 0.5 * unit(rs)));
            b.proc->set("resonance", static_cast<float>(0.1 + 0.4 * unit(rs)));
            b.proc->set("bounce", static_cast<float>(0.4 * unit(rs)));
            b.procStrike = b.proc->port("strike");
            b.percussive = true;
            b.tail = 1.2;
            return b;
        }
        default:
            break;
    }
    return Body { Stage(std::make_unique<Oscillator>(), sr) };
}
} // namespace

std::vector<float> renderModularNote(const Note& n, double sampleRate, std::uint64_t seed)
{
    if (!isModularTimbre(n.timbre) || n.duration <= 0.0)
        return {};
    const float sr = static_cast<float>(sampleRate);
    std::uint64_t rs = seed;
    Body b = makeBody(n.timbre, n, sr, rs);
    const auto& fdesc = b.src.module->parameters();
    float fMin = 8.0f, fMax = 8000.0f;
    for (const auto& p : fdesc)
        if (p.descriptor.id == b.freqParam) { fMin = p.descriptor.minimum; fMax = p.descriptor.maximum; }

    // movimentos de cada família ao longo da nota
    const double vowelA = unit(rs), vowelB = unit(rs);  // ditongo
    const double foldA = 0.25 + 0.55 * n.velocity, foldB = foldA * (0.3 + 0.5 * unit(rs));

    const double total = n.duration + b.tail;
    const auto frames = static_cast<std::size_t>(total * sampleRate);
    std::vector<float> out(frames, 0.0f);
    const auto trigFrames = static_cast<std::size_t>(0.005 * sampleRate);
    antitotem::NoisePalette breath(static_cast<std::uint32_t>(seed | 1u));
    breath.setColour(antitotem::NoisePalette::Colour::pink);
    for (std::size_t i0 = 0; i0 < frames; i0 += kBlock)
    {
        const double t = static_cast<double>(i0) / sampleRate;
        const double prog = std::min(1.0, t / std::max(n.duration, 1e-3));
        const double hz = midiHz(n.pitchAt(t));
        b.src.set(b.freqParam, std::clamp(static_cast<float>(hz), fMin, fMax));
        if (b.trigPort >= 0)
        {
            auto& blk = b.src.in[static_cast<std::size_t>(b.trigPort)];
            for (std::size_t k = 0; k < kBlock; ++k)
                blk.at(0, k) = (i0 + k) < trigFrames ? 1.0f : 0.0f;
            b.src.ptrs[static_cast<std::size_t>(b.trigPort)] = &blk;
        }
        if (b.breathPort >= 0)
        {
            // sopro: sobe em 60 ms, sustenta na nota, cai na cauda
            auto& blk = b.src.in[static_cast<std::size_t>(b.breathPort)];
            for (std::size_t k = 0; k < kBlock; ++k)
            {
                const double tt = static_cast<double>(i0 + k) / sampleRate;
                const double env = std::min(1.0, tt / 0.06) * (tt > n.duration ? std::max(0.0, 1.0 - (tt - n.duration) / 0.15) : 1.0);
                blk.at(0, k) = static_cast<float>(0.5 * env * breath.tick());
            }
            b.src.ptrs[static_cast<std::size_t>(b.breathPort)] = &blk;
        }
        b.src.run();
        const AudioBlock* sig = &b.src.out[static_cast<std::size_t>(b.srcOut)];
        if (b.proc)
        {
            if (n.timbre == Timbre::vocal)
                b.proc->set("vowel", static_cast<float>(vowelA + (vowelB - vowelA) * prog));
            else if (n.timbre == Timbre::folded)
                b.proc->set("fold", static_cast<float>(foldA + (foldB - foldA) * prog));
            if (b.procStrike >= 0)
            {
                auto& blk = b.proc->in[static_cast<std::size_t>(b.procStrike)];
                for (std::size_t k = 0; k < kBlock; ++k)
                    blk.at(0, k) = (i0 + k) < trigFrames ? 1.0f : 0.0f;
                b.proc->ptrs[static_cast<std::size_t>(b.procStrike)] = &blk;
            }
            b.proc->ptrs[0] = sig;
            b.proc->run();
            sig = &b.proc->out[0];
        }
        for (std::size_t k = 0; k < kBlock && i0 + k < frames; ++k)
        {
            const double tt = static_cast<double>(i0 + k) / sampleRate;
            double env;
            if (b.percussive)  // o corpo decai sozinho; só um fade no fim da cauda
                env = std::min(1.0, (total - tt) / 0.05);
            else               // sustentadas: ataque curto, release na cauda
                env = std::min(1.0, tt / 0.008) * (tt > n.duration ? std::max(0.0, 1.0 - (tt - n.duration) / b.tail) : 1.0);
            const float v = sig->at(0, k);
            out[i0 + k] = std::isfinite(v) ? static_cast<float>(v * env) : 0.0f;
        }
    }
    return out;
}

bool applyModularRoom(composition::Room room, double sampleRate, std::vector<float>& L, std::vector<float>& R,
                      std::uint64_t seed)
{
    if (room == composition::Room::trioio || L.empty())
        return false;
    const float sr = static_cast<float>(sampleRate);
    std::uint64_t rs = seed;
    std::unique_ptr<Signal> m;
    int wetL = 0, wetR = 1;
    float send = 0.35f;
    if (room == composition::Room::hall)
    {
        auto h = std::make_unique<Hall>();
        h->setParameter("size", static_cast<float>(0.5 + 0.4 * unit(rs)));
        h->setParameter("decay", static_cast<float>(0.45 + 0.35 * unit(rs)));
        h->setParameter("damp", static_cast<float>(0.3 + 0.3 * unit(rs)));
        h->setParameter("mod", 0.25f);
        h->setParameter("mix", 1.0f);  // só o molhado: a mistura é feita aqui
        m = std::move(h);
    }
    else
    {
        auto s = std::make_unique<Space>();
        s->setParameter("time", static_cast<float>(0.12 + 0.35 * unit(rs)));
        s->setParameter("taps", static_cast<float>(3 + static_cast<int>(unit(rs) * 5.0)));
        s->setParameter("spread", 0.7f);
        s->setParameter("feedback", static_cast<float>(0.25 + 0.3 * unit(rs)));
        s->setParameter("diffusion", static_cast<float>(0.4 + 0.4 * unit(rs)));
        wetL = 1;
        wetR = 1;  // SPACE é mono: o molhado vai aos dois lados, levemente defasado
        send = 0.3f;
        m = std::move(s);
    }
    Stage st(std::move(m), sr);
    AudioBlock in(sr, 1, kBlock);
    st.ptrs[0] = &in;
    std::vector<float> wl(L.size(), 0.0f), wr(L.size(), 0.0f);
    for (std::size_t i0 = 0; i0 < L.size(); i0 += kBlock)
    {
        for (std::size_t k = 0; k < kBlock; ++k)
            in.at(0, k) = i0 + k < L.size() ? 0.5f * (L[i0 + k] + R[i0 + k]) : 0.0f;
        st.run();
        for (std::size_t k = 0; k < kBlock && i0 + k < L.size(); ++k)
        {
            const float l = st.out[static_cast<std::size_t>(wetL)].at(0, k);
            const float r = st.out[static_cast<std::size_t>(wetR)].at(0, k);
            wl[i0 + k] = std::isfinite(l) ? l : 0.0f;
            wr[i0 + k] = std::isfinite(r) ? r : 0.0f;
        }
    }
    const std::size_t lag = room == composition::Room::space ? static_cast<std::size_t>(0.011 * sampleRate) : 0;
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        L[i] += send * wl[i];
        R[i] += send * (i >= lag ? wr[i - lag] : 0.0f);
    }
    return true;
}

#endif

} // namespace trioio
