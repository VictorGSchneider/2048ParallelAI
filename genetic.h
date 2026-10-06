#ifndef GENETIC_H
#define GENETIC_H

#include "network.h"

#define POP_SIZE             64   // múltiplo de 1/2/4/8/16 threads: ajuda a balancear a carga
#define GAMES_PER_INDIVIDUAL 5    // média de N partidas reduz o ruído do RNG
#define MAX_MOVES_PER_GAME   5000 // trava de segurança contra loop infinito
#define ELITE_COUNT          4    // os N melhores passam direto pra próxima geração
#define MUTATION_RATE        0.05 // chance de cada peso sofrer mutação
#define MUTATION_STRENGTH    0.3  // desvio da perturbação

typedef struct {
    Network net;
    double  fitness;
} Individual;

void population_init(Individual pop[POP_SIZE], unsigned int base_seed);

// >>> A FUNÇÃO PARALELIZADA <<<
// gen_seed deve variar por geração; a seed de cada indivíduo/partida deriva dele
// (senão: ou todas as threads sorteiam igual, ou o resultado não é reproduzível).
void population_evaluate(Individual pop[POP_SIZE], unsigned int gen_seed);

// Seleção + cruzamento + mutação. Lê pop (já avaliada), escreve next.
void population_evolve(const Individual pop[POP_SIZE], Individual next[POP_SIZE], unsigned int gen_seed);

// Auxiliar: índice do melhor indivíduo (maior fitness).
int population_best(const Individual pop[POP_SIZE]);

#endif
