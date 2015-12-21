#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <fcntl.h>
#include <unistd.h>

#include "main.h"
#include "util.h"
#include "web.h"
#include "http.h"
#include "ws.h"

/* Constants */
#define SLAVES  100
#define BACKLOG 1000

/* Slave types */
enum {
	WEB_NONE,
	WEB_HTTP,
	WEB_SOCK,
};

/* Slave types */
typedef struct slave_t {
	int    mode;
	int    sock;
	poll_t poll;
	peer_t peer;
	http_t http;
	ws_t   ws;
} slave_t;

/* Local Data */
static slave_t slaves[SLAVES];

/* Local functions */
static void web_drop(void *_slave)
{
	slave_t *slave = _slave;
	poll_del(&slave->poll);
	shutdown(slave->sock, SHUT_RDWR);
	close(slave->sock);
	slave->mode = 0;
}

static void web_recv(void *_slave)
{
	static char buf[10000];

	slave_t *slave = _slave;
	int len = recv(slave->sock, buf, sizeof(buf), 0);
	if (len <= 0) {
		debug("web_recv   - fail=%d", len);
		web_drop(slave);
	} else {
		debug("web_recv   - recv=%d mode=%d",
				len, slave->mode);
		for (int i = 0; i < len; i++) {
			switch (slave->mode) {
				case WEB_HTTP:
					switch (http_parse(&slave->http, buf[i])) {
						case HTTP_DONE:
							web_drop(slave);
							return;
						case HTTP_SOCK:
							slave->mode = WEB_SOCK;
							break;
					}
					break;
				case WEB_SOCK:
					switch (ws_parse(&slave->ws, buf[i], &slave->peer)) {
						case WS_DONE:
							web_drop(slave);
							return;
					}
					break;
			}
		}
	}
}

static void web_send(void *_slave, void *buf, int len)
{
	slave_t *slave = _slave;
	if (slave->mode != WEB_SOCK)
		return;
	int slen = ws_send(&slave->ws, buf, len);
	if (slen != len) {
		debug("  web_send - fail=%d!=%d", slen, len);
		web_drop(slave);
	} else {
		debug("  web_send - sent=%d", len);
	}
}

static void web_accept(void *_web)
{
	web_t *web = _web;
	int sock, flags, yes = 1, idle = 240;
	struct sockaddr_in addr = {};
	socklen_t length = sizeof(addr);

	if ((sock = accept(web->sock, (struct sockaddr*)&addr, &length)) < 0)
		error("Error accepting peer");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting slave flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting slave non-blocking");

	if (setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(int)))
		error("Error setting slave keep idle");

	if (setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &yes, sizeof(int)))
		error("Error setting slave keep count");

	if (setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &yes, sizeof(int)))
		error("Error setting slave keep interval");

	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)))
		error("Error setting slave keep alive");

	/* Find a free client */
	slave_t *slave = 0;
	for (int i = 0; !slave && i < SLAVES; i++)
		if (!slaves[i].mode)
			slave = &slaves[i];
	if (!slave)
		return;

	/* Setup client */
	slave->mode       = WEB_HTTP;
	slave->sock       = sock;

	slave->peer.send  = web_send;
	slave->peer.data  = slave;

	slave->poll.ready = web_recv;
	slave->poll.data  = slave;
	slave->poll.sock  = sock;

	slave->http.sock  = sock;
	slave->http.mode  = 0;
	slave->http.idx   = 0;

	slave->ws.sock    = sock;
	slave->ws.mode    = 0;
	slave->ws.idx     = 0;

	peer_add(&slave->peer);
	poll_add(&slave->poll);
}

/* Broadcast Functions */
void web_server(web_t *web, const char *host, int port)
{
	int sock, flags, value = 1;
	struct sockaddr_in addr = {};

	/* Setup address */
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_pton(AF_INET, host, &addr.sin_addr.s_addr);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("Error opening web socket");

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char*)&value, sizeof(value)) < 0)
		error("Error setting web socket reuseaddr");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting web socket flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting web socket non-blocking");

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding web socket");

	if (listen(sock, BACKLOG) < 0)
		error("Error listening on web socket");

	/* Setup client */
	web->sock      = sock;

	web->poll.ready = web_accept;
	web->poll.data  = web;
	web->poll.sock  = sock;

	poll_add(&web->poll);
}

void web_close(web_t *web)
{
	poll_del(&web->poll);
	shutdown(web->sock, SHUT_RDWR);
	close(web->sock);
}
