#include <arpa/inet.h>

typedef struct bc_t {
	int    sock;
	poll_t poll;
	peer_t peer;
	struct sockaddr_in send;
	struct sockaddr_in recv;
} bc_t;

void bc_client(bc_t *bc, int port);
void bc_close(bc_t *bc);
