CFLAGS=-std=c99 -g -march=native -flto -Wall -Wextra -pedantic -O2 -g
all: game board server client

server: server.c game.o rng.o tile.o board.o move.o slot.o serialization.o
	$(CC) $(CFLAGS) -o server server.c game.o rng.o tile.o board.o move.o \
		slot.o serialization.o -pthread -lm

client: client.c game.o rng.o tile.o board.o slot.o serialization.o
	$(CC) $(CFLAGS) -o client client.c game.o rng.o tile.o board.o move.o \
		slot.o serialization.o -lm

game: game.c rng.o tile.o board.o slot.o
	$(CC) $(CFLAGS) -DTEST -o test_game game.c rng.o tile.o board.o slot.o -lm

board: board.c board.h tile.o slot.o move.o
	$(CC) $(CFLAGS) -DTEST -o test_board board.c tile.o slot.o move.o -lm

game.o: game.c game.h
	$(CC) $(CFLAGS) -c -o game.o game.c

serialization.o: serialization.c serialization.h
	$(CC) $(CFLAGS) -c -o serialization.o serialization.c

board.o: board.c board.h
	$(CC) $(CFLAGS) -c -o board.o board.c

rng.o: rngs/mt19937-64.c rngs/mt19937-64.h
	$(CC) $(CFLAGS) -c -o rng.o rngs/mt19937-64.c

move.o: move.c move.h
	$(CC) ${CFLAGS} -c -o move.o move.c

tile.o: tile.c tile.h
	$(CC) ${CFLAGS} -c -o tile.o tile.c

slot.o: slot.c slot.h
	$(CC) ${CFLAGS} -c -o slot.o slot.c
