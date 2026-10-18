# A comparação entre versões

```
trioio_biblioteca versoes <dados> <ref> <ref> [<ref>…]
```

As versões podem ser várias gerações do mesmo prompt, várias gravações da
mesma peça ou as tomadas de uma sessão. O que não muda entre elas é o que a
peça "é", e o que muda é o espaço em que o Trioio pode jogar. Tudo é lido no
som, na escuta de cada versão, nunca em parâmetros internos de um gerador
(`src/core/Versions.h`).

## O que se lê de cada versão

- a duração, o andamento e a confiança do pulso, e as seções;
- a tonalidade (pelos acordes lidos);
- a fundamental do acorde em 64 pontos da peça (a progressão);
- a curva de energia em 32 pontos (a forma);
- a intenção ouvida: energia, densidade, brilho, harmonicidade e humor
  ([`PROMPTS_E_INTENCOES.md`](PROMPTS_E_INTENCOES.md)).

## Invariantes e divergências

A primeira versão é a referência, e cada outra é comparada com ela:

| o quê | invariante quando |
|---|---|
| andamento | a razão perto de 1, 2 ou ½ (±4 %), com os dois pulsos confiáveis (≥ 0,3) |
| tonalidade | o mesmo centro e o mesmo modo (o mesmo centro em outro modo é dito à parte) |
| forma | a correlação das curvas de energia ≥ 0,7 |
| progressão | ≥ 50 % das fundamentais iguais, onde as duas têm acorde |
| duração | diferença de no máximo 10 % |
| intenção ouvida | cada campo que muda 0,2 ou mais é uma divergência |

## As operações

O que o Trioio pode fazer com o resultado, já nos comandos:

- **mesmo andamento e mesmo tom:** o mashup (`responder A --com B`), em que
  as camadas de uma casam com as da outra;
- **mesmo andamento e outro tom:** o mashup só do ritmo (`--da-outra
  ritmo`);
- **mesma forma com outra matéria:** seguir a forma de uma e o campo da
  outra;
- **o que diverge:** é o espaço do Terceiro Olho. Dá para responder a uma
  com a intenção da outra (`prompt --revisar` com os valores da outra);
- **mesma progressão:** reler com outra linguagem harmônica;
- **sempre:** as linhas (`--linhas`), para ver em que banda as versões
  diferem.

## O conjunto de versões geradas como referência

As versões de um gerador entram como qualquer referência:

- **a autorização:** são autorizadas uma a uma pela Biblioteca
  (`autorizar … --usos …`), e a proveniência fica no manifesto;
- **o prompt:** vai para o `prompts.json` com a fonte `gerador`;
- **a comparação:** é a deste comando.

O Trioio **recombina por autoria**: lê, compara e responde em síntese
própria. Não copia o áudio e não treina nem alimenta o serviço que gerou as
versões.

## Testado

**`VersionsTests`:**

- duas versões iguais, uma no dobro do andamento, dão os cinco invariantes e
  o mashup;
- uma terceira em outro tom, com a forma invertida, outra progressão e
  outro brilho, dá as quatro divergências;
- uma versão só não compara nada.

**Sobre as três referências reais** (Recording_187, suno, vocalise), que são
peças diferentes: nenhum invariante, e as divergências ditas uma a uma.
