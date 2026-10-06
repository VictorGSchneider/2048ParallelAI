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

## 6. Paralelismo (`population_evaluate`)
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
| 1 | 2,53 | 1,00 | 100% |
| 2 | 1,29 | 1,96 | 98% |
| 4 | 0,66 | 3,85 | 96% |
| 8 | 0,66 | 3,53 | 44% |

Com 8 threads em 4 cores não há ganho (oversubscription). Rode `./bench.sh` na sua máquina
para refazer a medição; os dados brutos ficam em `speedup.csv`.
