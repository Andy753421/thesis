#include <sys/epoll.h>
#include <stdlib.h>

#include "main.h"
#include "poll.h"

/* Constants */
#define MAX_WAIT   1000

/* Poll Data */
static int epoll;

/* Poll functions */
void poll_init(void)
{
	if ((epoll = epoll_create(1)) < 0)
		error("Error creating epoll");
}

void poll_add(int fd, int type, int data)
{
	struct epoll_event ctl = {};

	ctl.events = EPOLLIN;
	(&ctl.data.u32)[0] = type;
	(&ctl.data.u32)[1] = data;

	if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ctl) < 0)
		error("Error adding poll");
}

void poll_mod(int fd, int type, int data)
{
	struct epoll_event ctl = {};

	ctl.events = EPOLLIN;
	(&ctl.data.u32)[0] = type;
	(&ctl.data.u32)[1] = data;

	if (epoll_ctl(epoll, EPOLL_CTL_MOD, fd, &ctl) < 0)
		error("Error modifying poll");
}

void poll_del(int fd)
{
	if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL) < 0)
		error("Error deleting poll");
}

int poll_wait(int *data, int *events)
{
	struct epoll_event event;
	int count = epoll_wait(epoll, &event, 1, MAX_WAIT);

	if (count < 0)
		error("Error waiting for event");

	if (count == 0)
		return EVT_TIMEOUT;

	*events = event.events;
	*data   = (&event.data.u32)[1];
	return    (&event.data.u32)[0];
}
