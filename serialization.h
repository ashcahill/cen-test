#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>

#include "tile.h"
#include "move.h"
#include "limits.h"

#define TILE_SZ 7		/* 1 byte per edge + center + attribute */
#define MOVE_SZ TILE_SZ + 3	/* TILE + 1 byte for x, y, and rotation. */
#define BUF_SZ 1 + TILE_SZ + MOVE_SZ /* Game_over? + TILE + MOVE */

void serialize_tile(struct tile t, unsigned char buf[TILE_SZ]);
struct tile deserialize_tile(unsigned char buf[TILE_SZ]);
void serialize_move(struct move m, unsigned char buf[MOVE_SZ]);
struct move deserialize_move(unsigned char buf[MOVE_SZ]);
void serialize_clock(uint64_t clock, unsigned char buf[sizeof(uint64_t)]);
uint64_t deserialize_clock(unsigned char buf[sizeof(uint64_t)]);
#endif
