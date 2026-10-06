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
        // Organização logo após o movimento (antes do spawn, que só adicionaria ruído).
        double snake = board_gabarito_similarity(&first);
        board_spawn_tile(&first, seed);

        double total = 0.0;
        for (int r = 0; r < rollouts; r++) {
            int s = mc_random_playout(first, seed);
            total += MC_LOG2_SCORE ? score_log2(s) : (double)s;  // média em log2: linear e robusta a partidas sortudas
        }

        double mean = (total / rollouts) * (1.0 + MC_SNAKE_WEIGHT * snake);
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
    GameStat *stats = calloc((size_t)games, sizeof(GameStat));

    // Cada partida escreve só na sua fatia de `buf` e em counts[g]/scores[g]/stats[g]: sem corrida.
    // dynamic: a duração das partidas varia muito (o Monte Carlo joga partidas longas).
    #pragma omp parallel for schedule(dynamic)
    for (int g = 0; g < games; g++) {
        unsigned int seed = mix_seed(base_seed, (unsigned int)g);
        Sample *out = buf + (size_t)g * MC_MAX_MOVES;
        Board board;
        board_init(&board, &seed);

        int n = 0, made = 0;
        double snake_sum = 0.0;
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
            made++;
            snake_sum += board_gabarito_similarity(&board);
        }
        counts[g] = n;
        scores[g] = board.score;

        // Teste desta iteração contra o gabarito "cobra".
        int max_exp = 0;
        for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
            if (board.grid[i / BOARD_SIZE][i % BOARD_SIZE] > max_exp) max_exp = board.grid[i / BOARD_SIZE][i % BOARD_SIZE];
        stats[g].score = board.score;
        stats[g].moves = made;
        stats[g].max_tile = 1 << max_exp;
        stats[g].snake_final = board_gabarito_similarity(&board);
        stats[g].snake_avg = made ? snake_sum / made : 0.0;
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
    for (int i = 0; i < keep; i++) stats[order[i]].kept = 1;

    int total = 0;
    for (int i = 0; i < keep; i++) total += counts[order[i]];

    ds->samples = malloc((size_t)(total > 0 ? total : 1) * sizeof(Sample));
    int pos = 0;
    for (int i = 0; i < keep; i++) {
        int g = order[i];
        memcpy(ds->samples + pos, buf + (size_t)g * MC_MAX_MOVES, (size_t)counts[g] * sizeof(Sample));
        pos += counts[g];
    }
    ds->stats = stats;
    ds->count = total;
    ds->games = games;
    ds->avg_score = games ? sum / games : 0.0;
    ds->best_score = keep > 0 ? scores[order[0]] : 0;

    free(order);
    free(buf); free(counts); free(scores);
}

int dataset_verify(const Dataset *ds, const char **why) {
    #define FAIL(msg) do { if (why) *why = (msg); return 0; } while (0)
    if (ds->count > 0 && !ds->samples) FAIL("samples nulo");
    if (ds->games > 0 && !ds->stats) FAIL("stats nulo");

    for (int i = 0; i < ds->count; i++) {
        const Sample *s = &ds->samples[i];
        if (s->move >= 4) FAIL("direcao fora de 0..3");
        Board b;
        memset(&b, 0, sizeof(b));
        for (int k = 0; k < BOARD_SIZE * BOARD_SIZE; k++) {
            if (s->cells[k] > 17) FAIL("expoente fora do intervalo");
            b.grid[k / BOARD_SIZE][k % BOARD_SIZE] = s->cells[k];
        }
        if (!board_move(&b, (Direction)s->move)) FAIL("jogada gravada e invalida no tabuleiro gravado");
    }

    int kept = 0;
    for (int g = 0; g < ds->games; g++) {
        const GameStat *st = &ds->stats[g];
        if (st->moves < 1 || st->moves > MC_MAX_MOVES) FAIL("numero de jogadas fora do intervalo");
        if (st->score < 0 || st->max_tile < 2) FAIL("score/maior bloco invalido");
        if (st->snake_final < 0.0 || st->snake_final > 1.0 || st->snake_avg < 0.0 || st->snake_avg > 1.0)
            FAIL("similaridade cobra fora de [0, 1]");
        kept += st->kept ? 1 : 0;
    }
    if (ds->games > 0 && kept < 1) FAIL("nenhuma partida marcada como mantida");
    #undef FAIL
    return 1;
}

void dataset_free(Dataset *ds) {
    free(ds->samples);
    free(ds->stats);
    ds->stats = NULL;
    ds->samples = NULL;
    ds->count = 0;
}
