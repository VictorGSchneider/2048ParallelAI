#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "genetic.h"
#include "mc.h"

#ifndef PRETRAIN_GENERATIONS
#define PRETRAIN_GENERATIONS 30  // gerações de pré-treino por imitação do Monte Carlo
#endif

#ifndef POP_SEED
#define POP_SEED 12345
#endif

#ifndef NUM_GENERATIONS
#define NUM_GENERATIONS 50
#endif

// Uso: ./2048-ga [threads] [csv_de_speedup] [partidas_monte_carlo]   (0 = sem Monte Carlo)
int main(int argc, char **argv) {
    int num_threads = (argc > 1) ? atoi(argv[1]) : 1;
    if (num_threads < 1) num_threads = 1;
    const char *csv_path = (argc > 2) ? argv[2] : "speedup.csv";
    int mc_games = (argc > 3) ? atoi(argv[3]) : MC_GAMES;
    omp_set_num_threads(num_threads);

    static Individual pop[POP_SIZE], next[POP_SIZE];
    population_init(pop, POP_SEED);

    // Fase 1 (Monte Carlo): o MC joga partidas inteiras e grava (tabuleiro -> melhor jogada).
    // Fase 2 (pré-treino): o GA evolui a população para imitar essas jogadas.
    // Fase 3: o GA segue com o fitness normal (score jogando de verdade), partindo da população pré-treinada.
    if (mc_games > 0) {
        Dataset ds;
        double t0 = omp_get_wtime();
        dataset_generate(&ds, mc_games, MC_KEEP_GAMES, MC_ROLLOUTS, 2024);
        printf("monte carlo: %d partidas (score medio %.1f, melhor %d); %d jogadas das melhores viram exemplo (%.2fs)\n",
               ds.games, ds.avg_score, ds.best_score, ds.count, omp_get_wtime() - t0);

        // Teste de cada iteração do Monte Carlo (uma partida): comparação com o gabarito "cobra".
        for (int g = 0; g < ds.games; g++) {
            const GameStat *st = &ds.stats[g];
            printf("mc %d | score %d | jogadas %d | maior %d | cobra final %3.0f%% | cobra media %3.0f%%%s\n",
                   g, st->score, st->moves, st->max_tile, 100.0 * st->snake_final, 100.0 * st->snake_avg,
                   st->kept ? " | *" : "");
        }
        const char *why = "";
        if (!dataset_verify(&ds, &why)) {
            fprintf(stderr, "dataset Monte Carlo invalido: %s\n", why);
            return 1;
        }

        for (int gen = 0; gen < PRETRAIN_GENERATIONS; gen++) {
            population_evaluate_imitation(pop, &ds, (unsigned int)(gen * 1000 + 3));
            printf("pre %d | imitacao %.1f%%\n", gen, 100.0 * pop[population_best(pop)].fitness);
            population_evolve(pop, next, (unsigned int)(gen * 1000 + 5));
            memcpy(pop, next, sizeof(pop));
        }
        dataset_free(&ds);
    }

    double total_eval_time = 0.0;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        double t0 = omp_get_wtime();
        population_evaluate(pop, (unsigned int)(gen * 1000 + 1));
        double t1 = omp_get_wtime();

        int best = population_best(pop);
        total_eval_time += (t1 - t0);

        // Acompanhamento por geração: score puro, similaridade média com o gabarito "cobra"
        // (1.0 = perfeitamente organizado) e maior bloco numa partida de replay do melhor.
        Board final;
        play_report(&pop[best].net, (unsigned int)(gen * 1000 + 11), &final);
        int max_exp = 0;
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++)
                if (final.grid[r][c] > max_exp) max_exp = final.grid[r][c];

        printf("gen %d | best %.1f | score %.1f | cobra %3.0f%% | maior %d | eval %.3fs\n", gen,
               pop[best].fitness, pop[best].score, 100.0 * pop[best].snake, 1 << max_exp, t1 - t0);

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
