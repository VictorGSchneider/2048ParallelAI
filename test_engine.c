// Testes automáticos do motor do jogo (make test).
#include <stdio.h>
#include <string.h>
#include "game.h"

static int failures = 0;

#define CHECK(cond) do { if (!(cond)) { printf("FALHOU (linha %d): %s\n", __LINE__, #cond); failures++; } } while (0)

// Monta um board a partir de VALORES (2,4,8...), 0 = vazio.
static Board make(const int v[BOARD_SIZE][BOARD_SIZE]) {
    Board b;
    memset(&b, 0, sizeof(b));
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++) {
            int e = 0;
            for (int x = v[r][c]; x > 1; x >>= 1) e++;
            b.grid[r][c] = e;
        }
    return b;
}

static int row_is(const Board *b, int r, int a, int b1, int c, int d) {
    int want[4] = {a, b1, c, d};
    for (int i = 0; i < 4; i++) {
        int got = b->grid[r][i] ? 1 << b->grid[r][i] : 0;
        if (got != want[i]) return 0;
    }
    return 1;
}

int main(void) {
    // Sem merge duplo: [2,2,4,0] <- esquerda = [4,4,0,0]
    {
        int v[4][4] = {{2,2,4,0},{0},{0},{0}};
        Board b = make(v);
        CHECK(board_move(&b, DIR_LEFT) == 1);
        CHECK(row_is(&b, 0, 4, 4, 0, 0));
        CHECK(b.score == 4);
    }
    // [2,2,2,2] <- esquerda = [4,4,0,0]; direita = [0,0,4,4]
    {
        int v[4][4] = {{2,2,2,2},{0},{0},{0}};
        Board b = make(v);
        board_move(&b, DIR_LEFT);
        CHECK(row_is(&b, 0, 4, 4, 0, 0));
        CHECK(b.score == 8);
        Board c = make(v);
        board_move(&c, DIR_RIGHT);
        CHECK(row_is(&c, 0, 0, 0, 4, 4));
    }
    // [2,0,0,2] <- direita = [0,0,0,4]
    {
        int v[4][4] = {{2,0,0,2},{0},{0},{0}};
        Board b = make(v);
        board_move(&b, DIR_RIGHT);
        CHECK(row_is(&b, 0, 0, 0, 0, 4));
    }
    // Vertical: coluna 0 = [2,2,4,8]; cima -> [4,4,8,0]; baixo -> [0,4,4,8]
    {
        int v[4][4] = {{2},{2},{4},{8}};
        Board b = make(v);
        board_move(&b, DIR_UP);
        CHECK(b.grid[0][0] == 2 && b.grid[1][0] == 2 && b.grid[2][0] == 3 && b.grid[3][0] == 0);
        Board c = make(v);
        board_move(&c, DIR_DOWN);
        CHECK(c.grid[0][0] == 0 && c.grid[1][0] == 2 && c.grid[2][0] == 2 && c.grid[3][0] == 3);
    }
    // Movimento inválido não muda nada e retorna 0
    {
        int v[4][4] = {{2,4,0,0},{0},{0},{0}};
        Board b = make(v);
        Board before = b;
        CHECK(board_move(&b, DIR_LEFT) == 0);
        CHECK(memcmp(&b, &before, sizeof(b)) == 0);
    }
    // Game over: tabuleiro cheio em xadrez; e um com merge possível não é game over
    {
        int dead[4][4] = {{2,4,2,4},{4,2,4,2},{2,4,2,4},{4,2,4,2}};
        Board b = make(dead);
        CHECK(board_is_game_over(&b) == 1);
        CHECK(board_spawn_tile(&b, &(unsigned int){1}) == 0);
        int alive[4][4] = {{2,2,2,4},{4,2,4,2},{2,4,2,4},{4,2,4,2}};
        Board c = make(alive);
        CHECK(board_is_game_over(&c) == 0);
    }
    // Init: 2 tiles, valores 2 ou 4; spawn só em célula vazia
    {
        unsigned int seed = 7;
        Board b;
        board_init(&b, &seed);
        int n = 0;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++) { if (b.grid[r][c]) n++; CHECK(b.grid[r][c] <= 2); }
        CHECK(n == 2 && b.score == 0 && b.game_over == 0);
        for (int i = 0; i < 14; i++) CHECK(board_spawn_tile(&b, &seed) == 1);
        CHECK(board_spawn_tile(&b, &seed) == 0);
    }

    if (failures) { printf("%d teste(s) falharam\n", failures); return 1; }
    printf("todos os testes passaram\n");
    return 0;
}
