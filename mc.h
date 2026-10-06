#ifndef MC_H
#define MC_H

#include "game.h"

#ifndef MC_ROLLOUTS
#define MC_ROLLOUTS   10    // simulações aleatórias por direção, a cada jogada
#endif
#ifndef MC_GAMES
#define MC_GAMES      16    // partidas completas jogadas pelo jogador Monte Carlo
#endif
#ifndef MC_MIN_MARGIN
#define MC_MIN_MARGIN 0.0   // descarta exemplos em que o MC não tem confiança (margem menor que isso)
#endif

#ifndef MC_KEEP_GAMES
#define MC_KEEP_GAMES 4     // só as jogadas das N melhores partidas (as que mais pontuaram) viram exemplo
#endif
#define MC_MAX_MOVES  3000  // trava de segurança por partida

// Um exemplo de treino: tabuleiro (expoentes) + a direção que o Monte Carlo escolheu.
typedef struct {
    unsigned char cells[BOARD_SIZE * BOARD_SIZE];
    unsigned char move;
} Sample;

typedef struct {
    Sample *samples;
    int     count;
    int     games;
    double  avg_score;  // score médio das partidas jogadas pelo Monte Carlo
    int     best_score; // melhor partida (a que "mais avançou")
} Dataset;

// Joga uma partida só com movimentos aleatórios (válidos) até travar. Retorna o score.
// `b` é uma cópia: o original não é alterado.
int mc_random_playout(Board b, unsigned int *seed);

// Para cada uma das 4 direções válidas: aplica o movimento, sorteia o spawn e
// faz `rollouts` partidas aleatórias até perder. Retorna a direção com maior
// score médio. Se nenhuma direção é válida (game over), devolve DIR_UP e o
// chamador percebe via board_move == 0.
Direction mc_choose_move(const Board *b, int rollouts, unsigned int *seed);

// Igual a mc_choose_move, mas devolve em *margin a vantagem relativa da melhor direção
// sobre a segunda melhor: (melhor - segunda) / melhor, em [0, 1]. Perto de 0 = empate ruidoso.
Direction mc_choose_move_margin(const Board *b, int rollouts, unsigned int *seed, double *margin);

// Joga `games` partidas inteiras com o Monte Carlo (em paralelo, uma partida por
// iteração) e guarda (tabuleiro, jogada escolhida) só das `keep` partidas de maior score.
// Resultado independe do nº de threads. avg_score/best_score descrevem as `games` partidas.
void dataset_generate(Dataset *ds, int games, int keep, int rollouts, unsigned int base_seed);
void dataset_free(Dataset *ds);

#endif
