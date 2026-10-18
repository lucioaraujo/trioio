#!/usr/bin/env bash
# Mede o tempo, a CPU e a memória de uma resposta por perfil de uso
# (TRIOIO_TAREFAS, Marco 4). Uso:
#   tools/medir_perfis.sh <dados> <ref> [saida.tsv]
# Um perfil de cada vez, com nice; as sessões geradas vão para a lixeira no fim.
set -u
dados="$1"; ref="$2"; out="${3:-/dev/stdout}"
B="$(dirname "$0")/../build/trioio_biblioteca_artefacts/Release/trioio_biblioteca"
printf "perfil\ttempo_s\tcpu_pct\tmemoria_max_mb\n" > "$out"
run() {
  local name="$1"; shift
  local avail; avail=$(df --output=avail -BG / | tail -1 | tr -dc 0-9)
  if [ "$avail" -lt 5 ]; then echo "menos de 5 GB livres em /: parei" >&2; return 1; fi
  local tf; tf=$(mktemp)
  /usr/bin/time -o "$tf" -f "%e\t%P\t%M" nice "$B" responder "$dados" "$ref" --seed 5 "$@" > /dev/null 2>&1
  local t; t=$(tail -1 "$tf"); rm -f "$tf"
  local e p m; IFS=$'\t' read -r e p m <<< "$t"
  printf "%s\t%s\t%s\t%d\n" "$name" "$e" "${p%\%}" "$((m / 1024))" >> "$out"
}
run "energia (sem partitura)" --modo seguir --motor energia
run "partitura (padrão)"      --modo seguir
run "conjunto jazz, rasgo 0"  --modo seguir --conjunto jazz --rasgo 0
run "jogo + critérios no som" --modo interromper --jogo sim
run "stems"                   --modo seguir --stems sim
run "linhas (3)"              --modo seguir --linhas 200,2000
"$B" limpar "$dados" --manter 0 --executar sim > /dev/null
