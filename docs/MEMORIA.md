# TRIOIO — memória performativa (oráculo de fatores)

**Estado:** implementado em 2026-10-08 (`src/core/OracleMemory.*`,
`tests/OracleMemoryTests.cpp`).

## De onde vem

O Atlas de Referência RASGO, na §63 (OMax, IRCAM), propõe a **memória
performativa**: "o sistema escuta, aprende, reorganiza, responde", com
aplicações como "repetir deformado", "reconstruir" e "gerar memória falsa".
O núcleo do OMax é um algoritmo público, o **oráculo de fatores**:

- C. Allauzen, M. Crochemore e M. Raffinot, "Factor oracle: a new structure
  for pattern matching", SOFSEM 1999;
- G. Assayag e S. Dubnov, "Using factor oracles for machine improvisation",
  *Soft Computing* 8, 2004.

A implementação do Trioio é própria, feita do zero a partir dos artigos. O
código do OMax não foi consultado (é distribuído pelo Forum do IRCAM, com
termos próprios).

## O que faz

1. **Aprende:** cada ataque da referência com altura definida vira um
   evento (altura em MIDI inteiro, distância até o próximo ataque e força).
   As alturas entram, em ordem, num oráculo de fatores. A estrutura
   reconhece todo trecho contínuo da sequência e liga cada ponto ao ponto
   anterior que tem o **mesmo contexto** (link de sufixo).
2. **Reorganiza:** a navegação segue adiante na ordem original ou **salta**
   pelo link de sufixo para outro ponto da gravação que compartilha o
   contexto, e segue dali. Se o contexto nunca se repetiu antes, salta para
   outra ocorrência da mesma nota. Assim cada par de notas seguidas continua
   sendo um par que existe na referência, mas a sequência não é mais a dela:
   é reconhecível e imprevisível.
3. **Responde:** de 1 a 4 passagens de 6 a 16 notas (1 + 3 ×
   imprevisibilidade) entram na partitura numa voz própria (`voice` 3, canal
   4 do MIDI), com o **corpo espectral** da referência, em lugares livres da
   peça. Com imprevisibilidade alta a navegação salta mais: a continuidade é
   0,85 − 0,5 × imprevisibilidade.
4. **Segundo o modo:**
   - **seguir:** a memória como ela é;
   - **transfigurar:** transposta uma quinta acima ou uma quarta abaixo;
   - **contrapor:** espelhada em torno da altura média ("memória falsa");
   - **interromper:** como ela é.

A memória entra **antes** do Terceiro Olho. O olho a lê e pode deformá-la
como deforma o resto (escala mutante, registro invertido, acorde que vira
campo). Cada passagem fica na forma, por exemplo "memória (oráculo): 12
notas da referência recombinadas, 4 saltos". Com imprevisibilidade 0 não
há memória, e a composição fica como antes.

## Validação

`tests/OracleMemoryTests.cpp`:

- **reconhecimento:** o oráculo reconhece todos os fatores de uma sequência
  de 14 notas, e cada link de sufixo aponta para a mesma nota (contexto
  comum);
- **navegação:**
  - com continuidade 1, ela devolve a ordem original;
  - com continuidade 0,4 (352 saltos em 50 passeios), **530 de 550 pares
    seguidos (96%) existem na referência**. Antes da correção do salto
    eram 83%: um contexto que nunca se repetiu levava a um ponto
    aleatório;
- **na partitura:**
  - **195 de 195** notas da memória têm alturas da referência (medido antes
    do Terceiro Olho);
  - sem imprevisibilidade não há memória;
  - em contrapor ela vem espelhada.

## Limites e próximos passos

- **O símbolo é só a altura.** O OMax usa símbolos ricos (altura + duração,
  ou descritores de timbre). Um próximo passo é aprender também o ritmo e
  o timbre (o corpo espectral de cada evento).
- **O corpo de onde veio (2026-10-08).** Cada nota da memória guarda o
  instante de origem (`Note::source`): a família espectral toca com o
  corpo daquele instante, não com o do instante em que soa. O mesmo
  mecanismo serve ao mosaico por timbre (`docs/MOSAICO.md`, §62), a
  segunda memória do Trioio, que lembra pelo som em vez da ordem.
- **A memória é da referência.** O OMax também aprende o que **ele mesmo**
  tocou. Uma memória das respostas anteriores do Trioio, com a escuta do
  autor (`docs/ESCUTA.md`) dizendo quais valem a pena lembrar, fecharia o
  ciclo.
- **Ao vivo:** o oráculo é incremental por construção (`add` a cada
  ataque), o que o deixa pronto para a escuta em tempo real.
