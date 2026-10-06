#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

void board_init(Board *b, unsigned int *seed) {
    // TODO: memset no grid, score = 0, game_over = 0, spawnar 2 tiles
    (void)b; (void)seed;
}

int board_spawn_tile(Board *b, unsigned int *seed) {
    // TODO: listar células vazias, sortear uma com rand_r(seed), colocar 2 (90%) ou 4 (10%)
    (void)b; (void)seed;
    return 0;
}

int board_move(Board *b, Direction dir) {
    // TODO: o coração do motor.
    // Dica: implemente SÓ "mover pra esquerda" para uma linha, e reduza as outras
    // 3 direções a isso (rotacionar/espelhar o grid, mover, desfazer).
    // Lembrar: 1) compactar, 2) mergear pares iguais uma única vez, 3) compactar de novo.
    // Somar o valor do merge em b->score.
    (void)b; (void)dir;
    return 0;
}

int board_is_game_over(const Board *b) {
    // TODO: copiar o board, tentar os 4 movimentos na cópia; se nenhum mudar nada, acabou.
    (void)b;
    return 0;
}

void board_print(const Board *b) {
    // TODO: opcional, mas você vai agradecer na hora de debugar
    (void)b;
}

// Descomenta pra testar o motor isolado (gcc -DTEST_GAME game.c -o test_game):
#ifdef TEST_GAME
int main(void) {
    unsigned int seed = 42;
    Board b;
    board_init(&b, &seed);
    // TODO: loop lendo w/a/s/d do terminal, chamando board_move + board_spawn_tile
    return 0;
}
#endif
