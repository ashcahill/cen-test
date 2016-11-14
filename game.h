#ifndef GAME_H_
#define GAME_H_

#include <stddef.h>	/* size_t */
#include <math.h>	/* round() */
#include <time.h>	/* clock_gettime() */
#include <assert.h>	/* assert() */

#include "limits.h"
#include "tile.h"
#include "board.h"
#include "rngs/mt19937-64.h" /* Mersenne Twister PRNG. Try PCG if too slow */

#define TILE_COUNT 72

struct game {
	int graphs[100]; /* Placeholder for struct graph */
	struct board board;
	struct tile tile_deck[TILE_COUNT];
	size_t graphs_used;
	size_t tiles_used;
	int scores[2];
};

void make_game(struct game *g);
void make_game_with_deck(struct game *g, struct tile *deck);
int play_move(struct game *g, struct move m, int player);
int more_tiles(struct game *g);
struct tile deal_tile(struct game *g);

#endif
