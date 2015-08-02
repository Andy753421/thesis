#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "main.h"
#include "peer.h"

/* Constants */
#define MAX_PEERS  10
#define MAX_READ   1500

/* Local data */
static int  src;
static int  dst;

static char buf[MAX_READ];
static int  len;

static int  peers[MAX_PEERS];
static int  types[MAX_PEERS];
static int  npeers;

/* Transmit / receive */
void peer_init(void)
{
}

int peer_add(int fd, int type)
{
	if (npeers >= MAX_PEERS) {
		debug("Aborting peer -- %d[%d]",
				MAX_PEERS, fd);
		return -1;
	}

	int id = npeers++;
	peers[id] = fd;
	types[id] = type;
	debug("Opened peer -- %d[%d]", id, fd);
	return id;
}

void peer_del(int id, int *old, int *new)
{
	*old = peers[id];
	peers[id] = peers[--npeers];
	*new = id<npeers ? peers[id] : -1;
	debug("Closed peer -- %d[%d]->[%d]", id, *old, *new);
}

int peer_recv(int id)
{
	src = peers[id];
	if ((len = recv(src, buf, sizeof(buf), 0)) <= 0)
		return -1;
	switch (types[id]) {
		case TYP_TCP: break;
		case TYP_UDP: break;
		case TYP_WEB: break;
	}
	debug("Read %d bytes <- %d[%d]", len, id, src);
	return npeers;
}

int peer_send(int id)
{
	dst = peers[id];
	if (src == dst) {
		debug("  From %d bytes <- %d[%d]", len, id, dst);
		return 0;
	}
	else if (types[id] == TYP_UDP) {
		struct sockaddr_in addr = {};

		addr.sin_family = AF_INET;
		addr.sin_port   = htons(12345);
		inet_pton(AF_INET, "255.255.255.255",
			&addr.sin_addr.s_addr);

		if (sendto(dst, buf, len, 0,
				(struct sockaddr*)&addr, sizeof(addr)) < len) {
			debug("  Dead %d bytes >> %d[%d]", len, id, dst);
			return -1;
		}
		debug("  Sent %d bytes >> %d[%d]", len, id, dst);
		return 0;
	}
	else if (send(dst, buf, len, 0) < len) {
		debug("  Dead %d bytes -> %d[%d]", len, id, dst);
		return -1;
	}
	else {
		debug("  Sent %d bytes -> %d[%d]", len, id, dst);
		return 0;
	}
}

void peer_exit(void)
{
	for (int i = 0; i < npeers; i++) {
		shutdown(peers[i], SHUT_RDWR);
		close(peers[i]);
	}
}
