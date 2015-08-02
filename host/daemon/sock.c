#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "sock.h"

/* Socket data */
static int master;

/* Functions */
void sock_init(const char *host, int port, int *fd)
{
	int flags;
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((master = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("Error opening master");

	if (bind(master, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding master");

	if (listen(master, 0) < 0)
		error("Error listening on port");

	if ((flags = fcntl(master, F_GETFL, 0)) < 0)
		error("Error getting master flags");

	if (fcntl(master, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting master non-blocking");

	*fd = master;
}

int sock_accept(void)
{
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

	return fd;
}

void sock_close(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

void sock_exit(void)
{
	shutdown(master, SHUT_RDWR);
	close(master);
}
