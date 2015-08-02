#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "sock.h"

/* Functions */
void sock_bc(const char *host, int port, int *fd)
{
	int flags, value = 1;
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		error("Error opening udp bc socket");

	if (setsockopt(*fd, SOL_SOCKET, SO_BROADCAST,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp bc socket broadcast");

	if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp bc socket reuseaddr");

	if ((flags = fcntl(*fd, F_GETFL, 0)) < 0)
		error("Error getting udp bc socket flags");

	if (fcntl(*fd, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting udp bc socket non-blocking");

	if (bind(*fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding udp bc socket");

	if (connect(*fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error connecting udp bc socket");
}

void sock_mc(const char *host, int port, int *fd)
{
	int flags, value = 1;
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		error("Error opening udp mc socket");

	if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp mc socket reuseaddr");

	if ((flags = fcntl(*fd, F_GETFL, 0)) < 0)
		error("Error getting udp mc socket flags");

	if (fcntl(*fd, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting udp mc socket non-blocking");

	if (bind(*fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding udp mc socket");
}

void sock_tcp(const char *host, int port, int *fd)
{
	int flags, value = 1;
	struct sockaddr_in addr = {};

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("Error opening tcp socket");

	if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting udp socket reuseaddr");

	if ((flags = fcntl(*fd, F_GETFL, 0)) < 0)
		error("Error getting tcp socket flags");

	if (fcntl(*fd, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting tcp socket non-blocking");

	if (bind(*fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding tcp socket");

	if (listen(*fd, 0) < 0)
		error("Error listening on tcp socket");
}

int sock_accept(int master)
{
	int flags, slave;
	struct sockaddr_in addr = {};
	socklen_t length = sizeof(addr);

	if ((slave = accept(master, (struct sockaddr*)&addr, &length)) < 0)
		error("Error accepting peer");

	if ((flags = fcntl(slave, F_GETFL, 0)) < 0)
		error("Error getting slave flags");

	if (fcntl(slave, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting slave non-blocking");

	return slave;
}

void sock_close(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}
