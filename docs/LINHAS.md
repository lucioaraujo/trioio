# Linhas por região de frequência

**Estado:** etapa 1 (dividir e ler) e etapa 2 (criar) implementadas e
medidas em 2026-10-13. Falta a etapa 3 (interface), e falta a escuta do
autor.

## Pedido do autor (2026-10-13)

> "tem como criar multiplas linhas de referencia de som baseadas em áreas de
> frequencias — e criar multi análises de criação baseadas nessas linhas
> (autonomas)"

Proposta aceita ("pode ser como sugeriu"):

- 3 bandas fixas e editáveis;
- pulso comum como padrão, com linhas livres como opção;
- uma peça única com um stem por linha.

Trabalho por etapas, medindo antes de compor:

1. **Dividir e ler:** o divisor e a leitura por linha na linha de comando
   (feita).
2. **Criar:** um compositor por linha (modo, semente, linguagem e Terceiro
   Olho próprios), um regente que junta as linhas (pulso comum ou linhas
   livres) e os stems por linha.
3. **Interface:** as linhas como faixas empilhadas e o julgamento por linha
   ("o grave sim, o agudo não").

## Etapa 1 — o divisor (`src/core/BandSplit.*`)

- **Filtro:** cada corte é um Butterworth de 2.ª ordem aplicado em ida e
  volta (fase zero). Isso dá a magnitude de um Linkwitz-Riley de 4.ª ordem:
  −6 dB no corte e 24 dB por oitava (S. Linkwitz, "Active Crossover Networks
  for Noncoincident Drivers", JAES 24(1), 1976). A ida e volta é a técnica
  clássica de filtragem de fase zero, possível porque o trabalho é offline.
- **Subtração:** cada banda sai do resto, `banda_k = PB_k(resto_k)` e
  `resto_{k+1} = resto_k − banda_k`. Por isso a soma das linhas é a
  referência, amostra a amostra; o erro medido é menor que 1e-5.
- **Cortes:** de 1 a 4 (2 a 5 linhas). São descartados os que ficam abaixo de
  20 Hz, acima de 0,45 × a taxa de amostragem, ou a menos de 1,2× do anterior.
- **Nível:** cada linha é levada ao RMS da referência antes da análise (até
  +30 dB), para os limiares de silêncio valerem igual em todas. A parte da
  energia é medida antes desse ajuste.
- **A soma das partes de energia fica abaixo de 1.** Perto de cada corte, as
  linhas vizinhas levam o mesmo sinal *em fase*: no corte, cada uma leva meia
  amplitude. A amplitude soma 1, mas a energia de cada parte fica menor.
  Medido: 0,96 com tons a 1,3 oitava dos cortes; 0,73 na Recording_187, que
  tem muito violão e voz perto de 200 Hz e de 2 kHz. A parte de energia é,
  portanto, uma indicação de peso, não uma partição.
- **Não é separação de fontes.** Uma banda corta a voz e o violão onde eles se
  cruzam. A linha é a escuta de uma região do espectro, não de um
  instrumento.
- **Outro crossover no RASGO:** o `MasterChain` do Rasgo Synth tem um
  crossover Linkwitz-Riley em tempo real. É outro contexto (é causal e roda
  dentro do callback de áudio), e o Rasgo Synth não é tocado por esta sessão.
  Este divisor é próprio do Trioio.
- **Teste:** `tests/BandSplitTests.cpp` confere a reconstrução, cada senoide
  na sua banda, o ganho limitado e os cortes inválidos.

## Etapa 1 — a leitura por linha (`ler --linhas`)

```
trioio_biblioteca ler <dados> <id> --linhas 200,2000        # 3 linhas
trioio_biblioteca ler <dados> <id> --linhas 150,600,4000    # 4 linhas
```

Primeiro sai a leitura da peça inteira; depois, para cada linha:

- a parte da energia;
- os ataques por segundo;
- o pulso e a relação com o da peça;
- a melodia;
- os acordes (ou as fundamentais, no grave);
- a tonalidade e a linguagem.

Cada leitura diz se **vale** naquela linha. As regras são heurísticas desta
etapa, ajustadas pelas medições abaixo:

| leitura | vale quando |
|---|---|
| pulso | confiança ≥ 0,25 e ≥ 0,5 ataque/s. Se for fraco mas no mesmo nível (ou metade/dobro) que a peça: "fraco, mas concorda com a peça" |
| melodia | a linha cobre parte de 400–2000 Hz e ≥ 25 % dos quadros têm altura. A saliência busca picos ≥ 250 Hz; no grave ela só acharia o vazamento do corte (o "Ré4" da primeira medição) |
| acordes | a linha cobre parte de 300–1500 Hz. Abaixo de 300 Hz o croma lê as **fundamentais** (o baixo), e é assim que a leitura do grave é apresentada |
| linguagem e tonalidade | só se a melodia ou os acordes valem |

### Medições (2026-10-13)

Recording_187 (`ref-f9796a7d29e2f8d9`, 3:44), cortes 200 e 2000 Hz:

| linha | energia | pulso | o que se lê |
|---|---|---|---|
| grave < 200 Hz | 6 % | 74,7 BPM, 0,57, ×1,00 | fundamentais Lá · Ré · Lá · Ré · Fá… (o baixo da cadência andaluza) |
| 200–2000 Hz | 56 % | 74,5 BPM, 0,58, ×1,00 | **maqam Hijaz sobre Lá**, Lá maior (a mesma da peça); acordes Lá, Rém, Sib… |
| agudo > 2000 Hz | 11 % | 73,9 BPM, 0,42, ×0,99 | melodia e acordes não valem (marcados) |

Suno (`ref-c2de6fb10363075a`, com bateria), cortes 200 e 2000 Hz:

- **Pulso:** cada linha sozinha tem confiança de 0,14 a 0,21, mas todas
  concordam com a peça, a ×1,00.
- **Grave:** fundamentais Sol · Fá# · Si · Ré.
- **Médio:** Sol menor (a mesma da peça), melakarta 13 sobre Ré; a peça
  inteira dava Hijaz/melakarta 14 sobre Ré.

Suno com 4 linhas (150, 600 e 4000 Hz):

- **Compasso:** o grave lê **3** tempos, enquanto as outras leem 4. É um
  material para as linhas livres da etapa 2, e uma dúvida a medir.
- **A linha de 600–4000 Hz** dá "Messiaen modo 3 sobre Sol". O filtro de
  linguagens ainda deixa passar os modos de Messiaen. Isso fica registrado
  como limite.

Custo: 76 s e 354 MB de pico para 3 linhas de uma gravação de 3:44. As linhas
são analisadas uma por vez, e a memória de cada linha volta antes da próxima.

## Proveniência

As linhas são derivados internos da referência. Ler exige o uso `analyze`;
compor sobre elas (etapa 2) exige `derive`, como hoje. Nada é gravado na
etapa 1: as linhas existem só na memória durante a leitura. Na etapa 2, o
manifesto da resposta deve registrar os cortes e o filtro.

## Etapa 2 — criar por linha (`responder --linhas`)

```
trioio_biblioteca responder <dados> <id> --modo seguir --alturas arquivo \
    --linhas 200,2000 --linhas-modos seguir,transfigurar,contrapor
trioio_biblioteca responder <dados> <id> --modo seguir --linhas 150,600,4000 --linhas-livres sim
```

Para cada linha (`composeLines`, em `src/library/Response.cpp`):

1. **Escuta própria.** A linha é dividida, levada ao nível da referência e
   analisada inteira.
2. **O que vale.** `assessLine` (`src/core/Lines.*`) aplica as mesmas regras
   do `ler --linhas`.
3. **Empréstimo de alturas.** Onde as alturas da linha não valem, os quadros
   dela recebem as da peça inteira: croma, melodia, YIN e picos
   (`borrowPitch`). A linha guarda o próprio ritmo, energia, brilho e
   ataques. No grave, as fundamentais também ficam as da linha.
4. **Um compositor próprio.**
   - modo: `--linhas-modos`, um por linha; os que faltam usam `--modo`;
   - semente: a da peça mais 7919 × o índice da linha;
   - o resto das opções vem da resposta (alturas, escalas, regras, escuta
     do autor).
5. **O pulso.** O regente dá a todas o pulso da peça (`Options::pulse`, novo
   no compositor). Com `--linhas-livres sim`, cada linha fica no seu pulso
   quando ele vale ou concorda com a peça; senão, recebe o da peça.
6. **O registro.** As notas da linha são dobradas em oitavas para a região
   dela (`bandRegister` e `foldIntoRegister`; a percussão fica). O agudo usa
   duas oitavas cujos parciais caem na banda.
7. **O filtro.** O som da linha passa pelo mesmo divisor e fica só na sua
   banda (`extractBand`). `--linhas-filtro nao` desliga o filtro.
8. **O equilíbrio.** Cada linha recebe a proporção de energia que a sua banda
   tem na referência, com o ganho limitado a ±12 dB. A soma vai até −1 dBFS
   de pico, e os stems recebem o mesmo ganho.

Saída:

- **O WAV da soma**, como sempre.
- **Os stems** em `saidas/linhas/<n>-<nome>.wav`. A soma dos stems é a
  resposta: diferença máxima de 3,6e-7.
- **No `decisoes.json`:**
  - `lines`, com, para cada linha: nome, cortes, parte da energia, modo,
    semente, BPM, pulso (peça ou linha), registro, ganho, de onde vieram as
    alturas, o que vale e o stem;
  - `line` em cada nota.
- **A forma** mostrada pela interface é a da linha que guia: a de mais
  energia entre as que têm melodia e acordes. As marcas das outras linhas
  levam o nome delas, entre colchetes.
- **O manifesto da derivação** declara as linhas, os cortes, o filtro e o
  pulso.

### Medições da etapa 2 (2026-10-13)

**Recording_187**, 3 linhas (grave segue, médio transfigura, agudo
contrapõe), pulso comum de 74,4 BPM:

- **Grave:** alturas da peça; fundamentais da linha.
- **Médio:** alturas próprias. Reconhece o Hijaz sobre Lá e compõe com o
  sayr (Nahawand sobre Ré).
- **Agudo:** alturas da peça.
- **Os stems ficam nas suas bandas:** 90 % da energia abaixo de 200 Hz no
  grave; 99 % entre 200 e 2000 Hz no médio; 84 % acima de 2 kHz no agudo.
- **O equilíbrio:** sem ele, o agudo era 1 % da energia da soma e ficava a
  −33 dBFS de RMS. Com ele, ganha +5 dB e fica em ~15 %, como na referência.
  Soma com pico de −1,0 dBFS, sem clipping.

**Suno**, 4 linhas livres:

- **Pulsos das linhas:** 84,5 · 84,6 · 84,8 · 85,1 BPM. A diferença é
  pequena e as linhas derivam devagar umas das outras: um politempo sutil,
  por desenho.
- **A soma:** 28/61/11 % de energia (grave/médio/agudo), próxima da
  referência.

**Testes:**

- `ResponseTests` §7: os stems somam a resposta, o teto de −1 dBFS, `line`
  em todas as notas, os modos por linha, o manifesto e o determinismo.
- `BandSplitTests` §5–6: `extractBand`, `assessLine`, `borrowPitch`, o
  registro e as dobras.

**Defeito achado pelo teste e corrigido:** o valor "peça" ia para o JSON por
`juce::String(const char*)`, que é só ASCII, e virava mojibake. Agora passa
por `u8(...)`, pela regra UTF-8 do RASGO.

**Limites:**

- **Cada linha é uma composição inteira dobrada para o seu registro.** O
  médio também traz um baixo e acordes, só que no médio. A divisão de papéis
  por linha (o grave só baixo e percussão, por exemplo) é uma decisão
  musical a tomar depois da escuta do autor.
- **O custo cresce com o número de linhas:** cada uma é uma análise mais uma
  composição.

## Próximo passo

1. A escuta do autor das duas respostas.
2. A etapa 3:
   - as linhas como faixas empilhadas na interface;
   - um stem por linha no ▶;
   - o julgamento por linha.
3. A decidir com a escuta: papéis por linha, e a linha livre com compasso
   próprio (o 3 do grave do Suno).
