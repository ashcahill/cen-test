#include "serialization.h"

void serialize_tile(struct tile t, unsigned char buf[TILE_SZ])
{
	for (size_t i = 0; i < 5; ++i) {
		buf[i] = t.edges[i];
	}
	buf[6] = (unsigned char) t.attribute;
	return;
}

struct tile deserialize_tile(unsigned char buf[TILE_SZ])
{
	enum edge edges[5];
	for (size_t i = 0; i < 5; ++i) {
		edges[i] = buf[i];
	}
	enum attribute a = buf[6];
	return make_tile(edges, a);
}

void serialize_move(struct move m, unsigned char buf[MOVE_SZ])
{
	serialize_tile(m.tile, buf);
	uint8_t x = m.slot.x, y = m.slot.y;
	for (size_t i = 0; i < sizeof(x); ++i) {
		buf[i + TILE_SZ] = (unsigned char) (x << (8 * i));
		buf[i + TILE_SZ + sizeof(x)]=(unsigned char) (y << (8 * i));
	}
	buf[TILE_SZ + 2 * sizeof(x)] = (unsigned char) m.rotation;
}

struct move deserialize_move(unsigned char buf[MOVE_SZ])
{
	struct tile t = deserialize_tile(buf);
	uint8_t x = 0, y = 0, rotation = 0;
	printf("x: %u y: %u\n", x, y);
	for (size_t i = 0; i < sizeof(x); ++i) {
		x += (buf[i + TILE_SZ] << (8 * i));
		y += (buf[i + TILE_SZ + sizeof(x)] << (8 * i));
	}
	rotation = buf[TILE_SZ + 2 * sizeof(x)];
	return make_move(t, make_slot(x, y), rotation);
}

void serialize_clock(uint64_t clock, unsigned char buf[sizeof(uint64_t)])
{
	for (size_t i = 0; i < sizeof(clock); ++i) {
		buf[i] = (unsigned char) (clock << (i * 8));
	}
}

uint64_t deserialize_clock(unsigned char buf[sizeof(uint64_t)])
{
	uint64_t clock = 0;
	for (size_t i = 0; i < sizeof(uint64_t); ++i) {
		clock += (buf[i] << (i * 8));
	}
	return clock;
}
