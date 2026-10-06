CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -fopenmp
LDFLAGS = -fopenmp -lm

SRC = main.c game.c mc.c network.c genetic.c
OBJ = $(SRC:.c=.o)

2048-ga: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# teste isolado do motor do jogo
test_game: game.c game.h
	$(CC) -O0 -g -Wall -DTEST_GAME game.c -o test_game -lm

# testes automáticos do motor (regras de merge, game over, spawn)
test: test_engine
	./test_engine

test_engine: test_engine.c game.c game.h mc.c mc.h network.c network.h genetic.c genetic.h rng.h
	$(CC) -O0 -g -Wall -Wextra -fopenmp test_engine.c game.c mc.c network.c genetic.c -o test_engine -lm

clean:
	rm -f $(OBJ) 2048-ga test_game test_engine speedup.csv

.PHONY: clean test
