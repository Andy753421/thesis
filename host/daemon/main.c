#include <signal.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "peer.h"
#include "poll.h"
#include "sock.h"

/* Constants */
#define BC_HOST   "255.255.255.255"
#define BC_PORT   12345

#define MC_HOST   "224.0.0.1"
#define MC_PORT   12345

#define TCP_HOST  "0.0.0.0"
#define TCP_PORT  12345

#define WEB_HOST  "0.0.0.0"
#define WEB_PORT  8080

/* Configuration */
#define USE_BC
#undef  USE_MC
#define USE_TCP
#define USE_WEB

/* Master sockets */
static int master[NUM_TYPES];

/* Main */
void on_sigint(int signum)
{
	debug("\rShutting down");
#ifdef USE_TCP
	sock_close(master[TYP_TCP]);
#endif
#ifdef USE_WEB
	sock_close(master[TYP_WEB]);
#endif
	peer_exit();
	exit(0);
}

int main(int argc, char **argv)
{
#if defined(USE_BC) || defined(USE_MC)
	int fd, id;
#endif

	/* Check arguments */
	if (argc > 1 && !strcmp(argv[1], "-v"))
		verbose = 1;

	/* Setup main */
	setbuf(stdout, NULL);
	signal(SIGINT, on_sigint);

	/* Setup polling and sockets */
	peer_init();
	poll_init();

	/* Setup initial peers */
#ifdef USE_BC
	sock_bc(BC_HOST, BC_PORT, &fd);
	id = peer_add(fd, TYP_UDP);
	poll_add(fd, EVT_SLAVE, id);
#endif
#ifdef USE_MC
	sock_mc(MC_HOST, MC_PORT, &fd);
	id = peer_add(fd, TYP_UDP);
	poll_add(fd, EVT_SLAVE, id);
#endif
#ifdef USE_TCP
	sock_tcp(TCP_HOST, TCP_PORT, &master[TYP_TCP]);
	poll_add(master[TYP_TCP], EVT_MASTER, TYP_TCP);
#endif
#ifdef USE_WEB
	sock_tcp(WEB_HOST, WEB_PORT, &master[TYP_WEB]);
	poll_add(master[TYP_WEB], EVT_MASTER, TYP_WEB);
#endif

	/* Listen for connections */
	while (1) {
		char *buf;
		int fd, id, data, cnt, len;
		int ev = poll_wait(&data);

		switch (ev) {
			case EVT_TIMEOUT:
				break;

			case EVT_MASTER:
				while ((fd = sock_accept(master[data])) >= 0) {
					id = peer_add(fd, data);
					if (id >= 0)
						poll_add(fd, EVT_SLAVE, id);
					else
						sock_close(fd);
				}
				break;

			case EVT_SLAVE:
				id  = data;
				cnt = peer_recv(id, &buf, &len);
				if (cnt < 0) {
					int fd = peer_del(id);
					poll_del(fd);
					sock_close(fd);
					continue;
				}
				for (int ii = 0; ii < cnt; ii++) {
					int to = peer_get(ii);
					if (id == to)
						continue;
					if (peer_send(to, buf, len) < 0) {
						int fd = peer_del(id);
						poll_del(fd);
						sock_close(fd);
						cnt--;
						ii--;
					}
				}
				break;
		}
	}

	return 0;
}
