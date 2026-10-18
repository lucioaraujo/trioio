#pragma once

// Os números desnormalizados (2026-10-18): as caudas dos filtros IIR no
// silêncio decaem até ~1e-38, e a CPU x86 os processa até 100× mais devagar
// (a mistura de uma resposta caiu de 12,9 s para 1,1 s, com o mesmo áudio em
// 24 bits). ScopedNoDenormals liga FTZ e DAZ na thread atual e restaura ao
// sair. O MXCSR é por thread: cada thread de trabalho cria o seu. Sem SSE
// (ARM), não faz nada (o áudio em ARM já trata os desnormalizados).

#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#include <xmmintrin.h>
#define TRIOIO_HAS_SSE 1
#endif

namespace trioio
{

struct ScopedNoDenormals
{
#ifdef TRIOIO_HAS_SSE
    unsigned int saved = _mm_getcsr();
    ScopedNoDenormals() { _mm_setcsr(saved | 0x8040u); }  // FTZ (bit 15) e DAZ (bit 6)
    ~ScopedNoDenormals() { _mm_setcsr(saved); }
    ScopedNoDenormals(const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;
#endif
};

} // namespace trioio
