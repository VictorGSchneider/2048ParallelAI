#include <math.h>
#include <stdlib.h>
#include "network.h"

// Expoente máximo esperado num 4x4 (2^17 = 131072): usado só para escalar a entrada em ~[0, 1].
#define MAX_EXPONENT 17.0

static double rand_weight(unsigned int *seed) {
    return 2.0 * ((double)rand_r(seed) / (double)RAND_MAX) - 1.0;
}

void network_init_random(Network *n, unsigned int *seed) {
    for (int i = 0; i < INPUT_SIZE; i++)
        for (int j = 0; j < HIDDEN_SIZE; j++) n->w1[i][j] = rand_weight(seed);
    for (int j = 0; j < HIDDEN_SIZE; j++) n->b1[j] = rand_weight(seed);
    for (int j = 0; j < HIDDEN_SIZE; j++)
        for (int k = 0; k < OUTPUT_SIZE; k++) n->w2[j][k] = rand_weight(seed);
    for (int k = 0; k < OUTPUT_SIZE; k++) n->b2[k] = rand_weight(seed);
}

void network_forward(const Network *n, const Board *b, double out[OUTPUT_SIZE]) {
    // O grid já guarda log2(valor), que é a escala natural: o valor bruto cresce
    // exponencialmente (2..65536) e uma tile grande esmagaria todas as outras na soma ponderada.
    double input[INPUT_SIZE];
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            input[r * BOARD_SIZE + c] = b->grid[r][c] / MAX_EXPONENT;

    double hidden[HIDDEN_SIZE];
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        double sum = n->b1[j];
        for (int i = 0; i < INPUT_SIZE; i++) sum += input[i] * n->w1[i][j];
        hidden[j] = tanh(sum);
    }

    for (int k = 0; k < OUTPUT_SIZE; k++) {
        double sum = n->b2[k];
        for (int j = 0; j < HIDDEN_SIZE; j++) sum += hidden[j] * n->w2[j][k];
        out[k] = sum;
    }
}

Direction network_predict(const Network *n, const Board *b) {
    double score[OUTPUT_SIZE];
    network_forward(n, b, score);

    // Tenta as direções da maior nota pra menor; a primeira válida vence.
    int used[OUTPUT_SIZE] = {0};
    for (int rank = 0; rank < OUTPUT_SIZE; rank++) {
        int best = -1;
        for (int d = 0; d < OUTPUT_SIZE; d++)
            if (!used[d] && (best < 0 || score[d] > score[best])) best = d;
        used[best] = 1;

        Board copy = *b;
        if (board_move(&copy, (Direction)best)) return (Direction)best;
    }
    return DIR_UP;  // nenhum movimento válido (game over); o chamador detecta via board_move == 0
}
