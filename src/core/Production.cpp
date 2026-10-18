#include "Production.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace trioio
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

// biquad (RBJ), forma direta transposta
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void highpass(double hz, double sr, double q = 0.7071)
    {
        const double w = 2.0 * kPi * hz / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q), a0 = 1.0 + al;
        b0 = (1.0 + c) / 2.0 / a0;
        b1 = -(1.0 + c) / a0;
        b2 = b0;
        a1 = -2.0 * c / a0;
        a2 = (1.0 - al) / a0;
    }
    void peak(double hz, double sr, double q, double db)
    {
        const double A = std::pow(10.0, db / 40.0), w = 2.0 * kPi * hz / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q);
        const double a0 = 1.0 + al / A;
        b0 = (1.0 + al * A) / a0;
        b1 = -2.0 * c / a0;
        b2 = (1.0 - al * A) / a0;
        a1 = -2.0 * c / a0;
        a2 = (1.0 - al / A) / a0;
    }
    void highShelf(double hz, double sr, double db)
    {
        const double A = std::pow(10.0, db / 40.0), w = 2.0 * kPi * hz / sr, c = std::cos(w), s = std::sin(w);
        const double al = s / 2.0 * std::sqrt(2.0), sq = 2.0 * std::sqrt(A) * al;
        const double a0 = (A + 1) - (A - 1) * c + sq;
        b0 = A * ((A + 1) + (A - 1) * c + sq) / a0;
        b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0;
        b2 = A * ((A + 1) + (A - 1) * c - sq) / a0;
        a1 = 2 * ((A - 1) - (A + 1) * c) / a0;
        a2 = ((A + 1) - (A - 1) * c - sq) / a0;
    }
};

struct BusSpec
{
    double hp;          // passa-altas (Hz; 0 = nenhum)
    double mudDb;       // corte em 300 Hz
    double presenceDb;  // 3 kHz
    double airDb;       // prateleira em 9 kHz (o fundo perde brilho)
    double ratio;       // compressão
    double attack, release;  // s
    double width;       // 1 = como veio; > 1 abre; < 1 fecha
    double send;        // envio para a sala
    double gain;        // nível do barramento
};

BusSpec specOf(int group)
{
    switch (group)
    {
        case 0: return { 90.0, 0.0, 2.0, 1.0, 2.5, 0.008, 0.12, 1.0, 0.16, 1.0 };     // melodia: na frente
        case 1: return { 30.0, 0.0, 1.0, -2.0, 3.0, 0.015, 0.2, 0.0, 0.04, 1.0 };     // baixo: centro, seco
        case 2: return { 150.0, -2.5, 0.0, -1.5, 1.8, 0.02, 0.25, 1.5, 0.32, 0.85 };  // harmonia e camadas: largas, ao fundo
        case 3: return { 30.0, -1.5, 1.0, 1.0, 1.8, 0.012, 0.1, 1.2, 0.12, 0.95 };    // percussão
        default: return { 40.0, 0.0, 0.0, 0.0, 1.0, 0.02, 0.2, 1.3, 0.35, 0.9 };      // efeitos
    }
}

// compressor de pico com detecção RMS curta, limiar relativo ao nível do barramento
void compress(std::vector<float>& L, std::vector<float>& R, double sr, double ratio, double attack, double release)
{
    if (ratio <= 1.0 || L.empty())
        return;
    double pow2 = 0.0;
    std::size_t n = 0;
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        const double v = 0.5 * (L[i] * L[i] + R[i] * R[i]);
        if (v > 1e-10)
        {
            pow2 += v;
            ++n;
        }
    }
    if (n == 0)
        return;
    const double rmsDb = 10.0 * std::log10(pow2 / static_cast<double>(n));
    const double thr = rmsDb + 4.0;  // o limiar 4 dB acima do nível médio: segura os picos, não esmaga
    const double aA = std::exp(-1.0 / (attack * sr)), aR = std::exp(-1.0 / (release * sr));
    double env = 0.0;
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        const double v = 0.5 * (L[i] * L[i] + R[i] * R[i]);
        env = v > env ? aA * env + (1.0 - aA) * v : aR * env + (1.0 - aR) * v;
        const double db = 10.0 * std::log10(env + 1e-12);
        const double over = db - thr;
        if (over > 0.0)
        {
            const double g = std::pow(10.0, -over * (1.0 - 1.0 / ratio) / 20.0);
            L[i] = static_cast<float>(L[i] * g);
            R[i] = static_cast<float>(R[i] * g);
        }
    }
}
} // namespace

RoomProfile roomForEnsemble(int e)
{
    // a ordem de composition::Ensemble
    switch (e)
    {
        case 1: case 2: case 9: case 13: return { "sala de choro", 1.1, 0.8, 0.012, 0.4, 0.9 };    // choro, samba, salsa, forró
        case 3: case 4: case 5: case 14: return { "clube", 0.9, 0.6, 0.008, 0.5, 0.85 };          // jazz, manouche, bolero, tango
        case 6: return { "salão", 1.2, 0.9, 0.014, 0.4, 0.9 };                                    // charanga
        case 7: return { "sala de câmara", 1.9, 1.1, 0.022, 0.3, 1.0 };                           // câmara
        case 8: return { "sala de concerto", 2.4, 1.4, 0.028, 0.3, 1.1 };                         // orquestra
        case 10: return { "igreja", 3.2, 1.8, 0.035, 0.25, 1.15 };                                // a cappella
        case 11: return { "estúdio (placa)", 1.3, 0.7, 0.02, 0.15, 0.8 };                         // rock
        case 12: return { "rua (o desfile)", 1.4, 1.4, 0.025, 0.45, 0.8 };                        // escola de samba
        default: return { "sala do Trioio", 1.6, 1.0, 0.018, 0.35, 1.0 };
    }
}

int mixGroupOf(int voice)
{
    switch (voice)
    {
        case 0: case 2: return 0;
        case 1: return 1;
        case 4: return 3;
        default: return 2;  // 3, 5, 6
    }
}

StageSpot stageFor(int e, int v)
{
    // as vozes: 0 motivo, 1 baixo, 2 contracanto, 3 memória, 4 percussão,
    // 5 mosaico, 6 harmonia; 7 os efeitos. A ordem de Ensemble.
    struct Row { StageSpot s[8]; };
    static const Row automatic { { { 0.0f, 1.0f, 0.2f }, { 0.0f, 0.2f, 0.3f }, { 0.0f, 1.0f, 0.35f }, { 0.0f, 1.0f, 0.6f },
                                   { 0.0f, 1.0f, 0.45f }, { 0.0f, 1.0f, 0.65f }, { 0.0f, 1.0f, 0.5f }, { 0.0f, 1.3f, 0.6f } } };
    // a orquestra na disposição clássica: os primeiros violinos à esquerda,
    // os baixos à direita, as madeiras no meio e atrás, os metais mais atrás,
    // a percussão no fundo
    static const Row orchestra { { { -0.45f, 0.6f, 0.4f }, { 0.5f, 0.5f, 0.5f }, { -0.1f, 0.3f, 0.7f }, { 0.15f, 0.3f, 0.7f },
                                   { 0.0f, 0.8f, 1.0f }, { 0.3f, 0.5f, 0.85f }, { 0.05f, 1.6f, 0.5f }, { 0.0f, 1.4f, 0.8f } } };
    static const Row chamber { { { -0.5f, 0.3f, 0.35f }, { 0.45f, 0.3f, 0.4f }, { -0.2f, 0.3f, 0.4f }, { 0.2f, 0.3f, 0.45f },
                                 { 0.0f, 0.5f, 0.8f }, { 0.25f, 0.3f, 0.5f }, { 0.1f, 1.0f, 0.45f }, { 0.0f, 1.2f, 0.7f } } };
    // o palco do jazz: o sax no centro e na frente, o trompete ao lado, o
    // piano à direita, o baixo à esquerda, a bateria aberta no fundo
    static const Row jazz { { { 0.0f, 0.3f, 0.15f }, { -0.15f, 0.2f, 0.3f }, { -0.3f, 0.3f, 0.25f }, { 0.5f, 0.4f, 0.5f },
                              { 0.0f, 1.4f, 0.5f }, { -0.5f, 0.4f, 0.5f }, { 0.35f, 0.8f, 0.4f }, { 0.0f, 1.3f, 0.6f } } };
    // o rock: a voz e o baixo no centro, a guitarra solo um pouco à esquerda,
    // as bases abertas nos dois lados, o órgão à direita
    static const Row rock { { { -0.2f, 0.3f, 0.2f }, { 0.0f, 0.1f, 0.25f }, { 0.4f, 0.4f, 0.35f }, { 0.0f, 0.2f, 0.15f },
                              { 0.0f, 1.3f, 0.35f }, { -0.6f, 0.4f, 0.4f }, { 0.0f, 1.9f, 0.3f }, { 0.0f, 1.3f, 0.6f } } };
    // a roda (choro, samba, forró, bolero, tango, salsa…): o solista no
    // centro, o contracanto à esquerda, o baixo à direita, a harmonia
    // aberta, a percussão um pouco atrás
    static const Row roda { { { 0.0f, 0.3f, 0.2f }, { 0.3f, 0.3f, 0.3f }, { -0.35f, 0.3f, 0.3f }, { 0.25f, 0.4f, 0.5f },
                              { 0.1f, 1.2f, 0.45f }, { -0.4f, 0.4f, 0.5f }, { -0.15f, 1.2f, 0.35f }, { 0.0f, 1.3f, 0.6f } } };
    // o coro: as vozes em arco, a igreja ao fundo
    static const Row choir { { { -0.2f, 0.6f, 0.35f }, { 0.35f, 0.6f, 0.4f }, { 0.2f, 0.6f, 0.4f }, { -0.4f, 0.8f, 0.55f },
                               { 0.0f, 1.0f, 0.6f }, { 0.0f, 1.5f, 0.7f }, { 0.0f, 1.6f, 0.55f }, { 0.0f, 1.3f, 0.7f } } };
    const Row* row = &automatic;
    switch (e)
    {
        case 8: row = &orchestra; break;
        case 7: row = &chamber; break;
        case 3: case 4: row = &jazz; break;
        case 11: row = &rock; break;
        case 10: row = &choir; break;
        case 0: row = &automatic; break;
        default: row = &roda; break;
    }
    return row->s[static_cast<std::size_t>(std::clamp(v, 0, 7))];
}

void mixBus(int group, std::vector<float>& L, std::vector<float>& R, double sr, const RoomProfile& room,
            std::vector<float>& outL, std::vector<float>& outR, std::vector<float>& sendL, std::vector<float>& sendR,
            const StageSpot& spot)
{
    auto s = specOf(group);
    // o palco: a largura do lugar, a profundidade (mais sala, menos brilho,
    // um pouco mais baixo)
    s.width *= spot.width;
    s.send *= 0.6 + 1.2 * spot.depth;
    s.airDb -= 3.0 * spot.depth;
    s.gain *= 1.0 - 0.2 * spot.depth;
    const double pa = (std::clamp(static_cast<double>(spot.pan), -1.0, 1.0) + 1.0) * kPi / 4.0;
    const double gl = std::cos(pa) * std::sqrt(2.0), gr = std::sin(pa) * std::sqrt(2.0);
    std::array<Biquad, 2> hp, mud, pres, air;
    for (int c = 0; c < 2; ++c)
    {
        if (s.hp > 0.0)
            hp[c].highpass(s.hp, sr);
        mud[c].peak(300.0, sr, 1.0, s.mudDb);
        pres[c].peak(3000.0, sr, 0.9, s.presenceDb);
        air[c].highShelf(9000.0, sr, s.airDb);
    }
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        double l = L[i], r = R[i];
        if (s.hp > 0.0)
        {
            l = hp[0].run(l);
            r = hp[1].run(r);
        }
        l = air[0].run(pres[0].run(mud[0].run(l)));
        r = air[1].run(pres[1].run(mud[1].run(r)));
        L[i] = static_cast<float>(l);
        R[i] = static_cast<float>(r);
    }
    compress(L, R, sr, s.ratio, s.attack, s.release);
    // a largura (meio e lados) e a soma
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        const double m = 0.5 * (L[i] + R[i]), d = 0.5 * (L[i] - R[i]) * s.width;
        const double l = (m + d) * s.gain * gl, r = (m - d) * s.gain * gr;
        outL[i] += static_cast<float>(l);
        outR[i] += static_cast<float>(r);
        sendL[i] += static_cast<float>(l * s.send * room.wet);
        sendR[i] += static_cast<float>(r * s.send * room.wet);
    }
}

void reverb(const std::vector<float>& sendL, const std::vector<float>& sendR, double sr, const RoomProfile& room,
            std::vector<float>& L, std::vector<float>& R)
{
    const std::size_t N = std::min({ sendL.size(), L.size(), R.size() });
    if (N == 0)
        return;
    // 1. o pré-atraso e as reflexões iniciais (taps estéreo diferentes)
    const auto pre = static_cast<std::size_t>(room.predelay * sr);
    static const double tapsL[8] = { 0.0043, 0.0079, 0.0121, 0.0172, 0.0219, 0.0287, 0.0341, 0.0413 };
    static const double tapsR[8] = { 0.0051, 0.0088, 0.0114, 0.0163, 0.0237, 0.0269, 0.0359, 0.0397 };
    // 2. a difusão de entrada: quatro passa-tudos em série por canal
    struct AP
    {
        std::vector<float> b;
        std::size_t w = 0;
        double g;
        double run(double x)
        {
            const double d = b[w];
            const double y = -g * x + d;
            b[w] = static_cast<float>(x + g * y);
            w = (w + 1) % b.size();
            return y;
        }
    };
    const auto ap = [&](double ms, double g) { return AP { std::vector<float>(std::max<std::size_t>(1, static_cast<std::size_t>(ms * 1e-3 * sr)), 0.0f), 0, g }; };
    std::array<AP, 4> difL { ap(4.77, 0.75), ap(3.59, 0.75), ap(12.73, 0.625), ap(9.31, 0.625) };
    std::array<AP, 4> difR { ap(4.53, 0.75), ap(3.71, 0.75), ap(12.21, 0.625), ap(9.83, 0.625) };
    // 3. a rede: 8 linhas, mistura de Hadamard, perda por frequência
    static const double baseMs[8] = { 29.7, 37.1, 41.1, 43.7, 53.0, 59.9, 67.7, 79.3 };
    std::array<std::vector<float>, 8> line;
    std::array<std::size_t, 8> len {}, w {};
    std::array<double, 8> g {}, lp {};
    for (int k = 0; k < 8; ++k)
    {
        len[k] = static_cast<std::size_t>(baseMs[k] * 1e-3 * sr * room.size) + 16;
        line[k].assign(len[k] + 16, 0.0f);
        // × 1,22: o filtro de agudos também tira energia a cada volta (medido:
        // sem a correção, o RT60 saía em ~82 % do pedido)
        g[k] = std::pow(10.0, -3.0 * static_cast<double>(len[k]) / (1.22 * room.rt60 * sr));
    }
    std::vector<float> preL(pre + 1, 0.0f), preR(pre + 1, 0.0f), erL(static_cast<std::size_t>(0.05 * sr), 0.0f),
        erR(static_cast<std::size_t>(0.05 * sr), 0.0f);
    std::size_t pw = 0, ew = 0;
    double modPh = 0.0;
    // o envio filtrado (como no estúdio): sem o grave que embola (passa-altas
    // 180 Hz) e sem o agudo que chia (passa-baixas 9 kHz)
    std::array<Biquad, 2> sendHp, sendLp;
    for (int c = 0; c < 2; ++c)
    {
        sendHp[c].highpass(180.0, sr);
        sendLp[c].b0 = 0.0;  // preenchido abaixo
    }
    const double lpA = 1.0 - std::exp(-2.0 * kPi * 9000.0 / sr);
    double slpL = 0.0, slpR = 0.0;
    for (std::size_t i = 0; i < N; ++i)
    {
        // o pré-atraso
        const double inL = preL[pw], inR = preR[pw];
        slpL += lpA * (sendHp[0].run(sendL[i]) - slpL);
        slpR += lpA * (sendHp[1].run(sendR[i]) - slpR);
        preL[pw] = static_cast<float>(slpL);
        preR[pw] = static_cast<float>(slpR);
        pw = (pw + 1) % preL.size();
        // as reflexões iniciais
        erL[ew] = static_cast<float>(inL);
        erR[ew] = static_cast<float>(inR);
        double eL = 0.0, eR = 0.0;
        for (int t = 0; t < 8; ++t)
        {
            const auto dl = static_cast<std::size_t>(tapsL[t] * sr * room.size);
            const auto dr = static_cast<std::size_t>(tapsR[t] * sr * room.size);
            const double gt = 0.6 / (1.0 + 0.5 * t);
            eL += gt * erL[(ew + erL.size() - std::min(dl, erL.size() - 1)) % erL.size()] * (t % 2 ? -1.0 : 1.0);
            eR += gt * erR[(ew + erR.size() - std::min(dr, erR.size() - 1)) % erR.size()] * (t % 3 ? 1.0 : -1.0);
        }
        ew = (ew + 1) % erL.size();
        // a difusão
        double dL = inL, dR = inR;
        for (auto& a : difL)
            dL = a.run(dL);
        for (auto& a : difR)
            dR = a.run(dR);
        // as saídas das linhas (duas moduladas, leitura fracionária)
        modPh += 2.0 * kPi * 0.7 / sr;
        std::array<double, 8> o {};
        for (int k = 0; k < 8; ++k)
        {
            double d = static_cast<double>(len[k]);
            if (k == 2)
                d += 6.0 * std::sin(modPh);
            if (k == 6)
                d += 5.0 * std::sin(1.3 * modPh + 1.0);
            const auto di = static_cast<std::size_t>(d);
            const double fr = d - static_cast<double>(di);
            const std::size_t sz = line[k].size();
            const double a = line[k][(w[k] + sz - di) % sz], b = line[k][(w[k] + sz - di - 1) % sz];
            const double v = a + fr * (b - a);
            lp[k] += (1.0 - room.damping) * (v - lp[k]);  // o agudo morre antes
            o[k] = g[k] * lp[k];
        }
        // Hadamard 8×8 (rápida), normalizada
        std::array<double, 8> h = o;
        for (int span = 1; span < 8; span *= 2)
            for (int s0 = 0; s0 < 8; s0 += 2 * span)
                for (int j = s0; j < s0 + span; ++j)
                {
                    const double x = h[j], y = h[j + span];
                    h[j] = x + y;
                    h[j + span] = x - y;
                }
        for (int k = 0; k < 8; ++k)
        {
            const double in = k < 4 ? dL : dR;
            line[k][w[k]] = static_cast<float>(h[k] / std::sqrt(8.0) + 0.5 * in);
            w[k] = (w[k] + 1) % line[k].size();
        }
        // as saídas: duas linhas da matriz de Hadamard (vetores ortogonais):
        // a cauda esquerda e a direita não se correlacionam
        const double tailL = (o[0] + o[1] - o[2] - o[3] + o[4] + o[5] - o[6] - o[7]) / std::sqrt(8.0) * 2.0;
        const double tailR = (o[0] - o[1] + o[2] - o[3] - o[4] + o[5] - o[6] + o[7]) / std::sqrt(8.0) * 2.0;
        L[i] += static_cast<float>(0.55 * eL + 0.45 * tailL);
        R[i] += static_cast<float>(0.55 * eR + 0.45 * tailR);
    }
}

void glue(std::vector<float>& L, std::vector<float>& R)
{
    float pk = 1e-9f;
    for (std::size_t i = 0; i < L.size(); ++i)
        pk = std::max({ pk, std::abs(L[i]), std::abs(R[i]) });
    const double k = 1.3;
    const double norm = std::tanh(k);
    for (std::size_t i = 0; i < L.size(); ++i)
    {
        const double l = L[i] / pk, r = R[i] / pk;
        L[i] = static_cast<float>(pk * (0.75 * l + 0.25 * std::tanh(k * l) / norm));
        R[i] = static_cast<float>(pk * (0.75 * r + 0.25 * std::tanh(k * r) / norm));
    }
}

} // namespace trioio

namespace trioio::composition
{

std::string humanize(Score& score, std::uint64_t seed)
{
    if (score.notes.empty())
        return {};
    std::uint64_t st = seed * 0xD1B54A32D192ED03ull + 0x77ull;
    const auto uni = [&] {
        st ^= st << 13;
        st ^= st >> 7;
        st ^= st << 17;
        return static_cast<double>(st >> 11) / 9007199254740992.0 * 2.0 - 1.0;
    };
    const auto freePitch = [](Timbre t) {
        switch (t)
        {
            case Timbre::bowed: case Timbre::stringSection: case Timbre::reed: case Timbre::sax: case Timbre::transverseFlute:
            case Timbre::trumpet: case Timbre::trombone: case Timbre::brassSection: case Timbre::singing: case Timbre::voiceChoir:
            case Timbre::hum: case Timbre::upright: case Timbre::breath: case Timbre::flute:
                return true;
            default: return false;
        }
    };
    // um passeio lento por voz (o músico adianta e atrasa por trechos)
    std::array<double, 8> drift {}, lastPitch {};
    std::array<int, 8> alternate {};
    std::vector<std::size_t> order(score.notes.size());
    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return score.notes[a].time < score.notes[b].time; });
    int touched = 0;
    for (std::size_t i : order)
    {
        auto& n = score.notes[i];
        const auto v = static_cast<std::size_t>(std::clamp(n.voice, 0, 7));
        const bool perc = n.voice == 4;
        drift[v] = 0.92 * drift[v] + 0.08 * uni();  // de baixa frequência
        const double ms = perc ? 3.0 : 9.0;
        n.time = std::max(0.0, n.time + (0.7 * drift[v] + 0.3 * uni()) * ms * 1e-3);
        // a força: alguns %, e a alternância nas notas repetidas
        float vel = n.velocity * static_cast<float>(1.0 + 0.05 * uni());
        if (std::abs(n.pitch - lastPitch[v]) < 0.01)
        {
            alternate[v] ^= 1;
            vel *= alternate[v] ? 0.93f : 1.0f;
        }
        else
            alternate[v] = 0;
        lastPitch[v] = n.pitch;
        n.velocity = std::clamp(vel, 0.03f, 1.0f);
        if (freePitch(n.timbre))
            n.pitch += static_cast<float>(0.015 * uni());  // ~1,5 cent
        ++touched;
    }
    return "humanização: " + std::to_string(touched) + " notas (tempo, força e afinação fina, em passeio lento por voz)";
}

} // namespace trioio::composition

namespace trioio::composition
{

std::string applyExpression(Score& score, bool ending)
{
    if (score.notes.empty() || score.duration < 8.0)
        return {};
    int phrases = 0;
    // 1. o arco da frase e a carga melódica, por voz melódica
    for (int v : { 0, 2 })
    {
        std::vector<std::size_t> idx;
        for (std::size_t i = 0; i < score.notes.size(); ++i)
            if (score.notes[i].voice == v)
                idx.push_back(i);
        std::sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) { return score.notes[a].time < score.notes[b].time; });
        std::size_t s = 0;
        while (s < idx.size())
        {
            // a frase: até uma pausa de mais de 0,6 s
            std::size_t e = s + 1;
            while (e < idx.size())
            {
                const auto& prev = score.notes[idx[e - 1]];
                if (score.notes[idx[e]].time - (prev.time + prev.duration) > 0.6)
                    break;
                ++e;
            }
            if (e - s >= 3)
            {
                const double t0 = score.notes[idx[s]].time;
                const auto& last = score.notes[idx[e - 1]];
                const double len = std::max(0.1, last.time + last.duration - t0);
                double mean = 0.0;
                for (std::size_t k = s; k < e; ++k)
                    mean += score.notes[idx[k]].pitch;
                mean /= static_cast<double>(e - s);
                for (std::size_t k = s; k < e; ++k)
                {
                    auto& n = score.notes[idx[k]];
                    const double u = (n.time - t0) / len;
                    // o arco: sobe até 0,6 e cai até o fim
                    const double arc = u < 0.6 ? std::sin(0.5 * 3.14159265358979 * u / 0.6)
                                               : std::cos(0.5 * 3.14159265358979 * (u - 0.6) / 0.4);
                    const double charge = 1.0 + 0.012 * (n.pitch - mean);
                    n.velocity = static_cast<float>(std::clamp(n.velocity * (0.82 + 0.3 * arc) * charge, 0.03, 1.0));
                }
                ++phrases;
            }
            s = e;
        }
    }
    if (!ending)
        return "expressão: o arco de " + std::to_string(phrases) + " frases (Todd; a carga melódica do KTH)";
    // 2. o final: o ritardando nos últimos ~6 s, em todas as vozes e gestos
    double lastStart = 0.0;
    for (const auto& n : score.notes)
        lastStart = std::max(lastStart, n.time);
    const double rStart = std::max(0.0, lastStart - 6.0), span = lastStart - rStart;
    // o tempo local alarga linearmente até +25 %: t' = rStart + ∫ (1 + 0,25 x/span) dx
    const auto warp = [&](double t) {
        if (t <= rStart || span <= 0.0)
            return t;
        const double x = std::min(t, lastStart) - rStart;
        double w = rStart + x + 0.125 * x * x / span;
        if (t > lastStart)
            w += (t - lastStart) * 1.25;
        return w;
    };
    const double oldDur = score.duration;
    for (auto& n : score.notes)
    {
        const double a = warp(n.time), b = warp(n.time + n.duration);
        n.time = a;
        n.duration = std::max(0.02, b - a);
    }
    for (auto& g : score.gestures)
    {
        const double a = warp(g.time), b = warp(g.time + g.duration);
        g.time = a;
        g.duration = std::max(0.1, b - a);
    }
    // 3. a fermata: as notas que começam no último instante seguram mais
    double newLast = 0.0;
    for (const auto& n : score.notes)
        newLast = std::max(newLast, n.time);
    double maxEnd = 0.0;
    for (auto& n : score.notes)
    {
        if (newLast - n.time < 0.15 && n.voice != 4)
            n.duration *= 1.6;
        maxEnd = std::max(maxEnd, n.time + n.duration);
    }
    score.duration = std::max(oldDur, maxEnd + 1.5);
    return "expressão: o arco de " + std::to_string(phrases) + " frases (Todd; a carga melódica do KTH), o ritardando final (+25 % nos últimos "
           + std::to_string(static_cast<int>(std::lround(span))) + " s) e a fermata";
}

} // namespace trioio::composition

