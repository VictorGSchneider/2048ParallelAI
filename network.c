#include <math.h>
#include <stdlib.h>
#include "network.h"

void network_init_random(Network *n, unsigned int *seed) {
    // TODO: preencher w1, b1, w2, b2 com doubles em [-1, 1] usando rand_r(seed)
    (void)n; (void)seed;
}

void network_forward(const Network *n, const Board *b, double out[OUTPUT_SIZE]) {
    // TODO:
    // 1) montar input[16] a partir do grid. Normalizar: pense em log2(valor) em vez do valor bruto. Por quê?
    // 2) hidden = ativação(input * w1 + b1)   (tanh ou ReLU)
    // 3) out = hidden * w2 + b2               (sem ativação, só o argmax importa)
    (void)n; (void)b;
    for (int i = 0; i < OUTPUT_SIZE; i++) out[i] = 0.0;
}

Direction network_predict(const Network *n, const Board *b) {
    // TODO: chamar network_forward, ordenar as 4 notas, e retornar a melhor
    // direção que seja válida (testar numa CÓPIA do board, sem alterar o original).
    (void)n; (void)b;
    return DIR_UP;
}
