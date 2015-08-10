#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>

#include "util.h"
#include "peer.h"

/* TPC Types */
struct udp_t {
	int sock;
};

/* UDP Data */
static udp_t   peers[UDP_PEERS];
static idx_t   cache[UDP_PEERS];
static alloc_t alloc;

/* UDP Functions */
void udp_init(void)
{
	alloc_init(&alloc, cache, UDP_PEERS);
}

udp_t *udp_add(int sock)
{
	udp_t *udp = alloc_get_ptr(&alloc, peers);
	if (udp) {
		udp->sock = sock;
	}
	return udp;
}

void udp_del(udp_t *udp)
{
	alloc_del_ptr(&alloc, peers, udp);
}

int udp_send(udp_t *udp, char *buf, int len)
{
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(12345);
	inet_pton(AF_INET, "255.255.255.255",
		&addr.sin_addr.s_addr);

	if (sendto(udp->sock, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr)) < len)
		return -1;

	return 1;
}

int udp_parse(udp_t *udp, char **buf, int *len)
{
	return 1;
}
