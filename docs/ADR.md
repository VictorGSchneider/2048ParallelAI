# ADR — Decisões de projeto do 2048ParallelAI

Algoritmo genético que evolui redes neurais para jogar 2048, com a avaliação da população paralelizada em OpenMP.

## 1. Representação do tabuleiro: expoente, não valor
O grid guarda `exp` (0 = vazio, 1 = 2, 2 = 4, ...); o valor é `1 << exp`.
- Merge vira `v + 1`, e comparar tiles é comparar inteiros pequenos.
- A entrada da rede já sai em escala log2, sem precisar de `log()` por célula.
- O score continua somando o valor real (`1 << (v + 1)`) de cada merge.

## 2. Motor do jogo
`board_move` implementa só "mover uma linha para a esquerda" (`slide_line`); as outras 3 direções
são reduzidas a isso por `line_cell`, que mapeia o índice `k` da linha para a coordenada real.
Cada tile participa de no máximo um merge por jogada (`[2,2,4]` ← esquerda = `[4,4,0]`).
`game_over` só é recalculado em `board_spawn_tile` quando o tabuleiro fica cheio (único caso em que
pode estar travado). Testes: `make test`.

## 3. Rede neural: 16 → 16 → 4, tanh
- **Entrada:** exponentes divididos por 17 (≈ [0, 1]). O valor bruto cresce exponencialmente e uma
  tile grande dominaria a soma ponderada; em log2 a escala é comparável entre células.
- **Oculta:** 16 neurônios, `tanh` (saída limitada, simétrica, boa para pesos em [-1, 1]).
- **Saída:** 4 notas lineares; só o argmax importa. `network_predict` escolhe a maior nota entre os
  movimentos **válidos**, para a rede não ficar presa tentando um movimento que não muda nada.
- 16 neurônios ocultos (324 pesos) foi o compromisso: o forward pass domina o custo de cada lance,
  e o GA só precisa de uma rede capaz de aprender heurísticas simples (tiles grandes juntas, no canto).

## 4. Algoritmo genético
| Etapa | Escolha | Motivo |
|---|---|---|
| Fitness | média do score em 5 partidas | reduz o ruído do RNG do jogo |
| Elitismo | 4 melhores copiados direto | nunca perde o melhor indivíduo |
| Seleção | torneio de 3 | barato, sem ordenar/normalizar fitness, pressão seletiva ajustável |
| Crossover | uniforme (cada peso vem de um pai) | os pesos não têm ordem espacial que um ponto de corte preservaria |
| Mutação | 5% por peso, ruído gaussiano σ = 0,3 | perturbação pequena e local |

Com 400 gerações o melhor fitness médio sobe de ~4.200 (gerações 0–49) para ~6.100 (350–399).
O fitness oscila entre gerações porque cada geração usa partidas novas (seed depende de `gen_seed`).

## 5. Monte Carlo + pré-treino do GA
Antes do GA de jogo, um jogador Monte Carlo (`mc.c`) gera exemplos:
- **Jogada MC:** para cada uma das 4 direções válidas, aplica o movimento e faz `MC_ROLLOUTS` (10)
  partidas **aleatórias até travar**; escolhe a direção de maior score médio (a média é mais robusta
  que o máximo, que premia sorte). É uma busca "achatada": simulações aleatórias em vez de expandir a
  árvore completa 4→16→64..., que cresce exponencialmente e não é viável até o fim da partida.
- **Dados:** o MC joga `MC_GAMES` (16) partidas completas e só as `MC_KEEP_GAMES` (4) de maior score
  (as que "mais avançaram") viram exemplos `(tabuleiro → jogada)`. Opcionalmente `MC_MIN_MARGIN`
  descarta jogadas em que o MC está em dúvida (padrão 0 = desligado).
- **Pré-treino:** `PRETRAIN_GENERATIONS` (30) gerações do GA com fitness = fração de jogadas MC que a
  rede reproduz (`population_evaluate_imitation`, paralelo). Depois o GA segue com o fitness normal.
- **Teste de cada iteração do MC:** cada partida MC é comparada com o gabarito "cobra" (§6) e a saída
  mostra uma linha por partida (`mc g | score | jogadas | maior | cobra final | cobra média`, com `*`
  nas partidas mantidas). Antes do pré-treino, `dataset_verify` confere cada jogada gravada (precisa
  ser válida no tabuleiro gravado, expoentes em 0..17) e a coerência das estatísticas; se falhar, o
  programa aborta. `make test` roda a mesma verificação e confirma que ela detecta um dado adulterado.
  Achado: o MC chega a 1024–2048, mas com cobra de só ~12–25% (média ~20%), no nível do GA sem bônus.
  Ele maximiza score nas simulações aleatórias e não organiza o tabuleiro em cobra, então suas
  jogadas não são um bom "professor" para essa organização.
- As partidas MC são paralelizadas (uma por iteração, `schedule(dynamic)`); o resultado independe das threads.
  `./2048-ga <threads> <csv> 0` desliga o Monte Carlo.

**O MC joga bem** (score médio ≈ 11.000, melhor ≈ 30.000, em ~0,6 s com 4 threads), contra ≈ 4.000–6.000
do GA. **Mas imitá-lo não ajudou o GA:** a acurácia de imitação fica em ~33–37% (as jogadas do MC se
distribuem de forma uniforme nas 4 direções e o alvo é ruidoso com poucas simulações).
Medição (média do melhor fitness em 20 gerações, 6 sementes de população; ruído ≈ ±130):

| Configuração | Média |
|---|---|
| sem MC, 50 gerações | 4.521 |
| MC padrão + 30 de pré-treino, 50 gerações | 4.982 |
| **controle:** sem MC, 80 gerações (mesmo total de gerações) | **5.085** |
| MC com 50 simulações, 32 partidas, 300 de pré-treino | 4.627 |
| MC com 30 simulações + filtro de margem 0,15, 100 de pré-treino | 4.619 |

O ganho aparente de +10% da 2ª linha vem das 30 gerações extras (o controle justo é a 3ª linha).
Conclusão: com esta rede/GA, o pré-treino por imitação é neutro. O MC fica como componente opcional;
caminhos que provavelmente rendem mais são usá-lo como parte do fitness ou como jogador-professor
numa rede maior.

## 6. Gabarito "cobra" (referência de organização)
O tabuleiro de referência (`board_load_gabarito`) é a configuração perfeita em zigue-zague:

```
65536 32768 16384  8192
  512  1024  2048  4096
  256   128    64    32
    2     4     8    16
```
`board_gabarito_similarity` ordena os blocos do maior pro menor e compara com o caminho da cobra
(linha 0 da esquerda pra direita, linha 1 da direita pra esquerda...): a fração das 16 posições
que têm o bloco certo (vazias por último). O gabarito dá 1,0. Comparar por valor torna empates
irrelevantes e funciona para qualquer tabuleiro, não só o final.
- **A cada `make test`:** o gabarito é conferido célula a célula contra os valores da referência, é um
  game over válido, dá nota 1,0, e variações (linha espelhada, cantos trocados) dão nota menor.
- **Bônus de fitness:** `fitness = score × (1 + SNAKE_WEIGHT × cobra)`, com `SNAKE_WEIGHT = 3` e `cobra`
  = similaridade **média ao longo da partida** (não só do tabuleiro final, que quase sempre é um
  tabuleiro travado e pouco informativo), média das `GAMES_PER_INDIVIDUAL` partidas. O bônus é
  multiplicativo de propósito: só amplifica quem já pontua, então uma rede que trava cedo com dois
  blocos bem arrumados não ganha fitness alto. Cada indivíduo guarda também o `score` puro e o `cobra`.
- **A cada geração:** a saída mostra `best` (fitness com bônus), `score` (puro), `cobra` (%) e
  `maior` (maior bloco numa partida de replay do melhor).

Medição (6 sementes, sem Monte Carlo, melhor indivíduo nas últimas 20 gerações; ruído do score ≈ ±130):

| Peso | 50 gerações: score / cobra | 150 gerações: score / cobra |
|---|---|---|
| 0 (sem bônus) | 4.521 / 19,8% | 5.607 / 20,2% |
| 1 | 4.653 / 41,4% | — |
| **3 (padrão)** | **4.688 / 41,5%** | **5.803 / 42,0%** |
| 10 | 4.641 / 42,9% | 5.672 / 42,5% |
| 30 | 4.703 / 42,9% | — |

O bônus **dobra a organização** (≈20% → ≈42%) e traz um ganho pequeno mas consistente de score
(+3–4% em 50 e em 150 gerações). O efeito satura a partir do peso ~1; 3 foi escolhido por ficar
no patamar sem dominar o fitness. Custo: ~20% mais tempo no `evaluate` (a similaridade é calculada a
cada lance), sem mudar o speedup. `SNAKE_WEIGHT = 0` reproduz exatamente o comportamento anterior.
Mesmo assim a rede ainda está longe do gabarito (maior bloco ~250–300), então o ganho de score é modesto.

## 7. Paralelismo (`population_evaluate`)
`#pragma omp parallel for schedule(dynamic)` sobre os indivíduos.
- **Compartilhado:** o array `pop`. Cada iteração escreve só em `pop[i].fitness` (i distinto): sem corrida.
- **Privado:** `seed`, `total`, `g` (declaradas dentro do loop) e o `Board` (local a `play_one_game`).
- **Seeds:** `mix_seed(gen_seed, i)` (hash estilo splitmix32). `gen_seed + i` geraria sequências
  correlacionadas no `rand_r`. Como a seed depende só de `(gen_seed, i)`, o resultado é
  **idêntico para qualquer nº de threads** (verificado: mesma saída com 1, 2 e 4 threads, inclusive a fase Monte Carlo).
- **`schedule(dynamic)`:** a duração das partidas varia bastante (redes boas jogam mais lances),
  então distribuir sob demanda balanceia melhor que blocos estáticos. O custo de despacho é
  desprezível frente a ~35 ms por geração.

### Amdahl: `population_evolve` fica sequencial
Medido: `evolve` ≈ 0,3 ms por geração contra ≈ 45 ms de `evaluate` (≈ 0,7% do tempo). Pela lei de
Amdahl o teto de speedup seria ~140×, muito acima do que o hardware oferece; paralelizá-la
só acrescentaria overhead.

### Resultados (`./bench.sh`, 50 gerações da fase de jogo, máquina de 4 cores)
| threads | tempo (s) | speedup | eficiência |
|---|---|---|---|
| 1 | 3,06 | 1,00 | 100% |
| 2 | 1,57 | 1,95 | 97% |
| 4 | 0,79 | 3,90 | 98% |
| 8 | 0,66 | 3,53 | 44% |

Com 8 threads em 4 cores não há ganho (oversubscription). Rode `./bench.sh` na sua máquina
para refazer a medição; os dados brutos ficam em `speedup.csv`.
