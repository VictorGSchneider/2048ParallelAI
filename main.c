#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "genetic.h"

#ifndef NUM_GENERATIONS
#define NUM_GENERATIONS 50
#endif

// Uso: ./2048-ga [threads] [csv_de_speedup]
int main(int argc, char **argv) {
    int num_threads = (argc > 1) ? atoi(argv[1]) : 1;
    if (num_threads < 1) num_threads = 1;
    const char *csv_path = (argc > 2) ? argv[2] : "speedup.csv";
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

        printf("gen %d | best %.1f | eval %.3fs\n", gen, pop[best].fitness, t1 - t0);

        population_evolve(pop, next, (unsigned int)(gen * 1000 + 7));
        memcpy(pop, next, sizeof(pop));
    }

    printf("threads=%d total_eval_time=%.3fs\n", num_threads, total_eval_time);

    // Append: uma linha por execução (threads,total_eval_time) -> base do gráfico de speedup.
    FILE *csv = fopen(csv_path, "a+");
    if (!csv) { perror(csv_path); return 1; }
    fseek(csv, 0, SEEK_END);  // em "a+" o ftell inicial é 0 mesmo com arquivo existente
    if (ftell(csv) == 0) fprintf(csv, "threads,total_eval_time\n");
    fprintf(csv, "%d,%.6f\n", num_threads, total_eval_time);
    fclose(csv);
    return 0;
}
