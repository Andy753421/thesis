#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

/* Macros */
#define IP_ADDR(a,b,c,d) \
	(((a) << 030) |  \
	 ((b) << 020) |  \
	 ((c) << 010) |  \
	 ((d) << 000))


/* Constants */
#define MAX_PEERS  10
#define MAX_READ   1500
#define MAX_WAIT   1000

#define HOST    "0.0.0.0"
#define PORT    12345

/* Local data */
int master;
int epoll;
int peers[MAX_PEERS];
int npeers;

/* Helpers */
void error(const char *str)
{
	perror(str);
	exit(-1);
}

/* Network helpers */
int init_epoll(void)
{
	int fd;
	if ((fd = epoll_create(1)) < 0)
		error("Error creating epoll");
	return fd;
}

int init_event(int *fd, int *events)
{
	int count;
	struct epoll_event event;

	if ((count = epoll_wait(epoll, &event, 1, MAX_WAIT)) < 0)
		error("waiting for event");

	if (count > 0) {
		*fd     = event.data.fd;
		*events = event.events;
	}

	return count;
}

int init_master(const char *host, int port)
{
	/* Setup socket */
	int flags, fd;
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("Error opening master");

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding master");

	if (listen(fd, 0) < 0)
		error("Error listening on port");

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		error("Error getting master flags");

	if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting master non-blocking");

	/* Setup poll */
	struct epoll_event ctl = {};

	ctl.events  = EPOLLIN | EPOLLET;
	ctl.data.fd = fd;

	if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ctl) < 0)
		error("Error adding master poll");

	return fd;
}

int init_slave(int master)
{
	/* Setup socket */
	int flags, fd;
	struct sockaddr_in addr = {};
	socklen_t length = sizeof(addr);

	if ((fd = accept(master,
			(struct sockaddr*)&addr, &length)) < 0)
		error("Error accepting peer");

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		error("Error getting slave flags");

	if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting slave non-blocking");

	/* Setup poll */
	struct epoll_event ctl = {};

	ctl.events  = EPOLLIN | EPOLLET;
	ctl.data.fd = fd;

	if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ctl) < 0)
		error("Error adding slave poll");

	return fd;
}

void close_slave(int fd)
{
	/* Remove poll */
	struct epoll_event ctl = {};

	ctl.events  = EPOLLIN | EPOLLET;
	ctl.data.fd = fd;
	if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, &ctl) < 0)
		error("Error deleting slave poll");

	/* Shutdown socket */
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

/* Misc */
void do_timeout(void)
{
	//printf("Timeout\n");
}

/* Transmit / receive */
int peer_find(int fd)
{
	for (int i = 0; i < npeers; i++)
		if (peers[i] == fd)
			return i;
	error("Error finding peer");
	return 0;
}

void peer_open(int fd)
{
	printf("Opened peer -- %d\n", fd);
	if (npeers >= MAX_PEERS) {
		close_slave(fd);
		return;
	}
	peers[npeers++] = fd;
}

void peer_close(int idx)
{
	int fd = peers[idx];
	printf("Closed peer -- %d\n", fd);
	close_slave(fd);
	peers[idx] = peers[--npeers];
}

void peer_write(int idx, void *buf, int len)
{
	int bytes, fd = peers[idx];
	if ((bytes = send(fd, buf, len, 0)) < len)
		peer_close(idx);
	printf("  Sent %d bytes -> %d\n", bytes, fd);
}

void peer_read(int idx)
{
	static char buf[MAX_READ];
	int bytes, fd = peers[idx];

	if ((bytes = recv(fd, buf, sizeof(buf), 0)) < 0) {
		peer_close(idx);
		return;
	}
	printf("Read %d bytes <- %d\n", bytes, fd);
	for (int i = 0; i < npeers; i++)
		if (i != idx)
			peer_write(i, buf, bytes);
}

/* Main */
int main(int argc, char **argv)
{
	/* Setup polling and master socket */
	epoll  = init_epoll();
	master = init_master(HOST, PORT);

	/* Listen for connections */
	while (1) {
		int fd, evs;

		/* Timeout */
		if (!init_event(&fd, &evs))
			do_timeout();

		/* Accept */
		else if (fd == master)
			peer_open(init_slave(master));

		/* Close */
		else if (evs & EPOLLHUP)
			peer_close(peer_find(fd));

		/* Read */
		else
			peer_read(peer_find(fd));

		fflush(stdout);
	}

	// TODO handle ctrl-c

	return 0;
}
