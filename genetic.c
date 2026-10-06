#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "genetic.h"
#include "game.h"

void population_init(Individual pop[POP_SIZE], unsigned int base_seed) {
    // TODO: pra cada indivíduo, seed própria derivada de base_seed + i; network_init_random; fitness = 0
    (void)pop; (void)base_seed;
}

// Joga UMA partida inteira com a rede. Retorna o score final.
static double play_one_game(const Network *net, unsigned int *seed) {
    // TODO: board_init; loop { dir = network_predict; board_move; se mudou -> board_spawn_tile }
    //       até game over ou MAX_MOVES_PER_GAME. Retorna board.score.
    (void)net; (void)seed;
    return 0.0;
}

void population_evaluate(Individual pop[POP_SIZE], unsigned int gen_seed) {
    // PASSO 1: faça sequencial e confirme que funciona.
    // PASSO 2: adicione o #pragma omp parallel for e pense em CADA variável:
    //   - quem é privada? (seed local, board local, acumulador do fitness)
    //   - quem é compartilhada? (o array pop)
    //   - tem escrita concorrente na mesma posição? (cada thread escreve em pop[i].fitness, i distinto)
    //   - schedule(static) ou schedule(dynamic)? Partidas têm duração variável... mede e justifica na ADR.
    for (int i = 0; i < POP_SIZE; i++) {
        unsigned int seed = gen_seed + (unsigned int)i;  // TODO: conferir se essa derivação é boa o bastante
        double total = 0.0;
        for (int g = 0; g < GAMES_PER_INDIVIDUAL; g++) {
            total += play_one_game(&pop[i].net, &seed);
        }
        pop[i].fitness = total / GAMES_PER_INDIVIDUAL;
    }
}

int population_best(const Individual pop[POP_SIZE]) {
    // TODO: argmax do fitness
    (void)pop;
    return 0;
}

void population_evolve(const Individual pop[POP_SIZE], Individual next[POP_SIZE], unsigned int gen_seed) {
    // TODO:
    // 1) elitismo: copiar os ELITE_COUNT melhores direto
    // 2) pro resto: selecionar 2 pais (torneio ou roleta, escolhe e justifica na ADR),
    //    crossover dos pesos (ponto de corte único, uniforme, ou média), e mutação.
    // Obs: essa função é sequencial de propósito. Mede quanto tempo ela leva vs. a evaluate.
    //      Se for desprezível, a lei de Amdahl diz que você fez a escolha certa. Isso vira parágrafo da ADR.
    (void)pop; (void)next; (void)gen_seed;
}
