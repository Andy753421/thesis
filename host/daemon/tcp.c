#define _GNU_SOURCE

#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>

#include "main.h"
#include "util.h"
#include "tcp.h"

/* Constants */
#define SLAVES  100
#define BACKLOG 1000

/* Slave types */
typedef struct slave_t {
	int    used;
	int    sock;
	poll_t poll;
	peer_t peer;
} slave_t;

/* Local Data */
static slave_t slaves[SLAVES];

/* Local functions */
static void tcp_drop(void *_slave)
{
	slave_t *slave = _slave;
	poll_del(&slave->poll);
	shutdown(slave->sock, SHUT_RDWR);
	close(slave->sock);
	slave->used = 0;
}

static void tcp_recv(void *_slave)
{
	static char buf[1500];

	slave_t *slave = _slave;
	int len = recv(slave->sock, buf, sizeof(buf), 0);
	if (len <= 0) {
		debug("tcp_recv   - fail=%d", len);
		tcp_drop(slave);
	} else {
		debug("tcp_recv   - recv=%d", len);
		peer_send(&slave->peer, buf, len);
	}
}

static void tcp_send(void *_slave, void *buf, int len)
{
	slave_t *slave = _slave;
	int rlen = send(slave->sock, buf, len, 0);
	if (rlen != len) {
		debug("  tcp_send - fail=%d!=%d", rlen, len);
		tcp_drop(slave);
	} else {
		debug("  tcp_send - sent=%d", len);
	}
}

static void tcp_accept(void *_tcp)
{
	tcp_t *tcp = _tcp;
	int sock, flags;
	struct sockaddr_in addr = {};
	socklen_t length = sizeof(addr);

	if ((sock = accept(tcp->sock, (struct sockaddr*)&addr, &length)) < 0)
		error("Error accepting peer");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting slave flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting slave non-blocking");

	/* Find a free client */
	slave_t *slave = 0;
	for (int i = 0; !slave && i < SLAVES; i++)
		if (!slaves[i].used)
			slave = &slaves[i];
	if (!slave)
		return;

	/* Setup client */
	slave->used       = 1;
	slave->sock       = sock;

	slave->peer.send  = tcp_send;
	slave->peer.data  = slave;

	slave->poll.ready = tcp_recv;
	slave->poll.data  = slave;
	slave->poll.sock  = sock;

	peer_add(&slave->peer);
	poll_add(&slave->poll);
}

/* Broadcast Functions */
void tcp_server(tcp_t *tcp, const char *host, int port)
{
	int sock, flags, value = 1;
	struct sockaddr_in addr = {};

	/* Setup address */
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("Error opening tcp socket");

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting tcp socket reuseaddr");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting tcp socket flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting tcp socket non-blocking");

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding tcp socket");

	if (listen(sock, BACKLOG) < 0)
		error("Error listening on tcp socket");

	/* Setup client */
	tcp->sock      = sock;

	tcp->poll.ready = tcp_accept;
	tcp->poll.data  = tcp;
	tcp->poll.sock  = sock;

	poll_add(&tcp->poll);
}

void tcp_close(tcp_t *tcp)
{
	poll_del(&tcp->poll);
	shutdown(tcp->sock, SHUT_RDWR);
	close(tcp->sock);
}
