#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

void board_init(Board *b, unsigned int *seed) {
    memset(b->grid, 0, sizeof(b->grid));
    b->score = 0;
    b->game_over = 0;
    board_spawn_tile(b, seed);
    board_spawn_tile(b, seed);
}

int board_spawn_tile(Board *b, unsigned int *seed) {
    int empty[BOARD_SIZE * BOARD_SIZE];
    int n = 0;
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (b->grid[r][c] == 0) empty[n++] = r * BOARD_SIZE + c;

    if (n == 0) return 0;

    int cell = empty[rand_r(seed) % n];
    b->grid[cell / BOARD_SIZE][cell % BOARD_SIZE] = (rand_r(seed) % 10 == 0) ? 2 : 1;

    // Só um tabuleiro cheio pode estar travado, então só aí vale o custo de checar.
    if (n == 1) b->game_over = board_is_game_over(b);
    return 1;
}

// Move/mescla uma linha de 4 células para o índice 0. Retorna a pontuação ganha.
// Cada tile participa de no máximo um merge: [1,1,2,0] -> [2,2,0,0].
static int slide_line(int line[BOARD_SIZE]) {
    int out[BOARD_SIZE] = {0};
    int pos = 0, score = 0, can_merge = 0;

    for (int i = 0; i < BOARD_SIZE; i++) {
        int v = line[i];
        if (v == 0) continue;
        if (can_merge && out[pos - 1] == v) {
            out[pos - 1] = v + 1;
            score += 1 << (v + 1);
            can_merge = 0;  // a tile resultante não pode mesclar de novo nesta jogada
        } else {
            out[pos++] = v;
            can_merge = 1;
        }
    }
    memcpy(line, out, sizeof(out));
    return score;
}

// Coordenadas da k-ésima célula da linha/coluna `idx`, na ordem "do lado
// pra onde o movimento empurra" até o lado oposto. Reduz as 4 direções a "esquerda".
static void line_cell(Direction dir, int idx, int k, int *r, int *c) {
    switch (dir) {
        case DIR_LEFT:  *r = idx;                  *c = k;                  break;
        case DIR_RIGHT: *r = idx;                  *c = BOARD_SIZE - 1 - k; break;
        case DIR_UP:    *r = k;                    *c = idx;                break;
        default:        *r = BOARD_SIZE - 1 - k;   *c = idx;                break;  // DIR_DOWN
    }
}

int board_move(Board *b, Direction dir) {
    int changed = 0;

    for (int idx = 0; idx < BOARD_SIZE; idx++) {
        int line[BOARD_SIZE], orig[BOARD_SIZE];
        for (int k = 0; k < BOARD_SIZE; k++) {
            int r, c;
            line_cell(dir, idx, k, &r, &c);
            orig[k] = line[k] = b->grid[r][c];
        }

        int gained = slide_line(line);

        if (memcmp(line, orig, sizeof(line)) != 0) {
            changed = 1;
            for (int k = 0; k < BOARD_SIZE; k++) {
                int r, c;
                line_cell(dir, idx, k, &r, &c);
                b->grid[r][c] = line[k];
            }
        }
        b->score += gained;
    }
    return changed;
}

int board_is_game_over(const Board *b) {
    for (int d = 0; d < 4; d++) {
        Board copy = *b;
        if (board_move(&copy, (Direction)d)) return 0;
    }
    return 1;
}

void board_print(const Board *b) {
    printf("score: %d\n", b->score);
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (b->grid[r][c]) printf("%5d", 1 << b->grid[r][c]);
            else               printf("    .");
        }
        printf("\n");
    }
}

// Teste isolado do motor, jogável no terminal (make test_game && ./test_game):
#ifdef TEST_GAME
int main(void) {
    unsigned int seed = 42;
    Board b;
    board_init(&b, &seed);

    char line[64];
    for (;;) {
        board_print(&b);
        if (board_is_game_over(&b)) { printf("Game over!\n"); break; }

        printf("w/a/s/d (q sai): ");
        if (!fgets(line, sizeof(line), stdin) || line[0] == 'q') break;

        Direction dir;
        switch (line[0]) {
            case 'w': dir = DIR_UP;    break;
            case 's': dir = DIR_DOWN;  break;
            case 'a': dir = DIR_LEFT;  break;
            case 'd': dir = DIR_RIGHT; break;
            default: continue;
        }
        if (board_move(&b, dir)) board_spawn_tile(&b, &seed);
        else printf("movimento inválido\n");
    }
    return 0;
}
#endif
