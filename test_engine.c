// Testes automáticos do motor do jogo (make test).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include <math.h>
#include "mc.h"
#include "genetic.h"

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

    // Monte Carlo: só uma jogada válida -> tem que escolhê-la; game over -> board_move falha
    {
        int v[4][4] = {{2,4,2,4},{4,2,4,2},{2,4,2,4},{4,2,4,0}};
        Board b = make(v);
        unsigned int seed = 3;
        Direction d = mc_choose_move(&b, 5, &seed);
        CHECK(d == DIR_DOWN || d == DIR_RIGHT);  // as únicas que mexem alguma tile
        CHECK(board_move(&b, d) == 1);
        int dead[4][4] = {{2,4,2,4},{4,2,4,2},{2,4,2,4},{4,2,4,2}};
        Board g = make(dead);
        CHECK(board_move(&g, mc_choose_move(&g, 5, &seed)) == 0);
    }
    // Premissa do bônus cobra no MC: a métrica ordena corretamente as jogadas que ele compara
    {
        // Linha 0 = [2,2,4,0]; esquerda dá [4,4,0,0] e direita dá [0,0,4,4]; só a esquerda
        // mantém os blocos no começo do caminho da cobra, então precisa ter nota maior.
        int v[4][4] = {{2,2,4,0},{0},{0},{0}};
        Board b = make(v);
        Board l = b, r = b;
        board_move(&l, DIR_LEFT); board_move(&r, DIR_RIGHT);
        CHECK(board_gabarito_similarity(&l) > board_gabarito_similarity(&r));
    }
    // Monte Carlo joga bem melhor que movimentos aleatórios, e o dataset é coerente
    {
        unsigned int seed = 11;
        double rnd = 0;
        for (int i = 0; i < 20; i++) { Board b; board_init(&b, &seed); rnd += mc_random_playout(b, &seed); }
        rnd /= 20;

        Dataset ds;
        dataset_generate(&ds, 3, 2, 8, 99);
        CHECK(ds.games == 3 && ds.count > 0);
        CHECK(ds.avg_score > rnd);  // mesmo com o bônus cobra, o MC joga melhor que o aleatório
        for (int i = 0; i < ds.count; i++) CHECK(ds.samples[i].move < 4);

        // Teste de cada iteração (partida) do Monte Carlo e de cada jogada gravada
        const char *why = "";
        CHECK(dataset_verify(&ds, &why));
        if (why[0]) printf("  dataset_verify: %s\n", why);
        int kept = 0;
        for (int g = 0; g < ds.games; g++) {
            CHECK(ds.stats[g].snake_final >= 0.0 && ds.stats[g].snake_final <= 1.0);
            CHECK(ds.stats[g].snake_avg >= 0.0 && ds.stats[g].snake_avg <= 1.0);
            CHECK(ds.stats[g].moves >= 1);
            kept += ds.stats[g].kept;
        }
        CHECK(kept == 2);  // keep = 2 no dataset_generate acima

        // O verificador precisa pegar uma jogada inválida adulterada
        Dataset bad = ds;
        Sample *copy = malloc((size_t)ds.count * sizeof(Sample));
        memcpy(copy, ds.samples, (size_t)ds.count * sizeof(Sample));
        memset(copy[0].cells, 0, sizeof(copy[0].cells));  // tabuleiro vazio: nenhuma jogada é válida
        bad.samples = copy;
        CHECK(!dataset_verify(&bad, NULL));
        free(copy);
        dataset_free(&ds);
    }

    // score_log2: 0 -> 0, monotônica, e 2^k - 1 -> k
    {
        CHECK(score_log2(0) == 0.0);
        CHECK(fabs(score_log2(1023) - 10.0) < 1e-9);
        CHECK(score_log2(100) < score_log2(200) && score_log2(200) < score_log2(30000));
        CHECK(score_log2(30000) < 15.0);  // 30.000 pontos viram ~14,9
    }
    // Gabarito (tabuleiro "cobra" da referência): confere célula a célula com os valores esperados
    {
        int ref[4][4] = {{65536, 32768, 16384, 8192},
                         {  512,  1024,  2048, 4096},
                         {  256,   128,    64,   32},
                         {    2,     4,     8,   16}};
        Board want = make(ref), g;
        board_load_gabarito(&g);
        CHECK(memcmp(want.grid, g.grid, sizeof(g.grid)) == 0);
        CHECK(board_is_game_over(&g) == 1);   // cheio e sem merges possíveis
        CHECK(board_gabarito_similarity(&g) == 1.0);

        // Espelhar a linha 1 (quebra a cobra) piora a nota
        Board bad = g;
        for (int c = 0; c < 4; c++) bad.grid[1][c] = g.grid[1][3 - c];
        CHECK(board_gabarito_similarity(&bad) < 1.0);

        // Trocar o maior bloco de canto: perde posições
        Board swapped = g;
        swapped.grid[0][0] = g.grid[3][3]; swapped.grid[3][3] = g.grid[0][0];
        CHECK(board_gabarito_similarity(&swapped) < board_gabarito_similarity(&g));

        // Poucos blocos: certos se estão no começo do caminho da cobra, errados se no fim
        int head[4][4] = {{8,4,0,0},{0},{0},{0}};
        int tail[4][4] = {{0},{0},{0},{0,0,4,8}};
        Board h = make(head), tl = make(tail);
        CHECK(board_gabarito_similarity(&h) == 1.0);
        CHECK(board_gabarito_similarity(&tl) < 1.0);

        // A nota fica em [0, 1] em todos os estados de uma partida
        unsigned int seed = 5;
        Board b;
        board_init(&b, &seed);
        for (int i = 0; i < 300 && !board_is_game_over(&b); i++) {
            board_move(&b, (Direction)(rand_r(&seed) % 4));
            board_spawn_tile(&b, &seed);
            double s = board_gabarito_similarity(&b);
            CHECK(s >= 0.0 && s <= 1.0);
        }
    }

    // Fitness = score * (1 + SNAKE_WEIGHT * cobra); cobra em [0, 1]; avaliação reprodutível
    {
        static Individual pop[POP_SIZE], again[POP_SIZE];
        population_init(pop, 77);
        memcpy(again, pop, sizeof(pop));
        population_evaluate(pop, 123);
        population_evaluate(again, 123);
        for (int i = 0; i < POP_SIZE; i++) {
            CHECK(pop[i].snake >= 0.0 && pop[i].snake <= 1.0);
            double base = FITNESS_LOG2 ? pop[i].lscore : pop[i].score;
            CHECK(fabs(pop[i].fitness - base * (1.0 + SNAKE_WEIGHT * pop[i].snake)) < 1e-9);
            CHECK(pop[i].fitness >= base);
            CHECK(pop[i].lscore <= log2(1.0 + pop[i].score) + 1e-9);  // Jensen: média dos logs <= log da média
            CHECK(pop[i].fitness == again[i].fitness);
        }
    }

    if (failures) { printf("%d teste(s) falharam\n", failures); return 1; }
    printf("todos os testes passaram\n");
    return 0;
}
