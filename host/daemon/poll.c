#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>

#include "util.h"
#include "poll.h"

/* Constants */
#define MAX_WAIT   1000
#define MAX_EVENTS 100

/* Poll Data */
static int epoll;
static int count;
static int index;
static struct epoll_event events[MAX_EVENTS];

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

void poll_del(int fd)
{
	if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL) < 0)
		error("Error deleting poll");
}

int poll_wait(int *data)
{
	if (index == count) {
		index = 0;
		do {
			errno = 0;
			count = epoll_wait(epoll, events, MAX_EVENTS, MAX_WAIT);
		} while (errno == EINTR);

		if (count < 0)
			error("Error waiting for event");

		if (count == 0)
			return EVT_TIMEOUT;
	}

	int type = (&events[index].data.u32)[0];
	*data    = (&events[index].data.u32)[1];
	index++;

	return type;
}
