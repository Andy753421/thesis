#include <signal.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "peer.h"
#include "poll.h"
#include "sock.h"

/* Constants */
#define HOST  "0.0.0.0"
#define PORT  12345

/* Helpers */
void debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void error(const char *str)
{
	perror(str);
	exit(-1);
}

/* Main */
void update(int id)
{
	int old, new;
	peer_del(id, &old, &new);
	poll_del(old);
	sock_close(old);
	if (new >= 0)
		poll_mod(new, EVT_SLAVE, id);
}

void on_sigint(int signum)
{
	printf("\rShutting down\n");
	peer_exit();
	sock_exit();
	exit(0);
}

int main(int argc, char **argv)
{
	int fd, id, data, evs, cnt;

	/* Setup main */
	setbuf(stdout, NULL);
	signal(SIGINT, on_sigint);

	/* Setup polling and sockets */
	peer_init();
	poll_init();
	sock_init(HOST, PORT, &fd);

	/* Setup polling */
	poll_add(fd, EVT_MASTER, 0);

	/* Listen for connections */
	while (1) {
		switch (poll_wait(&data, &evs)) {
			case EVT_TIMEOUT:
				break;

			case EVT_MASTER:
				fd = sock_accept();
				id = peer_add(fd);
				if (id >= 0)
					poll_add(fd, EVT_SLAVE, id);
				else
					sock_close(fd);
				break;

			case EVT_SLAVE:
				id  = data;
				cnt = peer_recv(id);
				if (cnt < 0) {
					update(id);
					continue;
				}
				for (int i = 0; i < cnt; i++) {
					if (peer_send(i) < 0) {
						update(id);
						cnt--;
						i--;
					}
				}
				break;
		}
	}

	return 0;
}
