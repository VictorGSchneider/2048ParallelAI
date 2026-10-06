#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "genetic.h"

#define NUM_GENERATIONS 50

int main(int argc, char **argv) {
    int num_threads = (argc > 1) ? atoi(argv[1]) : 1;
    omp_set_num_threads(num_threads);

    static Individual pop[POP_SIZE], next[POP_SIZE];
    population_init(pop, 12345);

    double total_eval_time = 0.0;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        double t0 = omp_get_wtime();
        population_evaluate(pop, (unsigned int)(gen * 1000 + 1));
        double t1 = omp_get_wtime();

        int best = population_best(pop);
        total_eval_time += (t1 - t0);

        // TODO: printar gen, melhor fitness, tempo da evaluate (stdout ou CSV)
        printf("gen %d | best %.1f | eval %.3fs\n", gen, pop[best].fitness, t1 - t0);

        population_evolve(pop, next, (unsigned int)(gen * 1000 + 7));
        memcpy(pop, next, sizeof(pop));
    }

    // TODO: gravar threads,total_eval_time num CSV (append) -> base do gráfico de speedup
    printf("threads=%d total_eval_time=%.3fs\n", num_threads, total_eval_time);
    return 0;
}
