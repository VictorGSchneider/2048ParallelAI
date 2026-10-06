# 2048ParallelAI

Algoritmo genético que evolui redes neurais para jogar 2048, pré-treinado com jogadas de um jogador Monte Carlo. A avaliação da população
(a parte cara) é paralelizada com OpenMP.

## Uso
```sh
make                     # gera ./2048-ga
./2048-ga [threads] [csv] [partidas_mc]  # padrão: 1 thread, speedup.csv, 16 partidas MC (0 = sem Monte Carlo)
./bench.sh [threads...]  # tabela de speedup (padrão: 1 2 4 8; REPS=3 por padrão)
make test                # testes do motor do jogo (inclui o gabarito "cobra")
make test_game && ./test_game   # jogue no terminal com w/a/s/d
```

## Estrutura
| Arquivo | Conteúdo |
|---|---|
| `game.c/.h` | motor do 2048 (grid em expoentes) |
| `network.c/.h` | rede 16→16→4 (tanh) e `network_predict` |
| `mc.c/.h` | jogador Monte Carlo (simulações aleatórias até travar) e geração do dataset de pré-treino |
| `genetic.c/.h` | população, avaliação paralela, seleção/crossover/mutação |
| `main.c` | loop de gerações e saída em CSV |
| `docs/ADR.md` | decisões de projeto, análise de paralelismo e resultados |

Cada geração imprime `best` (fitness), `cobra` (similaridade do tabuleiro final do melhor
indivíduo com o gabarito em zigue-zague, ver ADR §6) e `maior` (maior bloco).

Parâmetros do GA (população, mutação etc.) estão em `genetic.h`; o nº de gerações em `main.c`
(`-DNUM_GENERATIONS=N` para sobrescrever).
