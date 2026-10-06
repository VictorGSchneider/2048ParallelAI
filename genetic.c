#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "genetic.h"
#include "game.h"

#define TOURNAMENT_SIZE 3
#define NUM_WEIGHTS (sizeof(Network) / sizeof(double))

// Network é só doubles, então dá pra tratá-la como um vetor plano de pesos (crossover/mutação).
_Static_assert(sizeof(Network) % sizeof(double) == 0, "Network deve conter apenas doubles");

// Mistura (splitmix32-like) para derivar seeds independentes a partir de (base, índice).
// Seeds consecutivas (base+i) geram sequências correlacionadas no rand_r; o hash evita isso.
static unsigned int mix_seed(unsigned int base, unsigned int idx) {
    unsigned int z = base + 0x9e3779b9u * (idx + 1u);
    z = (z ^ (z >> 16)) * 0x85ebca6bu;
    z = (z ^ (z >> 13)) * 0xc2b2ae35u;
    return z ^ (z >> 16);
}

static double rand_unit(unsigned int *seed) {  // (0, 1]
    return ((double)rand_r(seed) + 1.0) / ((double)RAND_MAX + 1.0);
}

static double rand_gaussian(unsigned int *seed) {  // Box-Muller
    double u1 = rand_unit(seed), u2 = rand_unit(seed);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

void population_init(Individual pop[POP_SIZE], unsigned int base_seed) {
    for (int i = 0; i < POP_SIZE; i++) {
        unsigned int seed = mix_seed(base_seed, (unsigned int)i);
        network_init_random(&pop[i].net, &seed);
        pop[i].fitness = 0.0;
    }
}

// Joga UMA partida inteira com a rede. Retorna o score final.
static double play_one_game(const Network *net, unsigned int *seed) {
    Board board;
    board_init(&board, seed);

    for (int moves = 0; moves < MAX_MOVES_PER_GAME; moves++) {
        Direction dir = network_predict(net, &board);
        // network_predict só devolve um movimento inválido se NENHUM for válido: game over.
        if (!board_move(&board, dir)) break;
        board_spawn_tile(&board, seed);
    }
    return board.score;
}

void population_evaluate(Individual pop[POP_SIZE], unsigned int gen_seed) {
    // Compartilhado: pop (cada iteração escreve só em pop[i].fitness, i distinto: sem corrida).
    // Privado: seed, total, g (declarados dentro do loop) e o Board (local a play_one_game).
    // A seed deriva só de (gen_seed, i), então o resultado independe do nº de threads.
    // dynamic: a duração das partidas varia muito (redes boas jogam bem mais lances).
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < POP_SIZE; i++) {
        unsigned int seed = mix_seed(gen_seed, (unsigned int)i);
        double total = 0.0;
        for (int g = 0; g < GAMES_PER_INDIVIDUAL; g++) {
            total += play_one_game(&pop[i].net, &seed);
        }
        pop[i].fitness = total / GAMES_PER_INDIVIDUAL;
    }
}

int population_best(const Individual pop[POP_SIZE]) {
    int best = 0;
    for (int i = 1; i < POP_SIZE; i++)
        if (pop[i].fitness > pop[best].fitness) best = i;
    return best;
}

typedef struct { double fitness; int idx; } Ranked;

static int cmp_ranked_desc(const void *a, const void *b) {
    double fa = ((const Ranked *)a)->fitness, fb = ((const Ranked *)b)->fitness;
    return (fa < fb) - (fa > fb);
}

// Torneio: sorteia TOURNAMENT_SIZE indivíduos e devolve o de maior fitness.
static int tournament_select(const Individual pop[POP_SIZE], unsigned int *seed) {
    int best = (int)(rand_r(seed) % POP_SIZE);
    for (int t = 1; t < TOURNAMENT_SIZE; t++) {
        int cand = (int)(rand_r(seed) % POP_SIZE);
        if (pop[cand].fitness > pop[best].fitness) best = cand;
    }
    return best;
}

void population_evolve(const Individual pop[POP_SIZE], Individual next[POP_SIZE], unsigned int gen_seed) {
    unsigned int seed = mix_seed(gen_seed, 0xffffffffu);

    // 1) elitismo
    Ranked ranked[POP_SIZE];
    for (int i = 0; i < POP_SIZE; i++) { ranked[i].fitness = pop[i].fitness; ranked[i].idx = i; }
    qsort(ranked, POP_SIZE, sizeof(Ranked), cmp_ranked_desc);

    for (int e = 0; e < ELITE_COUNT; e++) next[e] = pop[ranked[e].idx];

    // 2) torneio + crossover uniforme + mutação gaussiana
    for (int i = ELITE_COUNT; i < POP_SIZE; i++) {
        const double *pa = (const double *)&pop[tournament_select(pop, &seed)].net;
        const double *pb = (const double *)&pop[tournament_select(pop, &seed)].net;
        double *child = (double *)&next[i].net;

        for (size_t w = 0; w < NUM_WEIGHTS; w++) {
            child[w] = (rand_r(&seed) & 1) ? pa[w] : pb[w];
            if (rand_unit(&seed) < MUTATION_RATE)
                child[w] += MUTATION_STRENGTH * rand_gaussian(&seed);
        }
        next[i].fitness = 0.0;
    }
}
