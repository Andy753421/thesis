#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>

#include "util.h"
#include "peer.h"

/* TPC Types */
struct tcp_t {
	int sock;
};

/* TCP Data */
static tcp_t   peers[TCP_PEERS];
static idx_t   cache[TCP_PEERS];
static alloc_t alloc;

/* TCP Functions */
void tcp_init(void)
{
	alloc_init(&alloc, cache, TCP_PEERS);
}

tcp_t *tcp_add(int sock)
{
	tcp_t *tcp = alloc_get_ptr(&alloc, peers);
	if (tcp) {
		tcp->sock = sock;
	}
	return tcp;
}

void tcp_del(tcp_t *tcp)
{
	alloc_del_ptr(&alloc, peers, tcp);
}

int tcp_send(tcp_t *tcp, char *buf, int len)
{
	if (send(tcp->sock, buf, len, 0) < len)
		return -1;
	return 1;
}

int tcp_parse(tcp_t *tcp, char **buf, int *len)
{
	return 1;
}
