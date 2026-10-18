#pragma once

// As palavras do autor sem depender do acento nem da caixa: "sinfonica" e
// "Sinfônica", "baiao" e "baião", "Grecia" e "Grécia" se encontram
// (2026-10-18: o autor escreveu "sinfonica" e "maculele" e o Trioio não leu).
// Minúsculas e os acentos latinos mais comuns (UTF-8) dobrados para ASCII;
// o resto fica como veio. O autor pediu (2026-10-18) que o Trioio reconheça
// "falta de acentos, acentos indevidos, separação de palavras, erros
// gramaticais", "caixa alta, caixa baixa, números e caracteres especiais":
//   foldText    — caixa e acento (o acento faltando e o indevido somem dos
//                 dois lados da comparação);
//   splitWords  — as palavras: pontuação, aspas, travessões, #, @, emoji e
//                 outros símbolos separam; o hífen dentro da palavra fica;
//   fuzzyEqual  — o erro de digitação ou de grafia: até 1 letra de diferença
//                 (trocada, faltando, sobrando ou invertida) em palavras de 5+
//                 letras, até 2 em 8+; o plural e o gênero no fim também.

#include <algorithm>
#include <string>
#include <vector>

namespace trioio::composition
{

[[nodiscard]] inline std::string foldText(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c >= 'A' && c <= 'Z')
        {
            out += static_cast<char>(c - 'A' + 'a');
            continue;
        }
        if ((c == 0xC3 || c == 0xC4 || c == 0xC5) && i + 1 < s.size())
        {
            const unsigned cp = ((c & 0x1Fu) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3Fu);
            char a = 0;
            if ((cp >= 0xC0 && cp <= 0xC5) || (cp >= 0xE0 && cp <= 0xE5) || (cp >= 0x100 && cp <= 0x105)) a = 'a';
            else if (cp == 0xC7 || cp == 0xE7 || (cp >= 0x106 && cp <= 0x10D)) a = 'c';
            else if ((cp >= 0xC8 && cp <= 0xCB) || (cp >= 0xE8 && cp <= 0xEB) || (cp >= 0x112 && cp <= 0x11B)) a = 'e';
            else if ((cp >= 0xCC && cp <= 0xCF) || (cp >= 0xEC && cp <= 0xEF) || (cp >= 0x128 && cp <= 0x131)) a = 'i';
            else if (cp == 0xD1 || cp == 0xF1 || (cp >= 0x143 && cp <= 0x148)) a = 'n';
            else if ((cp >= 0xD2 && cp <= 0xD6) || cp == 0xD8 || (cp >= 0xF2 && cp <= 0xF6) || cp == 0xF8 || (cp >= 0x14C && cp <= 0x151)) a = 'o';
            else if ((cp >= 0xD9 && cp <= 0xDC) || (cp >= 0xF9 && cp <= 0xFC) || (cp >= 0x168 && cp <= 0x173)) a = 'u';
            else if (cp == 0xDD || cp == 0xFD || cp == 0xFF) a = 'y';
            else if (cp >= 0x15A && cp <= 0x161) a = 's';
            if (a)
            {
                out += a;
                ++i;
                continue;
            }
        }
        out += static_cast<char>(c);
    }
    return out;
}

[[nodiscard]] inline std::vector<std::string> splitWords(const std::string& text)
{
    std::vector<std::string> out;
    const auto f = foldText(text);
    std::string w;
    const auto flush = [&] {
        while (!w.empty() && w.back() == '-')
            w.pop_back();
        while (!w.empty() && w.front() == '-')
            w.erase(w.begin());
        if (!w.empty())
            out.push_back(w);
        w.clear();
    };
    for (std::size_t i = 0; i < f.size(); ++i)
    {
        const auto c = static_cast<unsigned char>(f[i]);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            w += static_cast<char>(c);
        else if ((c == '~' || c == '^' || c == '`') && !w.empty() && i + 1 < f.size()
                 && static_cast<unsigned char>(f[i + 1]) >= 'a' && static_cast<unsigned char>(f[i + 1]) <= 'z')
            continue;  // a tecla morta solta no meio da palavra ("bai~ao" = "baião")
        else if (c == 0xC2 && i + 2 < f.size() && !w.empty()
                 && (static_cast<unsigned char>(f[i + 1]) == 0xB4 || static_cast<unsigned char>(f[i + 1]) == 0xA8)
                 && static_cast<unsigned char>(f[i + 2]) >= 'a' && static_cast<unsigned char>(f[i + 2]) <= 'z')
        {
            ++i;  // ´ e ¨ soltos, idem
            continue;
        }
        else if (c == '-')
        {
            if (!w.empty() && w.back() != '-')
                w += '-';
        }
        else if (c >= 0xF0)  // emoji e outros símbolos de 4 bytes
        {
            flush();
            i += 3;
        }
        else if (c == 0xE2 && i + 1 < f.size() && (static_cast<unsigned char>(f[i + 1]) == 0x80 || static_cast<unsigned char>(f[i + 1]) == 0x81
                                                 || (static_cast<unsigned char>(f[i + 1]) >= 0x86 && static_cast<unsigned char>(f[i + 1]) <= 0x9F)))
        {
            flush();  // pontuação tipográfica (“ ” – — … •), setas, símbolos
            i += 2;
        }
        else if (c == 0xC2)  // ¡ ¿ « » º ª ° · e o espaço inseparável
        {
            flush();
            i += 1;
        }
        else if (c >= 0x80)
            w += static_cast<char>(c);  // as outras letras (grego, cirílico, CJK…) ficam como vieram
        else
            flush();
    }
    flush();
    return out;
}

// a distância de edição com a transposição (Damerau, alinhamento ótimo)
[[nodiscard]] inline int editDistance(const std::string& a, const std::string& b)
{
    const std::size_t n = a.size(), m = b.size();
    std::vector<std::vector<int>> d(n + 1, std::vector<int>(m + 1));
    for (std::size_t i = 0; i <= n; ++i)
        d[i][0] = static_cast<int>(i);
    for (std::size_t j = 0; j <= m; ++j)
        d[0][j] = static_cast<int>(j);
    for (std::size_t i = 1; i <= n; ++i)
        for (std::size_t j = 1; j <= m; ++j)
        {
            const int cost = a[i - 1] == b[j - 1] ? 0 : 1;
            d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost });
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
                d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
        }
    return d[n][m];
}

// duas letras vizinhas invertidas ("rokc" = "rock"): o único erro aceito
// nas palavras curtas (4 letras), onde uma letra trocada já faz outra palavra
[[nodiscard]] inline bool adjacentSwap(const std::string& a, const std::string& b)
{
    if (a.size() != b.size() || a.size() < 4)
        return false;
    std::size_t i = 0;
    while (i < a.size() && a[i] == b[i])
        ++i;
    return i + 1 < a.size() && a[i] == b[i + 1] && a[i + 1] == b[i] && a.compare(i + 2, std::string::npos, b, i + 2, std::string::npos) == 0;
}

[[nodiscard]] inline int fuzzyLimit(std::size_t len) { return len >= 8 ? 2 : len >= 5 ? 1 : 0; }

// a palavra (já dobrada) é a outra com um erro pequeno, ou com a flexão no fim
[[nodiscard]] inline bool fuzzyEqual(const std::string& word, const std::string& target)
{
    if (word == target)
        return true;
    const auto strip = [](std::string x) {
        for (const char* suf : { "es", "s", "a", "o" })
        {
            const std::string sf = suf;
            if (x.size() > sf.size() + 4 && x.compare(x.size() - sf.size(), sf.size(), sf) == 0)
                return x.substr(0, x.size() - sf.size());
        }
        return x;
    };
    if (strip(word) == strip(target) || adjacentSwap(word, target))
        return true;
    const int lim = fuzzyLimit(std::min(word.size(), target.size()));
    if (lim == 0 || (word.size() > target.size() ? word.size() - target.size() : target.size() - word.size()) > static_cast<std::size_t>(lim))
        return false;
    return editDistance(word, target) <= lim;
}

// o radical do léxico (um prefixo) com um erro pequeno: o começo da palavra
// com o tamanho do radical (±1) está perto dele; radicais curtos, só exatos
[[nodiscard]] inline int fuzzyPrefixDistance(const std::string& word, const std::string& stem)
{
    // radicais curtos ("escur") confundiriam palavras vizinhas ("escuta"): só 6+
    const int lim = stem.size() >= 9 ? 2 : stem.size() >= 6 ? 1 : 0;
    if (stem.size() >= 4 && word.size() >= stem.size() && adjacentSwap(word.substr(0, stem.size()), stem))
        return 1;
    if (lim == 0 || word.size() < 5)
        return 99;
    int best = 99;
    for (int delta = -lim; delta <= lim; ++delta)
    {
        const auto len = static_cast<long>(stem.size()) + delta;
        if (len < 4 || len > static_cast<long>(word.size()))
            continue;
        best = std::min(best, editDistance(word.substr(0, static_cast<std::size_t>(len)), stem));
    }
    return best <= lim ? best : 99;
}

} // namespace trioio::composition
