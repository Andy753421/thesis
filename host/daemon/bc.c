#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "util.h"
#include "bc.h"

/* Local data */
static char rbuf[1500];
static char sbuf[1500];
static int  rlen;
static int  slen;

/* Local functions */
static void bc_recv(void *_bc)
{
	bc_t *bc = _bc;
	rlen = recvfrom(bc->sock, rbuf, sizeof(rbuf), 0, NULL, NULL);
	if (rlen == slen && !memcmp(rbuf, sbuf, slen))
		return;	// skip backets to self
	if (rlen <= 0) {
		debug("bc_recv    - fail=%d", rlen);
	} else {
		debug("bc_recv    - recv=%d", rlen);
		peer_send(&bc->peer, rbuf, rlen);
	}
}

static void bc_send(void *_bc, void *buf, int len)
{
	bc_t *bc = _bc;

	if (len > sizeof(sbuf))
		len = sizeof(sbuf);
	memcpy(sbuf, buf, len);

	slen = sendto(bc->sock, sbuf, len, 0,
			(struct sockaddr *)&bc->send, sizeof(bc->send));
	if (slen != len) {
		debug("  bc_send  - fail=%d!=%d", slen, len);
	} else {
		debug("  bc_send  - sent=%d", len);
	}
}

/* Broadcast Functions */
void bc_client(bc_t *bc, int port)
{
	int sock, flags, value = 1;

	/* Setup address */
	bc->send.sin_family = AF_INET;
	bc->send.sin_port   = htons(port);
	inet_pton(AF_INET, "255.255.255.255", &bc->send.sin_addr.s_addr);

	bc->recv.sin_family = AF_INET;
	bc->recv.sin_port   = htons(port);
	inet_pton(AF_INET, "255.255.255.255", &bc->recv.sin_addr.s_addr);

	/* Setup socket */
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		error("Error opening udp bc socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp bc socket broadcast");

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp bc socket reuseaddr");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting udp bc socket flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting udp bc socket non-blocking");

	if (bind(sock, (struct sockaddr*)&bc->recv, sizeof(bc->recv)) < 0)
		error("Error binding udp bc socket");

	/* Setup client */
	bc->sock       = sock;

	bc->peer.send  = bc_send;
	bc->peer.data  = bc;

	bc->poll.ready = bc_recv;
	bc->poll.data  = bc;
	bc->poll.sock  = sock;

	peer_add(&bc->peer);
	poll_add(&bc->poll);
}

void bc_close(bc_t *bc)
{
	peer_del(&bc->peer);
	poll_del(&bc->poll);
	shutdown(bc->sock, SHUT_RDWR);
	close(bc->sock);
}
