// As regras do autor (Atlas §55; docs/REGRAS.md): o texto é lido com erros
// por linha; proibido nunca acontece, obrigatório sempre acontece, salto e
// âmbito valem, memória mínima é cumprida, e a exceção quebra marcada.

#include "Composer.h"
#include "ThirdEye.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace trioio;
using namespace trioio::composition;
using namespace trioio::perception;
using Mode = trioio::strategy::Mode;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

constexpr double SR = 48000.0;
constexpr double PI = 3.14159265358979323846;

static bool tem(const Score& s, const std::string& termo)
{
    return std::any_of(s.marks.begin(), s.marks.end(), [&](const Mark& m) { return m.text.find(termo) != std::string::npos; });
}

int main()
{
    // 1. leitura: acentos opcionais, nomes por início único, erros com a linha
    {
        std::vector<std::string> erros;
        const auto c = parseConstraints(
            "# regras de teste\n"
            "proibido timbre chip            # só o início do nome\n"
            "proibido olho silencio subito   # sem acento: não casa (o nome tem acento)\n"
            "proibido olho silêncio súbito\n"
            "obrigatorio olho congelar\n"
            "proibido sala SPACE\n"
            "maximo salto 5\n"
            "âmbito 50 80\n"
            "mínimo memória 3\n"
            "excecao 0.25\n"
            "proibido timbre violino\n"
            "qualquer coisa\n",
            erros);
        for (const auto& e : erros)
            std::printf("  erro esperado: %s\n", e.c_str());
        CHECK(erros.size() == 3);  // "silencio subito", "violino", "qualquer coisa"
        CHECK(c.noTimbre[static_cast<std::size_t>(Timbre::chip)]);
        CHECK(c.noRoom[static_cast<std::size_t>(Room::space)]);
        std::vector<std::string> e2;
        CHECK(parseConstraints("proibido sala trioio\n", e2).noRoom[static_cast<std::size_t>(Room::trioio)] && e2.empty());
        CHECK(c.maxLeap == 5 && c.lowest == 50.0f && c.highest == 80.0f && c.minMemory == 3);
        CHECK(std::abs(c.exception - 0.25f) < 1e-6f);
        CHECK(c.lines.size() == 8);  // 11 linhas com conteúdo, 3 com erro
        int silencio = -1, congelar = -1;
        for (int k = 0; k < kEyeChoices; ++k)
        {
            if (std::string(eyeChoiceName(k)) == "silêncio súbito") silencio = k;
            if (std::string(eyeChoiceName(k)) == "congelar") congelar = k;
        }
        CHECK(silencio >= 0 && c.noEye[static_cast<std::size_t>(silencio)]);
        CHECK(congelar >= 0 && c.mustEye[static_cast<std::size_t>(congelar)]);
    }

    // referência melódica de 40 s
    const int mel[] = { 60, 62, 64, 67, 64, 62, 60, 62, 64, 69, 67, 64, 72, 71, 67, 60 };
    std::vector<float> v(static_cast<std::size_t>(SR * 40.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
        const int k = static_cast<int>(t / 0.5) % 16;
        const double f = 440.0 * std::pow(2.0, (mel[k] - 69) / 12.0);
        const double nz = (static_cast<int>(t / 10.0) % 2) ? 0.04 * std::sin(2 * PI * 3137.0 * t * (1.0 + tt)) : 0.0;
        v[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * (std::sin(2 * PI * f * t) + 0.3 * std::sin(4 * PI * f * t)) + nz);
    }
    Analyzer an(SR);
    an.push(v.data(), v.size());
    an.finish();

    // 2. as regras valem em 24 seeds
    {
        std::vector<std::string> erros;
        const auto regras = parseConstraints("proibido timbre espectral\nproibido timbre aditivo\n"
                                             "proibido olho silêncio súbito\nproibido olho massa de glissandos\n"
                                             "obrigatório olho congelar\nproibido sala SPACE\n"
                                             "máximo salto 5\nâmbito 50 80\nmínimo memória 3\n",
                                             erros);
        CHECK(erros.empty());
        int pecas = 0, timbreProibido = 0, olhoProibido = 0, comCongelar = 0, salaProibida = 0, saltos = 0,
            foraDoAmbito = 0, comMemoria = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            Options o { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 0.8f };
            o.constraints = regras;
            const auto sc = compose(an.frames(), an.onsets(), 40.0, o);
            ++pecas;
            float prev = -1.0f;
            for (const auto& n : sc.notes)
            {
                timbreProibido += n.timbre == Timbre::spectral || n.timbre == Timbre::additive;
                if (n.voice != 1 && n.voice != 4)  // baixo e percussão ficam fora do âmbito
                    foraDoAmbito += n.pitch < 50.0f - 1e-3f || n.pitch > 80.0f + 1e-3f;
                if (n.voice == 0)
                {
                    if (prev >= 0.0f && std::abs(n.pitch - prev) > 5.0f + 1e-3f)
                        ++saltos;
                    prev = n.pitch;
                }
            }
            for (const auto& m : sc.marks)
                if (m.text.rfind("Terceiro Olho", 0) == 0)  // não a marca que lista as regras
                    olhoProibido += m.text.find("silêncio súbito") != std::string::npos
                                    || m.text.find("massa de glissandos") != std::string::npos;
            comCongelar += tem(sc, "regra oculta: congelar");
            salaProibida += sc.room == Room::space;
            int mem = 0;
            for (const auto& m : sc.marks)
                mem += m.text.find("memória (oráculo)") != std::string::npos;
            comMemoria += mem >= 3;
            CHECK(tem(sc, "regras do autor:"));
        }
        std::printf("24 peças com regras: timbre proibido %d · olho proibido %d · congelar em %d · SPACE %d · "
                    "saltos > 5: %d · fora do âmbito: %d · com ≥ 3 memórias: %d\n",
                    timbreProibido, olhoProibido, comCongelar, salaProibida, saltos, foraDoAmbito, comMemoria);
        CHECK(timbreProibido == 0 && olhoProibido == 0 && salaProibida == 0);
        CHECK(comCongelar == pecas);
        CHECK(saltos == 0 && foraDoAmbito == 0);
        CHECK(comMemoria == pecas);
    }

    // 3. exceção: com chance 1, o olho pode quebrar a proibição, e marca
    {
        std::vector<std::string> erros;
        const auto r = parseConstraints("proibido olho cluster\nproibido olho rarefazer\nexceção 1\n", erros);
        int quebras = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            Options o { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 1.0f };
            o.constraints = r;
            const auto sc = compose(an.frames(), an.onsets(), 40.0, o);
            for (const auto& m : sc.marks)
                quebras += m.text.find("exceção à regra") != std::string::npos;
        }
        std::printf("exceções marcadas (chance 1): %d\n", quebras);
        CHECK(quebras >= 1);
    }

    // 4. condicionado e incompatível
    {
        std::vector<std::string> erros;
        const auto c = parseConstraints("condicionado olho cluster se energia alta\n"
                                        "incompatível olho drone e nuvem granular\n"
                                        "condicionado olho cluster se calor\n"
                                        "incompativel olho drone e drone\n",
                                        erros);
        CHECK(erros.size() == 2);
        int cluster = -1;
        for (int k = 0; k < kEyeChoices; ++k)
            if (std::string(eyeChoiceName(k)) == "cluster") cluster = k;
        CHECK(cluster >= 0 && c.when[static_cast<std::size_t>(cluster)] == Condition::energyHigh);
        CHECK(c.incompatible.size() == 1);

        // referência com trechos fortes e fracos alternados a cada 10 s
        std::vector<float> w(static_cast<std::size_t>(SR * 60.0));
        for (std::size_t i = 0; i < w.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.25);
            const double amp = (static_cast<int>(t / 10.0) % 2) ? 0.006 : 0.5;
            w[i] = static_cast<float>(amp * std::exp(-tt * 6.0) * std::sin(2 * PI * (220.0 + 40.0 * std::floor(t / 2.0)) * t));
        }
        Analyzer a2(SR);
        a2.push(w.data(), w.size());
        a2.finish();
        std::vector<std::string> e2;
        const auto regras = parseConstraints("condicionado olho cluster se energia alta\n"
                                             "incompatível olho drone e nuvem granular\n", e2);
        int clusters = 0, clustersFracos = 0, drones = 0, nuvens = 0, sobrepostos = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            Options o { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 1.0f };
            o.constraints = regras;
            const auto sc = compose(a2.frames(), a2.onsets(), 60.0, o);
            struct Span { double a, b; };
            std::vector<Span> d, n;
            for (const auto& m : sc.marks)
            {
                const auto dur = [&]() {
                    const auto p = m.text.find(" por ");
                    return p == std::string::npos ? 0.0 : std::stod(m.text.substr(p + 5));
                };
                if (m.text.rfind("Terceiro Olho · objeto sonoro: cluster", 0) == 0)
                {
                    ++clusters;
                    clustersFracos += summarize(a2.frames(), a2.onsets(), m.time, m.time + 4.0).energy < 0.6f;
                }
                if (m.text.rfind("Terceiro Olho · objeto sonoro: drone", 0) == 0) d.push_back({ m.time, m.time + dur() });
                if (m.text.rfind("Terceiro Olho · objeto sonoro: nuvem granular", 0) == 0) n.push_back({ m.time, m.time + dur() });
            }
            drones += static_cast<int>(d.size());
            nuvens += static_cast<int>(n.size());
            // o olho não começa uma enquanto a outra soa (ver ThirdEye.cpp, "incompatível")
            for (const auto& x : d)
                for (const auto& y : n)
                    sobrepostos += (y.a >= x.a && y.a < x.b + 0.5) || (x.a >= y.a && x.a < y.b + 0.5);
        }
        std::printf("condicionado: %d clusters, %d em trecho fraco · incompatível: %d drones, %d nuvens, %d sobrepostos\n",
                    clusters, clustersFracos, drones, nuvens, sobrepostos);
        CHECK(clusters >= 3 && clustersFracos == 0);
        CHECK(drones >= 1 && nuvens >= 1 && sobrepostos == 0);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("regras do autor: todas as checagens passaram");
    return EXIT_SUCCESS;
}
