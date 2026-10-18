#!/usr/bin/env python3
"""Gera docs/REFERENCIAS_POR_MODULO.md: as referências de cada módulo do Trioio.

Lê os comentários dos fontes em src/ (core, library, app, tools) e registra,
por arquivo:
  - a função (o primeiro parágrafo do comentário de abertura);
  - as referências EXTERNAS citadas (linhas de comentário com autor e ano):
    inspiração conceitual, teoria, artigos, gravações — nunca código copiado;
  - as referências INTERNAS (os instrumentos RASGO irmãos citados);
  - a origem do código: próprio, ou "adaptado de" (com a proveniência no
    próprio arquivo ou em docs/TRIOIO_INVENTARIO_RASGO.md).
A licença de todo o código é a do Trioio (AGPL-3.0-or-later); terceiros
incorporados: nenhum (CREDITS_AND_SOURCES.md).

É uma extração heurística: o que não estiver escrito no comentário do
fonte não aparece. Rode de novo depois de mudar os fontes:

    python3 tools/gerar_referencias_por_modulo.py docs/REFERENCIAS_POR_MODULO.md
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SIBLINGS = ["Rasgo Synth", "Rasgo Modular", "Antitotem", "Aquorbium", "AQUORBIUM", "Navalha", "Maravi", "Studio",
            "Trucmetry", "Risco", "Rasgo Performance"]
YEAR = re.compile(r"\b(1[5-9]\d\d|20[0-2]\d)\b")
CAPWORD = re.compile(r"\b[A-ZÁÉÍÓÚÂÊÔÃÕÇ][a-záéíóúâêôãõçü]+")


def comments(text):
    """Os blocos de comentário juntados e partidos em frases (; ou . seguido de espaço)."""
    blocks, cur = [], []
    for line in text.splitlines():
        s = line.strip()
        c = None
        if "//" in s:
            c = s[s.index("//") + 2:].strip()
        elif s.startswith("*") or s.startswith("/*"):
            c = s.lstrip("/*").strip()
        if c:
            cur.append(c)
        elif cur:
            blocks.append(" ".join(cur))
            cur = []
    if cur:
        blocks.append(" ".join(cur))
    out = []
    for b in blocks:
        for part in re.split(r"(?<=[;:])\s+|(?<=\.)\s+(?=[A-ZÁÉÍÓÚ(])", b):
            part = part.strip()
            if part:
                out.append(part)
    return out


def opening(text):
    para = []
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#pragma") or (not s and not para):
            continue
        if s.startswith("//"):
            c = s[2:].strip()
            if not c and para:
                break
            if c:
                para.append(c)
        elif para or s.startswith("#include"):
            break
    return " ".join(para)


def main():
    out = pathlib.Path(sys.argv[1])
    rows = []
    for d in ["core", "library", "app", "tools"]:
        files = sorted((ROOT / "src" / d).glob("*.h")) + sorted((ROOT / "src" / d).glob("*.cpp"))
        seen = set()
        for f in files:
            stem = f.stem
            if stem in seen:
                continue
            seen.add(stem)
            pair = [p for p in (f.with_suffix(".h"), f.with_suffix(".cpp")) if p.exists()]
            text = "\n".join(p.read_text(encoding="utf-8", errors="replace") for p in pair)
            cs = comments(text)
            purpose = opening(pair[0].read_text(encoding="utf-8", errors="replace"))
            ext = []
            for c in cs:
                if YEAR.search(c) and CAPWORD.search(c) and 12 < len(c) and "2026-" not in c and "(2026" not in c:
                    c = c.strip(" -;:")
                    if len(c) > 220:
                        c = c[:217] + "…"
                    if c not in ext:
                        ext.append(c)
            internal = sorted({s for s in SIBLINGS if s in text})
            origin = "adaptado (ver o comentário do fonte)" if re.search(r"adaptad[oa] d[oa]|copiad[oa] de .*commit", text) else "próprio"
            rows.append((f"src/{d}/{stem}", purpose, ext[:8], len(ext), internal, origin))
    with open(out, "w", encoding="utf-8", newline="\n") as o:
        o.write("# Referências por módulo\n\n"
                "Gerado por `tools/gerar_referencias_por_modulo.py` a partir dos comentários dos fontes. "
                "Separa a inspiração conceitual (as fontes externas citadas: teoria, artigos, gravações), "
                "as referências internas (os instrumentos RASGO irmãos) e a origem do código. "
                "Licença de todo o código: AGPL-3.0-or-later; código de terceiros incorporado: nenhum "
                "([`CREDITS_AND_SOURCES.md`](../CREDITS_AND_SOURCES.md)). Extração heurística: o que não "
                "está no comentário do fonte não aparece aqui.\n\n")
        o.write(f"{len(rows)} módulos.\n\n")
        for name, purpose, ext, n, internal, origin in rows:
            o.write(f"## `{name}`\n\n")
            if purpose:
                o.write(purpose[:600] + ("…" if len(purpose) > 600 else "") + "\n\n")
            o.write(f"- **código:** {origin}\n")
            o.write(f"- **referências internas:** {', '.join(internal) if internal else 'nenhuma'}\n")
            if ext:
                o.write(f"- **fontes externas citadas** ({n}{', as 8 primeiras' if n > 8 else ''}):\n")
                for e in ext:
                    o.write(f"  - {e}\n")
            else:
                o.write("- **fontes externas citadas:** nenhuma\n")
            o.write("\n")
    print(f"{len(rows)} módulos em {out}")


if __name__ == "__main__":
    main()
