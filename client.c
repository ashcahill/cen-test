#include <errno.h>	/* errno */
#include <stdio.h>	/* printf() */
#include <stdint.h>	/* uint32_t */
#include <string.h>	/* memset(), strerror() */
#include <unistd.h>	/* read(), write() */

#include <arpa/inet.h>  /* inet_pton() */

#include <sys/socket.h> /* socket() */
#include <sys/uio.h>	/* read(), write() */
#include <sys/types.h>	/* read(), write() */

#include "limits.h"
#include "game.h"
#include "serialization.h"

static struct sockaddr_in make_sockaddr_in_port(int port)
{
	struct sockaddr_in s;
	memset(&s, '0', sizeof(s));
	s.sin_family = AF_INET;
	s.sin_port = port;
	return s;
}

static int retry_connection(int sock, struct sockaddr *a, size_t asize, int cnt)
{
	for (int i = 0; i < cnt; ++i) {
		if (!connect(sock, a, asize)) {
			return 0;
		}
		switch (errno) {
		case ECONNREFUSED:
		case EHOSTUNREACH:
		case ENETDOWN:
		case ENETUNREACH:
		case ETIMEDOUT:
		case ECONNRESET:
			break;	/* retry */
		default:	/* can't fix, so bail out */
			return 1;
		}
		sleep(1);
	}
	return 1;
}

static int connect_retry(char *host, int port)
{
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket: %s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in s = make_sockaddr_in_port(port);
	if (!inet_pton(AF_INET, host, &s.sin_addr)) {
		printf("Bad address.\n");
		return -1;
	} 

	if (retry_connection(sockfd, (struct sockaddr *)&s, sizeof(s), 10)) {
		printf("Connection Failure: %s\n", strerror(errno));
		return -1;
	}

	return sockfd;
}

static int get_deck(int sockfd, struct tile *deck, size_t clen, size_t dlen)
{
	unsigned char buf[clen];
	for (size_t i = 0; i < dlen; ++i) {
		read(sockfd, buf, clen);
		enum edge edges[5];
		enum attribute a;
		edges[0] = buf[0];
		edges[1] = buf[1];
		edges[2] = buf[2];
		edges[3] = buf[3];
		edges[4] = buf[4];
		a = buf[5];
		deck[i] = make_tile(edges, a);
	}
	return 0;
}

#define REMOTE_HOST "127.0.0.1" /* TODO: Get a command line variable. */
#define REMOTE_PORT 5000 /* TODO: Factor into command line variable. */

//int main(int argc, char *argv[])
int main(void)
{
	int sockfd;
	if ((sockfd = connect_retry(REMOTE_HOST, htons(REMOTE_PORT))) < 0) {
		printf("Error: %s\n", strerror(errno));
		return 1;
	} 

	uint16_t port;
	if (read(sockfd, &port, sizeof(port)) < 0) {
		printf("Read error: %s\n", strerror(errno));
		return 1;
	}
	printf("Got port: %u\n", ntohs(port));
	close(sockfd);

	if ((sockfd = connect_retry(REMOTE_HOST, port)) < 0) {
		printf("Error: %s\n", strerror(errno));
		return 1;
	}

	printf("Successfully connected.\n");
	unsigned char buf[BUF_SZ];
	read(sockfd, buf, sizeof(buf));
	if (buf[0] == 1) {
		printf("I'm first\n");
	} else {
		printf("I'm second\n");
	}
	uint64_t move_clock = deserialize_clock(&buf[1]);
	printf("Clock: %llu\n", move_clock);

	/* TODO: Refactor? */
	struct game *g = malloc(sizeof(*g));
	struct tile *tileset = malloc(sizeof(*tileset) * TILE_COUNT);
	get_deck(sockfd, tileset, TILE_SZ, TILE_COUNT);
	make_game_with_deck(g, tileset);
	free(tileset);

	for (int i = 0; i < TILE_COUNT; ++i) {
		printf("%s\n", print_tile(deal_tile(g), buf));
	}

	close(sockfd);

	free(g);
	return 0;
}
