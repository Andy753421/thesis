#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>

#include "main.h"
#include "util.h"
#include "mc.h"

/* Local functions */
static void mc_recv(void *_mc)
{
	static char buf[1500];

	mc_t *mc = _mc;
	int len = recvfrom(mc->sock, buf, sizeof(buf), 0, NULL, NULL);
	if (len <= 0) {
		debug("mc_recv    - fail=%d", len);
	} else {
		debug("mc_recv    - recv=%d", len);
		peer_send(&mc->peer, buf, len);
	}
}

static void mc_send(void *_mc, void *buf, int len)
{
	mc_t *mc = _mc;
	int rlen = sendto(mc->sock, buf, len, 0,
			(struct sockaddr *)&mc->send, sizeof(mc->send));
	if (rlen != len) {
		debug("  mc_send  - fail=%d!=%d", rlen, len);
	} else {
		debug("  mc_send  - sent=%d", len);
	}
}

/* Broadcast Functions */
void mc_client(mc_t *mc, const char *group, int port)
{
	int sock, flags, yes = 1, no = 0;
	struct ip_mreq mreq;

	/* Setup address */
	mc->send.sin_family = AF_INET;
	mc->send.sin_port   = htons(port);
	inet_pton(AF_INET, group,     &mc->send.sin_addr.s_addr);

	mc->recv.sin_family = AF_INET;
	mc->recv.sin_port   = htons(port);
	inet_pton(AF_INET, group,     &mc->recv.sin_addr.s_addr);

	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_interface.s_addr);
	inet_pton(AF_INET, group,     &mreq.imr_multiaddr.s_addr);

	/* Setup socket */
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		error("Error opening multicast socket");

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char*)&yes, sizeof(yes)) < 0)
		error("Error setting multicast socket reuseaddr");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting multicast socket flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting multicast socket non-blocking");

	if (bind(sock, (struct sockaddr*)&mc->recv, sizeof(mc->recv)) < 0)
		error("Error binding multicast socket");

	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		error("Error joining multicast group");

	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &no, sizeof(no)) < 0)
		error("Error setting multicast loop diabled");

	/* Setup client */
	mc->sock       = sock;

	mc->peer.send  = mc_send;
	mc->peer.data  = mc;

	mc->poll.ready = mc_recv;
	mc->poll.data  = mc;
	mc->poll.sock  = sock;

	peer_add(&mc->peer);
	poll_add(&mc->poll);
}

void mc_close(mc_t *mc)
{
	peer_del(&mc->peer);
	poll_del(&mc->poll);
	shutdown(mc->sock, SHUT_RDWR);
	close(mc->sock);
}
