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

// Gabarito: tabuleiro "cobra" perfeito (referência de organização). Ordem do caminho:
//   65536 32768 16384  8192
//     512  1024  2048  4096
//     256   128    64    32
//       2     4     8    16
void board_load_gabarito(Board *b);

// Similaridade com o gabarito, em [0, 1]. Ordena os blocos do tabuleiro do maior pro menor e
// compara com o caminho da cobra: a k-ésima posição do caminho deveria ter o k-ésimo maior bloco
// (células vazias por último). Conta a fração das 16 posições corretas. Comparar por valor torna
// empates irrelevantes. O próprio gabarito dá 1.0; um tabuleiro vazio também (trivialmente).
double board_gabarito_similarity(const Board *b);

// Debug: imprime o tabuleiro no terminal.
void board_print(const Board *b);

#endif
