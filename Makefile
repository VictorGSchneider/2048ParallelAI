CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -fopenmp
LDFLAGS = -fopenmp -lm

SRC = main.c game.c network.c genetic.c
OBJ = $(SRC:.c=.o)

2048-ga: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# teste isolado do motor do jogo
test_game: game.c game.h
	$(CC) -O0 -g -Wall -DTEST_GAME game.c -o test_game

# testes automáticos do motor (regras de merge, game over, spawn)
test: test_engine
	./test_engine

test_engine: test_engine.c game.c game.h
	$(CC) -O0 -g -Wall -Wextra test_engine.c game.c -o test_engine

clean:
	rm -f $(OBJ) 2048-ga test_game test_engine speedup.csv

.PHONY: clean test
