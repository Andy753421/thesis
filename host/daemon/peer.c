#define _POSIX_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"
#include "peer.h"

/* Web Socket Junk */
#define WS_FINISH 0x80
#define WS_MASK   0x80

enum {
	WS_MORE   = 0x00,
	WS_TEXT   = 0x01,
	WS_BINARY = 0x02,
	WS_CLOSE  = 0x08,
	WS_PING   = 0x09,
	WS_PONG   = 0x0A,
} opcode_t;

struct {
	uint8_t  opcode;
	uint64_t length;
	uint32_t masking;
	uint8_t  data[];
} mesg_t;

/* Constants */
#define MAX_PEERS   10
#define MAX_READ    1500
#define MAX_METHOD  8
#define MAX_PATH    256
#define MAX_VERSION 8
#define MAX_FIELD   128
#define MAX_VALUE   128

/* Web state */
typedef enum {
	MD_METHOD,
	MD_PATH,
	MD_VERSION,
	MD_FIELD,
	MD_VALUE,
	MD_BODY,
} md_t;

typedef struct {
	int  mode;
	int  index;

	// Header attributes
	int  connect;
	int  upgrade;
	int  accept;
	char key[28];

	// Parser strings
	char method[MAX_METHOD+1];
	char path[MAX_PATH+1];
	char version[MAX_VERSION+1];
	char field[MAX_FIELD+1];
	char value[MAX_VALUE+1];
} state_t;

/* Local data */
static int     src;
static int     dst;

static char    buf[MAX_READ];
static int     len;

static int     peers[MAX_PEERS];
static int     types[MAX_PEERS];
static state_t state[MAX_PEERS];
static int     npeers;

/* Web helper functions */
static inline void web_mode(int id, int mode, char *buf)
{
	buf[state[id].index] = '\0';
	state[id].index      = 0;
	state[id].mode       = mode;
}

void web_zero(int id)
{
	state[id].mode    = 0;
	state[id].index   = 0;
	state[id].connect = 0;
	state[id].upgrade = 0;
	state[id].accept  = 0;
}

void web_open(int id, const char *method,
		const char *path, const char *version)
{
	state_t *st = &state[id];
	debug("%s [%s] [%s]", st->method, st->path, st->version);
}

void web_head(int id, const char *field, const char *value)
{
	static char hash[20] = {};
	static char extra[]  = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	debug("\tHeader: %-20s -> [%s]", field, value);
	if (!strcasecmp(field, "Upgrade")) {
		state[id].upgrade = !!strcasestr(value, "websocket");
	}
	else if (!strcasecmp(field, "Connection")) {
		state[id].connect = !!strcasestr(value, "upgrade");
	}
	else if (!strcasecmp(field, "Sec-WebSocket-Key")) {
		gnutls_hash_hd_t sha1;
		gnutls_hash_init(&sha1, GNUTLS_DIG_SHA1);
		gnutls_hash(sha1, value, strlen(value));
		gnutls_hash(sha1, extra, strlen(extra));
		gnutls_hash_output(sha1, hash);
		state[id].accept = base64(hash, sizeof(hash),
			state[id].key, sizeof(state[id].key));
	}
}

int web_send(int id)
{
	state_t *st = &state[id];
	FILE    *fd = fdopen(peers[id], "w");
	debug("Sending to [%s]", st->path);
	if (!strcmp(st->path, "/")) {
		fprintf(fd,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=UTF-8\r\n"
			"\r\n"
			"<html>\r\n"
			"<body>\r\n"
			"<h1>Hello, World</h1>\r\n"
			"</body>\r\n"
			"</html>\r\n"
		);
	}
	else if (!strcmp(st->path, "/socket")) {
		debug("socket: connect=%d upgrade=%d accept=%d\n"
		      "        key=%s",
		      st->connect, st->upgrade, st->accept, st->key);

		if (st->connect && st->upgrade && st->accept) {
			fprintf(fd,
				"HTTP/1.1 101 WebSocket Protocol Handshake\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: WebSocket\r\n"
				"Sec-WebSocket-Accept: %.*s\r\n"
				"\r\n",
				st->accept, st->key
			);
			fflush(fd);
			return 0;
		}
		else {
			fprintf(fd,
				"HTTP/1.1 400 Bad Request\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"\r\n"
				"<html>\r\n"
				"<body>\r\n"
				"<h1>Bad Request!</h1>\r\n"
				"</body>\r\n"
				"</html>\r\n"
			);
		}
	}
	else {
		fprintf(fd,
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/html; charset=UTF-8\r\n"
			"\r\n"
			"<html>\r\n"
			"<body>\r\n"
			"<h1>Not Found!</h1>\r\n"
			"</body>\r\n"
			"</html>\r\n"
		);
	}
	fflush(fd);
	return -1;
}

int web_recv(int id)
{
	int status = 0;
	for (int i = 0; i < len; i++) {
		int      ch = buf[i];
		state_t *st = &state[id];

		if (ch == '\r')
			continue;
		if (ch == ' ' && st->index == 0)
			continue;

		switch (st->mode) {
			case MD_METHOD:
				if (ch == ' ')
					web_mode(id, MD_PATH, st->method);
				else if (st->index < MAX_METHOD)
					st->method[st->index++] = ch;
				break;

			case MD_PATH:
				if (ch == ' ')
					web_mode(id, MD_VERSION, st->path);
				else if (st->index < MAX_PATH)
					st->path[st->index++] = ch;
				break;

			case MD_VERSION:
				if (ch == '\n') {
					web_mode(id, MD_FIELD, st->version);
					web_open(id, st->method, st->path, st->version);
				}
				else if (st->index < MAX_VERSION)
					st->version[st->index++] = ch;
				break;

			case MD_FIELD:
				if (ch == '\n') {
					if ((status = web_send(id)) >= 0)
						web_mode(id, MD_BODY, st->field);
					else
						web_zero(id);
				} else if (ch == ':')
					web_mode(id, MD_VALUE, st->field);
				else if (st->index < MAX_FIELD)
					st->field[st->index++] = ch;
				break;

			case MD_VALUE:
				if (ch == '\n') {
					web_mode(id, MD_FIELD, st->value);
					web_head(id, st->field, st->value);
				} else if (st->index < MAX_VALUE)
					st->value[st->index++] = ch;
				break;

			case MD_BODY:
				debug("Body ..");
				break;
		}
	}
	return status;
}

/* Transmit / receive */
void peer_init(void)
{
}

int peer_add(int fd, int type)
{
	if (npeers >= MAX_PEERS) {
		debug("Aborting peer -- %d[%d]",
				MAX_PEERS, fd);
		return -1;
	}

	int id = npeers++;
	peers[id] = fd;
	types[id] = type;
	debug("Opened peer -- %d[%d]", id, fd);
	return id;
}

void peer_del(int id, int *old, int *new)
{
	// TODO - moving id breaks the status data
	web_zero(id);
	*old = peers[id];
	peers[id] = peers[--npeers];
	*new = id<npeers ? peers[id] : -1;
	debug("Closed peer -- %d[%d]->[%d]", id, *old, *new);
}

int peer_recv(int id)
{
	src = peers[id];
	if ((len = recvfrom(src, buf, sizeof(buf), 0, NULL, NULL)) <= 0)
		return -1;
	switch (types[id]) {
		case TYP_TCP:
			break;
		case TYP_UDP:
			break;
		case TYP_WEB:
			return web_recv(id);

	}
	debug("Read %d bytes <- %d[%d]", len, id, src);
	return npeers;
}

int peer_send(int id)
{
	dst = peers[id];
	if (src == dst) {
		debug("  From %d bytes <- %d[%d]", len, id, dst);
		return 0;
	}
	else if (types[id] == TYP_WEB) {
		char head[2] = { WS_FINISH|WS_TEXT, len };
		if (send(dst, head, 2, 0) < 2) {
			debug("  Dead %d bytes -> %d[%d]", len, id, dst);
			return -1;
		}
		if (send(dst, buf, len, 0) < len) {
			debug("  Dead %d bytes -> %d[%d]", len, id, dst);
			return -1;
		}
		return 0;
	}
	//else if (types[id] == TYP_UDP) {
	//	struct sockaddr_in addr = {};

	//	addr.sin_family = AF_INET;
	//	addr.sin_port   = htons(12345);
	//	inet_pton(AF_INET, "255.255.255.255",
	//		&addr.sin_addr.s_addr);

	//	if (sendto(dst, buf, len, 0,
	//			(struct sockaddr*)&addr, sizeof(addr)) < len) {
	//		debug("  Dead %d bytes >> %d[%d]", len, id, dst);
	//		return -1;
	//	}
	//	debug("  Sent %d bytes >> %d[%d]", len, id, dst);
	//	return 0;
	//}
	else if (send(dst, buf, len, 0) < len) {
		debug("  Dead %d bytes -> %d[%d]", len, id, dst);
		return -1;
	}
	else {
		debug("  Sent %d bytes -> %d[%d]", len, id, dst);
		return 0;
	}
}

void peer_exit(void)
{
	for (int i = 0; i < npeers; i++) {
		shutdown(peers[i], SHUT_RDWR);
		close(peers[i]);
	}
}
