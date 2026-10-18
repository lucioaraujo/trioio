#!/usr/bin/env bash
# O pacote de fontes de uma versão do Trioio, com manifesto e teste de
# restauração (TRIOIO_TAREFAS, "Continuidade de acervo"). Uso:
#   tools/empacotar_release.sh <pasta-de-saida> [--sem-teste]
#
# 1. o RECORTE: os arquivos versionados no git do Trioio (git ls-files),
#    sem o site (publicação própria). Nada de build/, áudio, dados locais ou
#    0_BRAINSTORM: o que não está no git não entra;
# 2. o MANIFESTO: o SHA-256 de cada arquivo (MANIFESTO.sha256, dentro do
#    pacote) e o do próprio pacote (ao lado);
# 3. a RESTAURAÇÃO: extrai numa pasta nova, confere os checksums, configura
#    só o core (sem JUCE), compila e roda os testes do core. Se algo falhar, o
#    pacote não é dado como bom.
# A pasta de saída deve ficar no disco de 4 TB (o / tem pouco espaço).
set -euo pipefail
out="$1"; shift || true
teste=1; [ "${1:-}" = "--sem-teste" ] && teste=0
here="$(cd "$(dirname "$0")/.." && pwd)"
ver=$(sed -n 's/^project(Trioio VERSION \([0-9.]*\).*/\1/p' "$here/CMakeLists.txt")
nome="trioio-${ver}-fontes"
mkdir -p "$out"
stage="$out/$nome"
rm -rf "$stage"; mkdir -p "$stage"
cd "$here"
git ls-files -- . ':!website' > "$out/$nome.lista"
while IFS= read -r f; do mkdir -p "$stage/$(dirname "$f")"; cp -p "$f" "$stage/$f"; done < "$out/$nome.lista"
( cd "$stage" && find . -type f ! -name MANIFESTO.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFESTO.sha256 )
tar -C "$out" -czf "$out/$nome.tar.gz" "$nome"
( cd "$out" && sha256sum "$nome.tar.gz" > "$nome.tar.gz.sha256" )
n=$(wc -l < "$out/$nome.lista")
echo "pacote: $out/$nome.tar.gz ($n arquivos, $(du -h "$out/$nome.tar.gz" | cut -f1))"
rm -rf "$stage" "$out/$nome.lista"
[ "$teste" = 0 ] && exit 0
# a restauração
rest="$out/restauracao-$nome"
rm -rf "$rest"; mkdir -p "$rest"
( cd "$out" && sha256sum -c "$nome.tar.gz.sha256" )
tar -C "$rest" -xzf "$out/$nome.tar.gz"
( cd "$rest/$nome" && sha256sum --quiet -c MANIFESTO.sha256 && echo "checksums: ok" )
nice cmake -S "$rest/$nome" -B "$rest/build" -DCMAKE_BUILD_TYPE=Release > "$rest/cmake.log"
nice cmake --build "$rest/build" -j2 > "$rest/build.log" 2>&1 || { echo "falhou a compilação: $rest/build.log"; exit 1; }
( cd "$rest/build" && nice ctest -j2 2>&1 | tail -3 )
echo "restauração: ok (pasta $rest; apague depois de conferir)"
