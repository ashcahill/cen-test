#include <stdlib.h>     /* NULL, malloc(), free() */
#include <stdint.h>	/* uint32_t */
#include <string.h>     /* memset() */
#include <unistd.h>     /* write() */
#include <pthread.h>	/* pthread */

#include <arpa/inet.h>	/* htons */
#include <sys/socket.h> /* bind(), listen(), setsockopt() */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <signal.h>	/* SIGPIPE */

#include <stdio.h>	/* DEBUG */
#include "limits.h"	/* AXIS, TILE_SZ */
#include "game.h"	/* Server needs to validate moves. */
#include "serialization.h"	/* Serializing */

void signal_handler(int signum)
{
	return;
}

struct sockaddr_in init_sockaddr(int port)
{
	struct sockaddr_in s;
	memset(&s, '0', sizeof(s));
	s.sin_family = AF_INET;
	s.sin_addr.s_addr = htonl(INADDR_ANY);
	s.sin_port = htons(port);
	return s;
}

/* TODO: Send client hosts with sockets so that 3rd parties can't jump in */
#define PLAYER_COUNT 2

static int send_deck(int *players, size_t pcnt, struct tile *deck, size_t dlen)
{
	char buf[TILE_SZ];

	for (size_t i = 0; i < dlen; ++i) {
		buf[0] = deck[i].edges[0];
		buf[1] = deck[i].edges[1];
		buf[2] = deck[i].edges[2];
		buf[3] = deck[i].edges[3];
		buf[4] = deck[i].edges[4];
		buf[5] = deck[i].attribute;
		for (size_t j = 0; j < pcnt; ++j) {
			/* TODO: Error handling. */
			write(players[j], buf, TILE_SZ);
		}
	}
	return 0;
}

enum reason {
	SCORE = 0,
	TIMEOUT = 1,
	INVALID = 2
};

static int tile_eq(struct tile a, struct tile b)
{
	for (size_t i = 0; i < 5; ++i) {
		if (a.edges[i] != b.edges[i]) {
			return 0;
		}
	}
	if (a.attribute != b.attribute) {
		return 0;
	} else {
		return 1;
	}
}

static int game_over(int *players,int w, enum reason r, unsigned char *buf)
{
	buf[0] = 1;
	buf[2] = (uint8_t) r;
	for (size_t i = 0; i < PLAYER_COUNT; ++i) {
		if (i == w) { /* i'th player is the winner */
			buf[1] = 1;
		} else {
			buf[1] = 0;
		}
		write(players[i], buf, sizeof(buf));
	}
	return 0;
}

/* Step through protocol with clients. */
static void protocol(void *args)
{
	int *hostfd = (int *)args;
	struct game *g = malloc(sizeof(*g));
	make_game(g);
	listen(*hostfd, 10);

	int players[PLAYER_COUNT] = {0};
	int player = 0; /* TODO: Randomly pick player to go first. */
	unsigned char buf[BUF_SZ];
	memset(buf, 0, BUF_SZ);
	for (size_t i = 0; i < PLAYER_COUNT; ++i) {
		players[i] = accept(*hostfd, NULL, NULL);

		struct timeval tm;
		memset(&tm, 0, sizeof(tm));
		tm.tv_sec = 5; /* 5 seconds per move timer. */
		setsockopt(players[i], SOL_SOCKET, SO_RCVTIMEO,
				(char *)&tm, sizeof(tm));

		if (i == player) {
			buf[0] = 1;
		} else {
			buf[0] = 0;
		}
		serialize_clock(tm.tv_sec, &buf[1]);
		write(players[i], buf, sizeof(buf));
	}
	printf("Connected both players!\n");

	if (send_deck(players, PLAYER_COUNT, g->tile_deck, TILE_COUNT)) {
		printf("Failed to send deck.\n");
	}

	struct move prev_move;
	while (1) { /* Play game. */
		if (!more_tiles(g)) {
			game_over(players, 0, SCORE, buf); // TODO Scoring
		}
		struct tile t = deal_tile(g);
		buf[0] = 0; /* Assume still playing. */
		serialize_tile(t, &buf[1]);
		serialize_move(prev_move, &buf[1 + TILE_SZ]);
		write(players[player], buf, sizeof(buf));
		if (read(players[player], buf, sizeof(buf))<sizeof(buf)) {
			game_over(players, player ^ 1, TIMEOUT, buf);
			break;
		}
		struct move m = deserialize_move(buf);
		if (!tile_eq(t, m.tile) || play_move(g, m, player)) {
			game_over(players, player ^ 1, INVALID, buf);
			break;
		}
		prev_move = m;
		player ^= 1; /* Switch */
		break;
	}
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		close(players[i]);
	}
	free(g);
	free(hostfd);
        return;
}

/* Returns the port a socket is listen()ing on in NETWORK ORDER */
static int get_socket_port(int socket)
{
	struct sockaddr_in info;
	memset(&info, 0, sizeof(info));
	socklen_t len = sizeof(info);
	getsockname(socket, (struct sockaddr *)&info, &len);
	return info.sin_port; /* Already in network order. */
}

#define LISTEN_PORT 5000 /* Arbitrarily chosen server port. */
int main(void)
{
        struct sockaddr_in serv_addr = init_sockaddr(LISTEN_PORT);

        int listenfd = socket(AF_INET, SOCK_STREAM, 0);
        bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        listen(listenfd, 10);

	int players[PLAYER_COUNT];
	int queued_players = 0;

        while (1) {
		/* TODO Ensure unique clients (can't play with self) */
		players[queued_players++] = accept(listenfd, NULL, NULL);
		if (queued_players < PLAYER_COUNT) { /* Wait for match */
			printf("Waiting for match.\n");
			continue;
		}

		/* Found a match. Create a child thread to host game. */
		/* TODO: Lots of error handling. */
		pthread_t *child = malloc(sizeof(pthread_t));
		if (child == NULL) {
			printf("Failed to allocate memory!\n");
			return 1;
		}
		pthread_detach(*child);			/* OS will free() */
		int *hostfd = malloc(sizeof(int));	/* Thread will free() */
		if (hostfd == NULL) {
			printf("Failed to allocate memory!\n");
			free(child);
			return 1;
		}
		*hostfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in host_addr = init_sockaddr(0); // random port
		bind(*hostfd, (struct sockaddr *)&host_addr, sizeof(host_addr));
		pthread_create(child, NULL, &protocol, hostfd);
		
		/* Tell the clients where to go. */
		uint16_t port = get_socket_port(*hostfd);
		printf("Sending port: %u\n", ntohs(port));
		for (int i = 0; i < PLAYER_COUNT; ++i) {
			write(players[i], &port, sizeof(port));
			close(players[i]); /* No longer need to talk to them. */
		}
		queued_players = 0;
        }
	close(listenfd);

        return 0;
}
