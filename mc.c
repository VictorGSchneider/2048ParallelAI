#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "mc.h"
#include "rng.h"

int mc_random_playout(Board b, unsigned int *seed) {
    for (;;) {
        // Sorteia uma direção e, se for inválida, tenta as seguintes em ordem circular.
        int start = (int)(rand_r(seed) % 4), moved = 0;
        for (int k = 0; k < 4 && !moved; k++)
            moved = board_move(&b, (Direction)((start + k) % 4));
        if (!moved) return b.score;  // nenhuma direção muda o grid: perdeu
        board_spawn_tile(&b, seed);
    }
}

Direction mc_choose_move_margin(const Board *b, int rollouts, unsigned int *seed, double *margin) {
    Direction best = DIR_UP;
    double best_mean = -1.0, second_mean = -1.0;

    for (int d = 0; d < 4; d++) {
        Board first = *b;
        if (!board_move(&first, (Direction)d)) continue;  // movimento inválido
        board_spawn_tile(&first, seed);

        double total = 0.0;
        for (int r = 0; r < rollouts; r++) total += mc_random_playout(first, seed);

        double mean = total / rollouts;
        if (mean > best_mean) { second_mean = best_mean; best_mean = mean; best = (Direction)d; }
        else if (mean > second_mean) second_mean = mean;
    }
    // Com uma única direção válida não há dúvida (margem 1).
    if (margin) *margin = (second_mean < 0 || best_mean <= 0) ? 1.0 : (best_mean - second_mean) / best_mean;
    return best;
}

Direction mc_choose_move(const Board *b, int rollouts, unsigned int *seed) {
    return mc_choose_move_margin(b, rollouts, seed, NULL);
}

void dataset_generate(Dataset *ds, int games, int keep, int rollouts, unsigned int base_seed) {
    if (keep > games) keep = games;
    Sample *buf = malloc((size_t)games * MC_MAX_MOVES * sizeof(Sample));
    int *counts = calloc((size_t)games, sizeof(int));
    int *scores = calloc((size_t)games, sizeof(int));

    // Cada partida escreve só na sua fatia de `buf` e em counts[g]/scores[g]: sem corrida.
    // dynamic: a duração das partidas varia muito (o Monte Carlo joga partidas longas).
    #pragma omp parallel for schedule(dynamic)
    for (int g = 0; g < games; g++) {
        unsigned int seed = mix_seed(base_seed, (unsigned int)g);
        Sample *out = buf + (size_t)g * MC_MAX_MOVES;
        Board board;
        board_init(&board, &seed);

        int n = 0;
        for (int moves = 0; moves < MC_MAX_MOVES; moves++) {
            double margin;
            Direction dir = mc_choose_move_margin(&board, rollouts, &seed, &margin);
            Board next = board;
            if (!board_move(&next, dir)) break;  // game over

            // A jogada sempre é feita, mas só vira exemplo quando o MC está confiante.
            if (margin >= MC_MIN_MARGIN) {
                for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
                    out[n].cells[i] = (unsigned char)board.grid[i / BOARD_SIZE][i % BOARD_SIZE];
                out[n].move = (unsigned char)dir;
                n++;
            }

            board = next;
            board_spawn_tile(&board, &seed);
        }
        counts[g] = n;
        scores[g] = board.score;
    }

    // Seleciona as `keep` partidas de maior score (empate: menor índice, p/ ser determinístico).
    int *order = malloc((size_t)games * sizeof(int));
    for (int g = 0; g < games; g++) order[g] = g;
    for (int i = 0; i < keep; i++) {
        int m = i;
        for (int j = i + 1; j < games; j++)
            if (scores[order[j]] > scores[order[m]]) m = j;
        int t = order[i]; order[i] = order[m]; order[m] = t;
    }

    double sum = 0.0;
    for (int g = 0; g < games; g++) sum += scores[g];

    int total = 0;
    for (int i = 0; i < keep; i++) total += counts[order[i]];

    ds->samples = malloc((size_t)(total > 0 ? total : 1) * sizeof(Sample));
    int pos = 0;
    for (int i = 0; i < keep; i++) {
        int g = order[i];
        memcpy(ds->samples + pos, buf + (size_t)g * MC_MAX_MOVES, (size_t)counts[g] * sizeof(Sample));
        pos += counts[g];
    }
    ds->count = total;
    ds->games = games;
    ds->avg_score = games ? sum / games : 0.0;
    ds->best_score = keep > 0 ? scores[order[0]] : 0;

    free(order);
    free(buf); free(counts); free(scores);
}

void dataset_free(Dataset *ds) {
    free(ds->samples);
    ds->samples = NULL;
    ds->count = 0;
}
