#include <arpa/inet.h>

typedef struct mc_t {
	int    sock;
	poll_t poll;
	peer_t peer;
	struct sockaddr_in send;
	struct sockaddr_in recv;
} mc_t;

void mc_client(mc_t *mc, const char *group, int port);
void mc_close(mc_t *mc);
