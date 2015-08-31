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
	uint8_t  finish;
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

	MD_OPCODE,
	MD_LENGTH,
	MD_EXTEND,
	MD_MASKED,
	MD_DATA,
} md_t;

struct web_t {
	// Common stuff
	int   cache;
	int   sock;
	int   mode;
	int   idx;

	// Http Request
	char  req_method[MAX_METHOD+1];
	char  req_path[MAX_PATH+1];
	char  req_version[MAX_VERSION+1];

	// Http Headers
	int   hdr_connect;
	int   hdr_upgrade;
	int   hdr_accept;
	char  hdr_key[28];
	char  hdr_field[MAX_FIELD+1];
	char  hdr_value[MAX_VALUE+1];

	// WebSocket attributes
	uint8_t  ws_finish;
	uint8_t  ws_opcode;
	uint8_t  ws_masked;
	uint8_t  ws_mask[4];
	uint64_t ws_length;
};

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
	trace("  web_open:  %s [%s] [%s]", method, path, version);
}

static void web_head(web_t *web, const char *field, const char *value)
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

static int web_body(web_t *web)
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

/* Web functions */
void web_init(void)
{
	alloc_init(&alloc, cache, WEB_PEERS);
}

web_t *web_add(int sock)
{
	web_t *web = alloc_get_ptr(&alloc, peers);
	if (web) {
		web->sock        = sock;
		web->mode        = 0;
		web->idx         = 0;

		web->hdr_connect = 0;
		web->hdr_upgrade = 0;
		web->hdr_accept  = 0;

		web->ws_finish   = 0;
		web->ws_opcode   = 0;
		web->ws_masked   = 0;
		web->ws_length   = 0;
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

	if (web->mode < MD_OPCODE)
		return 0;

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
	trace("  web_parse: %s", str_mode[web->mode]);

	int mode = 0, status = 0;
	for (int i = 0; i < *len; i++) {
		uint8_t ch = (*buf)[i];

		if (web->mode < MD_BODY) {
			if (ch == '\r')
				continue;
			if (ch == ' ' && web->idx == 0)
				continue;
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
				*buf = &(*buf)[i];
				*len = (*len)-i;

				for (int i = 0; i < *len; i++)
					(*buf)[i] ^= web->ws_mask[i%4];

				web->idx  = 0;
				web->mode = MD_OPCODE;

				if (*len != web->ws_length) {
					trace("    web_parse: data   - [length error: got %d]",
							*len);
					return 0;
				} else {
					trace("    web_parse: data   - '%.*s'",
							*len, *buf);
					return 1;
				}
		}
	}
	return status;
}
