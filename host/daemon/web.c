#define _GNU_SOURCE

#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#endif
#ifdef USE_OPENSSL
#include <openssl/sha.h>
#endif

#include "main.h"
#include "util.h"
#include "web.h"

/* Constants */
#define SLAVES  100
#define BACKLOG 1000

/* Constants */
#define MAX_METHOD  8
#define MAX_PATH    256
#define MAX_VERSION 8
#define MAX_FIELD   128
#define MAX_VALUE   128
#define MAX_DATA    1500

/* Web Sockets Flags */
#define WS_FINISH   0x80
#define WS_MASK     0x80

/* Web Socket Opcodes */
enum {
	WS_MORE   = 0x00,
	WS_TEXT   = 0x01,
	WS_BINARY = 0x02,
	WS_CLOSE  = 0x08,
	WS_PING   = 0x09,
	WS_PONG   = 0x0A,
} opcode_t;

/* HTTP state */
typedef enum {
	MD_METHOD,
	MD_PATH,
	MD_VERSION,
	MD_FIELD,
	MD_VALUE,
	MD_BODY,

	MD_OPCODE,
	MD_LENGTH,
	MD_EXTEND,
	MD_MASKED,
	MD_DATA,
} md_t;

/* Slave types */
typedef struct slave_t {
	int    used;
	int    sock;
	poll_t poll;
	peer_t peer;

	// Common stuff
	int    mode;
	int    idx;

	// Http Request
	char   req_method[MAX_METHOD+1];
	char   req_path[MAX_PATH+1];
	char   req_version[MAX_VERSION+1];

	// Http Headers
	int    hdr_connect;
	int    hdr_upgrade;
	int    hdr_accept;
	char   hdr_key[28];
	char   hdr_field[MAX_FIELD+1];
	char   hdr_value[MAX_VALUE+1];

	// WebSocket attributes
	uint8_t  ws_finish;
	uint8_t  ws_opcode;
	uint8_t  ws_masked;
	uint8_t  ws_mask[4];
	uint64_t ws_length;
	char     ws_data[MAX_DATA];
} slave_t;

/* Debug data */
const char *str_mode[] = {
	[MD_METHOD]  "METHOD",
	[MD_PATH]    "PATH",
	[MD_VERSION] "VERSION",
	[MD_FIELD]   "FIELD",
	[MD_VALUE]   "VALUE",
	[MD_BODY]    "BODY",

	[MD_OPCODE]  "OPCODE",
	[MD_LENGTH]  "LENGTH",
	[MD_EXTEND]  "EXTEND",
	[MD_MASKED]  "MASKED",
	[MD_DATA]    "DATA",
};

const char *str_opcode[0xF] = {
	[ 0] "more  ",
	[ 1] "text  ",
	[ 2] "binary",
	[ 8] "close ",
	[ 9] "ping  ",
	[10] "pong  ",
};

/* Local Data */
static slave_t slaves[SLAVES];

/* Forward functions */
static void web_drop(void *_slave);

/* Web helper functions */
static void web_mode(slave_t *web, int mode, char *buf)
{
	buf[web->idx] = '\0';
	web->idx      = 0;
	web->mode     = mode;
}


static int web_printf(int sock, const char *fmt, ...)
{
	static char buf[512];

	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	return send(sock, buf, len, MSG_DONTWAIT);
}

/* Web parser functions */
static void web_open(slave_t *web, const char *method,
		const char *path, const char *version)
{
	trace("  web_open:  %s [%s] [%s]", method, path, version);
}

static void web_head(slave_t *web, const char *field, const char *value)
{
	static char hash[20] = {};
	static char extra[]  = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	trace("    web_head: %-20s -> [%s]", field, value);
	if (!strcasecmp(field, "Upgrade")) {
		web->hdr_upgrade = !!strcasestr(value, "websocket");
	}
	else if (!strcasecmp(field, "Connection")) {
		web->hdr_connect = !!strcasestr(value, "upgrade");
	}
	else if (!strcasecmp(field, "Sec-WebSocket-Key")) {
#ifdef USE_GNUTLS
		gnutls_hash_hd_t sha1;
		gnutls_hash_init(&sha1, GNUTLS_DIG_SHA1);
		gnutls_hash(sha1, value, strlen(value));
		gnutls_hash(sha1, extra, strlen(extra));
		gnutls_hash_output(sha1, hash);
#endif
#ifdef USE_OPENSSL
		SHA_CTX sha1;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, value, strlen(value));
		SHA1_Update(&sha1, extra, strlen(extra));
		SHA1_Final((unsigned char*)hash, &sha1);
#endif
		web->hdr_accept = base64(hash, sizeof(hash),
			web->hdr_key, sizeof(web->hdr_key));
	}
}

static int web_body(slave_t *web)
{
	trace("  web_body:  path=%s", web->req_path);
	if (!strcmp(web->req_path, "/")) {
		web_printf(web->sock,
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
	else if (!strcmp(web->req_path, "/socket")) {
		trace("  web_body:  socket - connect=%d upgrade=%d accept=%d\n"
		      "            key=%s",
		      web->hdr_connect, web->hdr_upgrade, web->hdr_accept, web->hdr_key);

		if (web->hdr_connect && web->hdr_upgrade && web->hdr_accept) {
			web_printf(web->sock,
				"HTTP/1.1 101 WebSocket Protocol Handshake\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: WebSocket\r\n"
				"Sec-WebSocket-Accept: %.*s\r\n"
				"\r\n",
				web->hdr_accept, web->hdr_key
			);
			return MD_OPCODE;
		}
		else {
			web_printf(web->sock,
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
		web_printf(web->sock,
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
	return -1;
}

static void web_parse(slave_t *web, char ch)
{
	int mode = 0;

	if (web->mode < MD_BODY) {
		if (ch == '\r')
			return;
		if (ch == ' ' && web->idx == 0)
			return;
	}

	switch (web->mode) {
		/* HTTP Parsing */
		case MD_METHOD:
			if (ch == ' ')
				web_mode(web, MD_PATH, web->req_method);
			else if (web->idx < MAX_METHOD)
				web->req_method[web->idx++] = ch;
			break;

		case MD_PATH:
			if (ch == ' ')
				web_mode(web, MD_VERSION, web->req_path);
			else if (web->idx < MAX_PATH)
				web->req_path[web->idx++] = ch;
			break;

		case MD_VERSION:
			if (ch == '\n') {
				web_mode(web, MD_FIELD, web->req_version);
				web_open(web, web->req_method,
					      web->req_path,
					      web->req_version);
			}
			else if (web->idx < MAX_VERSION)
				web->req_version[web->idx++] = ch;
			break;

		case MD_FIELD:
			if (ch == '\n') {
				if ((mode = web_body(web)) >= 0)
					web_mode(web, mode, web->hdr_field);
			} else if (ch == ':')
				web_mode(web, MD_VALUE, web->hdr_field);
			else if (web->idx < MAX_FIELD)
				web->hdr_field[web->idx++] = ch;
			break;

		case MD_VALUE:
			if (ch == '\n') {
				web_mode(web, MD_FIELD, web->hdr_value);
				web_head(web, web->hdr_field, web->hdr_value);
			} else if (web->idx < MAX_VALUE)
				web->hdr_value[web->idx++] = ch;
			break;

		case MD_BODY:
			trace("  web_parse: body");
			break;

		/* Web Socket Parsing */
		case MD_OPCODE:
			trace("    web_parse: opcode - fin=%d, op=%s",
					(ch&0x80) !=  0,
					str_opcode[ch&0x0F] ?: "[opcode error]");

			web->ws_finish = (ch&0x80)!=0;
			web->ws_opcode = (ch&0x0F);
			web->mode      = MD_LENGTH;
			break;

		case MD_LENGTH:
			trace("    web_parse: length - masked=%d, length=%d",
					(ch&0x80) != 0,
					(ch&0x7F));

			web->ws_length = (ch&0x7F);
			web->ws_masked = (ch&0x80)!=0;


			if (web->ws_length == 126) {
				web->ws_length = 0;
				web->idx       = 2;
				web->mode      = MD_EXTEND;
			}
			else if (web->ws_length == 127) {
				web->ws_length = 0;
				web->idx       = 8;
				web->mode      = MD_EXTEND;
			}
			else if (web->ws_masked) {
				web->mode      = MD_MASKED;
			}
			else {
				web->mode      = MD_DATA;
			}
			break;

		case MD_EXTEND:
			trace("    web_parse: extend - %d=%02hhx", web->idx, ch);

			web->idx       -= 1;
			web->ws_length |= ((uint64_t)ch) << (web->idx*8);

			if (web->idx == 0) {
				if (web->ws_masked)
					web->mode = MD_MASKED;
				else
					web->mode = MD_DATA;
			}
			break;

		case MD_MASKED:
			trace("    web_parse: masked - %d=%02hhx", web->idx, ch);

			web->ws_mask[web->idx++] = ch;
			if (web->idx >= 4) {
				web->idx  = 0;
				web->mode = MD_DATA;
			}
			break;

		case MD_DATA:
			if (web->idx < MAX_DATA) {
				uint8_t mask = web->ws_mask[web->idx%4];
				web->ws_data[web->idx] = ch ^ mask;
			}
			web->idx++;
			if (web->idx < web->ws_length)
				return;

			trace("    web_parse: data   - '%.*s'",
					web->ws_length, web->ws_data);
			web->idx  = 0;
			web->mode = MD_OPCODE;

			switch (web->ws_opcode) {
				case WS_TEXT:
				case WS_BINARY:
					peer_send(&web->peer, web->ws_data, web->ws_length);
					break;
				case WS_CLOSE:
					web_drop(web);
					break;
			}
	}
}

/* Local functions */
static void web_drop(void *_slave)
{
	slave_t *slave = _slave;
	poll_del(&slave->poll);
	shutdown(slave->sock, SHUT_RDWR);
	close(slave->sock);
	slave->used = 0;
}

static void web_recv(void *_slave)
{
	static char buf[1500];

	slave_t *slave = _slave;
	int len = recv(slave->sock, buf, sizeof(buf), 0);
	if (len <= 0) {
		debug("web_recv   - fail=%d", len);
		web_drop(slave);
	} else {
		debug("web_recv   - recv=%d", len);
		for (int i = 0; i < len; i++)
			web_parse(slave, buf[i]);
	}
}

static void web_send(void *_slave, void *buf, int len)
{
	slave_t *slave    = _slave;
	int      bytes    = 0;
	uint8_t  head[10] = { WS_FINISH|WS_TEXT };

	uint8_t  *flag  = (uint8_t *)&head[1];
	uint16_t *ext16 = (uint16_t*)&head[2];
	uint64_t *ext64 = (uint64_t*)&head[2];

	if (len < 126) {
		bytes  = 2;
		*flag  = len;
	}
	else if (len < 0x10000) {
		bytes  = 4;
		*flag  = 126;
		*ext16 = net16(len);
	}
	else {
		bytes  = 10;
		*flag  = 127;
		*ext64 = net64(len);
	}

	if (write(slave->sock, head, bytes) != bytes) {
		web_drop(slave);
		return;
	}
	if (write(slave->sock, buf, len) != len) {
		web_drop(slave);
		return;
	}
}

static void web_accept(void *_web)
{
	web_t *web = _web;
	int sock, flags;
	struct sockaddr_in addr = {};
	socklen_t length = sizeof(addr);

	if ((sock = accept(web->sock, (struct sockaddr*)&addr, &length)) < 0)
		error("Error accepting peer");

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		error("Error getting slave flags");

	if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) < 0)
		error("Error setting slave non-blocking");

	/* Find a free client */
	slave_t *slave = 0;
	for (int i = 0; !slave && i < SLAVES; i++)
		if (!slaves[i].used)
			slave = &slaves[i];
	if (!slave)
		return;

	/* Setup client */
	slave->used       = 1;
	slave->sock       = sock;

	slave->peer.send  = web_send;
	slave->peer.data  = slave;

	slave->poll.ready = web_recv;
	slave->poll.data  = slave;
	slave->poll.sock  = sock;

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
