#ifndef GENETIC_H
#define GENETIC_H

#include "network.h"
#include "mc.h"

#define POP_SIZE             64   // múltiplo de 1/2/4/8/16 threads: ajuda a balancear a carga
#define GAMES_PER_INDIVIDUAL 5    // média de N partidas reduz o ruído do RNG
#define MAX_MOVES_PER_GAME   5000 // trava de segurança contra loop infinito
#define ELITE_COUNT          4    // os N melhores passam direto pra próxima geração
#define MUTATION_RATE        0.05 // chance de cada peso sofrer mutação
#define MUTATION_STRENGTH    0.3  // desvio da perturbação
#define IMITATION_SAMPLES    1024 // exemplos sorteados do dataset Monte Carlo a cada geração de pré-treino

typedef struct {
    Network net;
    double  fitness;
} Individual;

void population_init(Individual pop[POP_SIZE], unsigned int base_seed);

// >>> A FUNÇÃO PARALELIZADA <<<
// gen_seed deve variar por geração; a seed de cada indivíduo/partida deriva dele
// (senão: ou todas as threads sorteiam igual, ou o resultado não é reproduzível).
void population_evaluate(Individual pop[POP_SIZE], unsigned int gen_seed);

// Fitness por imitação (pré-treino): fração de jogadas do dataset Monte Carlo que a rede
// reproduz, em [0, 1]. Usa IMITATION_SAMPLES exemplos sorteados por gen_seed (os mesmos
// para todos os indivíduos, então a comparação é justa). Paralela como population_evaluate.
void population_evaluate_imitation(Individual pop[POP_SIZE], const Dataset *ds, unsigned int gen_seed);

// Seleção + cruzamento + mutação. Lê pop (já avaliada), escreve next.
void population_evolve(const Individual pop[POP_SIZE], Individual next[POP_SIZE], unsigned int gen_seed);

// Joga UMA partida com a rede (seed própria) e devolve o tabuleiro final em *final.
// Usado para acompanhar o melhor indivíduo a cada geração (similaridade com o gabarito).
double play_report(const Network *net, unsigned int seed, Board *final);

// Auxiliar: índice do melhor indivíduo (maior fitness).
int population_best(const Individual pop[POP_SIZE]);

#endif
