#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"

#define INPUT_SIZE  16   // grid 4x4 achatado
#define HIDDEN_SIZE 16   // TODO: decidir (8? 16? 32?) e justificar na ADR
#define OUTPUT_SIZE 4    // up, down, left, right

typedef struct {
    double w1[INPUT_SIZE][HIDDEN_SIZE];
    double b1[HIDDEN_SIZE];
    double w2[HIDDEN_SIZE][OUTPUT_SIZE];
    double b2[OUTPUT_SIZE];
} Network;

// Pesos aleatórios em [-1, 1]. Usa rand_r(seed) (thread-safe).
void network_init_random(Network *n, unsigned int *seed);

// Forward pass. Preenche out[OUTPUT_SIZE] com as "notas" de cada direção.
void network_forward(const Network *n, const Board *b, double out[OUTPUT_SIZE]);

// Retorna a direção de maior nota que seja um movimento VÁLIDO no tabuleiro.
// (se a melhor direção não muda nada, tenta a segunda melhor, e assim por diante)
Direction network_predict(const Network *n, const Board *b);

#endif
