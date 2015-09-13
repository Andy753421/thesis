#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include "main.h"
#include "util.h"

/* Includes */
#include "bc.h"
#include "mc.h"
#include "tcp.h"
#include "web.h"

/* Constants */
#define MAX_WAIT   1000
#define MAX_EVENTS 100

/* Constants */
#define BC_PORT   12345

#define MC_GROUP  "224.0.0.1"
#define MC_PORT   12345

#define TCP_HOST  "0.0.0.0"
#define TCP_PORT  12345

#define WEB_HOST  "0.0.0.0"
#define WEB_PORT  8080

/* Options */
static struct option long_options[] = {
	/* name hasarg flag val */
	{"verbose", 0, NULL, 'v'},
	{"help",    0, NULL, 'h'},
	{NULL,      0, NULL,  0 },
};

/* Local Data */
static int     epoll;
static struct  epoll_event events[MAX_EVENTS];
static peer_t *head;
static peer_t *tail;

/* Interfaces */
static bc_t    bc;
static mc_t    mc;
static tcp_t   tcp;
static web_t   web;

/* Socket functions */
void poll_add(poll_t *data)
{
	struct epoll_event ctl = {
		.events   = EPOLLIN,
		.data.ptr = data,
	};
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, data->sock, &ctl) < 0)
		error("Error adding poll");
}

void poll_del(poll_t *data)
{
	if (epoll_ctl(epoll, EPOLL_CTL_DEL, data->sock, NULL) < 0)
		error("Error deleting poll");
}

/* Peer functions */
void peer_add(peer_t *peer)
{
	if (tail) {
		tail->next = peer;
		peer->prev = tail;
		peer->next = NULL;
		tail       = peer;
	} else {
		peer->prev = NULL;
		peer->next = NULL;
		head       = peer;
		tail       = peer;
	}
}

void peer_del(peer_t *peer)
{
	peer_t *next = peer->next;
	peer_t *prev = peer->prev;
	if (next) next->prev = prev;
	if (prev) prev->next = next;
	if (head == peer) head = next;
	if (tail == peer) tail = prev;
}

void peer_send(peer_t *peer, void *buf, int len)
{
	for (peer_t *cur = head; cur; cur = cur->next)
		if (cur != peer)
			cur->send(cur->data, buf, len);
}

/* Helper functions */
static void on_sigint(int signum)
{
	debug("\rShutting down");
	exit(0);
}

static void usage(char *name)
{
	printf("Usage:\n");
	printf("  %s [OPTION...]\n", name);
	printf("\n");
	printf("Options\n");
	printf("  -v, --verbose Increase verbosity level\n");
	printf("  -h, --help    Print usage information\n");
}

static void parse(int argc, char **argv)
{
	while (1) {
		int c = getopt_long(argc, argv, "vh", long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
			case 'v':
				verbose++;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			default:
				usage(argv[0]);
				exit(-1);
		}
	}
}

/* Main */
int main(int argc, char **argv)
{
	/* Setup main */
	setbuf(stdout, NULL);
	signal(SIGINT, on_sigint);
	parse(argc, argv);

	if ((epoll = epoll_create(1)) < 0)
		error("Error creating epoll");

	/* Setup initial peers */
	bc_client(&bc, BC_PORT);
	mc_client(&mc, MC_GROUP, MC_PORT);
	tcp_server(&tcp, TCP_HOST, TCP_PORT);
	web_server(&web, WEB_HOST, WEB_PORT);

	/* Run main loop */
	while (1) {
		errno = 0;
		int count = epoll_wait(epoll, events, MAX_EVENTS, MAX_WAIT);
		if (errno == EINTR)
			continue;
		if (count < 0)
			error("Error waiting for event");
		for (int i = 0; i < count; i++) {
			poll_t *poll = events[i].data.ptr;
			poll->ready(poll->data);
		}
	}

	return 0;
}
