#define _POSIX_SOURCE
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>

#ifdef USE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#endif
#ifdef USE_OPENSSL
#include <openssl/sha.h>
#endif

#include <unistd.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "util.h"
#include "peer.h"

/* Constants */
#define MAX_METHOD  8
#define MAX_PATH    256
#define MAX_VERSION 8
#define MAX_FIELD   128
#define MAX_VALUE   128

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

/* Web Socket Types */
struct {
	uint8_t  opcode;
	uint64_t length;
	uint32_t masking;
	uint8_t  data[];
} mesg_t;

/* HTTP state */
typedef enum {
	MD_METHOD,
	MD_PATH,
	MD_VERSION,
	MD_FIELD,
	MD_VALUE,
	MD_BODY,
} md_t;

struct web_t {
	// Web socket stuff
	int   cache;
	int   sock;
	int   mode;
	int   idx;

	// Header attributes
	int   connect;
	int   upgrade;
	int   accept;
	char  key[28];

	// Parser strings
	char  method[MAX_METHOD+1];
	char  path[MAX_PATH+1];
	char  version[MAX_VERSION+1];
	char  field[MAX_FIELD+1];
	char  value[MAX_VALUE+1];
};

/* Local data */
static web_t   peers[WEB_PEERS];
static idx_t   cache[WEB_PEERS];
static alloc_t alloc;

/* Web helper functions */
static void web_mode(web_t *web, int mode, char *buf)
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
static void web_open(web_t *web, const char *method,
		const char *path, const char *version)
{
	debug("  Web Open: %s [%s] [%s]", method, path, version);
}

static void web_head(web_t *web, const char *field, const char *value)
{
	static char hash[20] = {};
	static char extra[]  = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	debug("    Web Head: %-20s -> [%s]", field, value);
	if (!strcasecmp(field, "Upgrade")) {
		web->upgrade = !!strcasestr(value, "websocket");
	}
	else if (!strcasecmp(field, "Connection")) {
		web->connect = !!strcasestr(value, "upgrade");
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
		web->accept = base64(hash, sizeof(hash),
			web->key, sizeof(web->key));
	}
}

static int web_body(web_t *web)
{
	debug("  Web Body: path=%s", web->path);
	if (!strcmp(web->path, "/")) {
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
	else if (!strcmp(web->path, "/socket")) {
		debug("  Web Sock: connect=%d upgrade=%d accept=%d\n"
		      "            key=%s",
		      web->connect, web->upgrade, web->accept, web->key);

		if (web->connect && web->upgrade && web->accept) {
			web_printf(web->sock,
				"HTTP/1.1 101 WebSocket Protocol Handshake\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: WebSocket\r\n"
				"Sec-WebSocket-Accept: %.*s\r\n"
				"\r\n",
				web->accept, web->key
			);
			return 0;
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

/* Web functions */
void web_init(void)
{
	alloc_init(&alloc, cache, WEB_PEERS);
}

web_t *web_add(int sock)
{
	web_t *web = alloc_get_ptr(&alloc, peers);
	if (web) {
		web->sock    = sock;
		web->mode    = 0;
		web->idx     = 0;
		web->connect = 0;
		web->upgrade = 0;
		web->accept  = 0;
	}
	return web;
}

void web_del(web_t *web)
{
	alloc_del_ptr(&alloc, peers, web);
}

int web_send(web_t *web, char *buf, int len)
{
	int     bytes    = 0;
	uint8_t head[10] = { WS_FINISH|WS_TEXT };

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

	if (write(web->sock, head, bytes) < 1)
		return -1;
	if (write(web->sock, buf, len) < 1)
		return -1;
	return 0;
}

int web_parse(web_t *web, char **buf, int *len)
{
	int status = 0;
	for (int i = 0; i < *len; i++) {
		int      ch = (*buf)[i];

		if (ch == '\r')
			continue;
		if (ch == ' ' && web->idx == 0)
			continue;

		switch (web->mode) {
			case MD_METHOD:
				if (ch == ' ')
					web_mode(web, MD_PATH, web->method);
				else if (web->idx < MAX_METHOD)
					web->method[web->idx++] = ch;
				break;

			case MD_PATH:
				if (ch == ' ')
					web_mode(web, MD_VERSION, web->path);
				else if (web->idx < MAX_PATH)
					web->path[web->idx++] = ch;
				break;

			case MD_VERSION:
				if (ch == '\n') {
					web_mode(web, MD_FIELD, web->version);
					web_open(web, web->method, web->path, web->version);
				}
				else if (web->idx < MAX_VERSION)
					web->version[web->idx++] = ch;
				break;

			case MD_FIELD:
				if (ch == '\n') {
					if ((status = web_body(web)) >= 0)
						web_mode(web, MD_BODY, web->field);
				} else if (ch == ':')
					web_mode(web, MD_VALUE, web->field);
				else if (web->idx < MAX_FIELD)
					web->field[web->idx++] = ch;
				break;

			case MD_VALUE:
				if (ch == '\n') {
					web_mode(web, MD_FIELD, web->value);
					web_head(web, web->field, web->value);
				} else if (web->idx < MAX_VALUE)
					web->value[web->idx++] = ch;
				break;

			case MD_BODY:
				debug("  Web Body: ...");
				break;
		}
	}
	return status;
}
