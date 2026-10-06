#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE 4

typedef enum { DIR_UP = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_RIGHT = 3 } Direction;

typedef struct {
    int grid[BOARD_SIZE][BOARD_SIZE];  // guarda o EXPOENTE (0 = vazio, 1 = 2, 2 = 4, 3 = 8...); o valor é 1 << exp
    int score;
    int game_over;
} Board;

// Zera o tabuleiro e spawna as 2 tiles iniciais.
// seed: ponteiro pro estado do RNG (thread-safe, vai ser usado com rand_r)
void board_init(Board *b, unsigned int *seed);

// Spawna uma tile (90% = 2, 10% = 4) numa posição vazia aleatória.
// Retorna 1 se spawnou, 0 se não havia espaço.
int board_spawn_tile(Board *b, unsigned int *seed);

// Aplica o movimento. Retorna 1 se o grid mudou, 0 se o movimento foi inválido.
// Atenção: sem merge duplo na mesma jogada ([2,2,4] <- esquerda = [4,4,0], não [8,0,0]).
int board_move(Board *b, Direction dir);

// Game over = nenhum dos 4 movimentos muda o grid.
int board_is_game_over(const Board *b);

// Debug: imprime o tabuleiro no terminal.
void board_print(const Board *b);

#endif
