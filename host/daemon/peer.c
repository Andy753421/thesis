#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

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

//static int  types[MAX_PEERS];
static int  peers[MAX_PEERS];
static int  npeers;

/* Transmit / receive */
void peer_init(void)
{
}

int peer_add(int fd)
{
	if (npeers >= MAX_PEERS) {
		debug("Aborting peer -- %d[%d]\n",
				MAX_PEERS, fd);
		return -1;
	}

	int id = npeers++;
	peers[id] = fd;
	debug("Opened peer -- %d[%d]\n", id, fd);
	return id;
}

void peer_del(int id, int *old, int *new)
{
	*old = peers[id];
	debug("Closed peer -- %d[%d]\n", id, *old);
	peers[id] = peers[--npeers];
	*new = id<npeers ? peers[id] : -1;
}

int peer_recv(int id)
{
	src = peers[id];
	if ((len = recv(src, buf, sizeof(buf), 0)) <= 0)
		return -1;
	debug("Read %d bytes <- %d[%d]\n", len, id, src);
	return npeers;
}

int peer_send(int id)
{
	dst = peers[id];
	if (src == dst)
		return 0;
	if (send(dst, buf, len, 0) < len)
		return -1;
	debug("  Sent %d bytes -> %d[%d]\n", len, id, dst);
	return 0;
}

void peer_exit(void)
{
	for (int i = 0; i < npeers; i++) {
		shutdown(peers[i], SHUT_RDWR);
		close(peers[i]);
	}
}
