#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <sys/socket.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>

#ifdef USE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#endif
#ifdef USE_OPENSSL
#include <openssl/sha.h>
#endif

#include "util.h"
#include "http.h"

/* HTTP state */
typedef enum {
	MD_METHOD,
	MD_PATH,
	MD_VERSION,
	MD_FIELD,
	MD_VALUE,
	MD_POST,
	MD_SOCK,
} md_t;

/* Helper functions */
static void http_mode(http_t *http, int mode, char *buf)
{
	buf[http->idx] = '\0';
	http->idx      = 0;
	http->mode     = mode;
}

static int http_printf(int sock, const char *fmt, ...)
{
	static char buf[512];

	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	return send(sock, buf, len, MSG_DONTWAIT);
}

/* Parser callbacks */
static void http_open(http_t *http, const char *method,
		const char *path, const char *version)
{
	trace("  http_open:  %s [%s] [%s]", method, path, version);
}

static void http_head(http_t *http, const char *field, const char *value)
{
	static char hash[20] = {};
	static char extra[]  = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	trace("    http_head: %-20s -> [%s]", field, value);
	if (!strcasecmp(field, "Upgrade")) {
		http->hdr_upgrade = !!strcasestr(value, "websocket");
	}
	else if (!strcasecmp(field, "Connection")) {
		http->hdr_connect = !!strcasestr(value, "upgrade");
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
		http->hdr_accept = base64(hash, sizeof(hash),
			http->hdr_key, sizeof(http->hdr_key));
	}
}

static int http_body(http_t *http)
{
	trace("  http_body:  path=%s", http->req_path);
	if (!strcmp(http->req_path, "/")) {
		http_printf(http->sock,
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
	else if (!strcmp(http->req_path, "/socket")) {
		trace("  http_body:  socket - connect=%d upgrade=%d accept=%d\n"
		      "              key=%s",
		      http->hdr_connect, http->hdr_upgrade, http->hdr_accept, http->hdr_key);

		if (http->hdr_connect && http->hdr_upgrade && http->hdr_accept) {
			http_printf(http->sock,
				"HTTP/1.1 101 WebSocket Protocol Handshake\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: WebSocket\r\n"
				"Sec-WebSocket-Accept: %.*s\r\n"
				"\r\n",
				http->hdr_accept, http->hdr_key
			);
			return MD_SOCK;
		}
		else {
			http_printf(http->sock,
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
		http_printf(http->sock,
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
	return 0;
}

/* HTTP Functions */
int http_parse(http_t *http, char ch)
{
	int mode = 0;

	if (http->mode < MD_POST) {
		if (ch == '\r')
			return 0;
		if (ch == ' ' && http->idx == 0)
			return 0;
	}

	switch (http->mode) {
		/* HTTP Parsing */
		case MD_METHOD:
			if (ch == ' ')
				http_mode(http, MD_PATH, http->req_method);
			else if (http->idx < MAX_METHOD)
				http->req_method[http->idx++] = ch;
			return 0;

		case MD_PATH:
			if (ch == ' ')
				http_mode(http, MD_VERSION, http->req_path);
			else if (http->idx < MAX_PATH)
				http->req_path[http->idx++] = ch;
			return 0;

		case MD_VERSION:
			if (ch == '\n') {
				http_mode(http, MD_FIELD, http->req_version);
				http_open(http, http->req_method,
					      http->req_path,
					      http->req_version);
			}
			else if (http->idx < MAX_VERSION)
				http->req_version[http->idx++] = ch;
			return 0;

		case MD_FIELD:
			if (ch == '\n') {
				mode = http_body(http);
				http_mode(http, mode, http->hdr_field);
				switch (mode) {
					case MD_METHOD: return HTTP_DONE;
					case MD_SOCK:   return HTTP_SOCK;
					case MD_POST:   return HTTP_POST;
				}
			} else if (ch == ':')
				http_mode(http, MD_VALUE, http->hdr_field);
			else if (http->idx < MAX_FIELD)
				http->hdr_field[http->idx++] = ch;
			return 0;

		case MD_VALUE:
			if (ch == '\n') {
				http_mode(http, MD_FIELD, http->hdr_value);
				http_head(http, http->hdr_field, http->hdr_value);
			} else if (http->idx < MAX_VALUE)
				http->hdr_value[http->idx++] = ch;
			return 0;

		case MD_POST:
			return 0;

		case MD_SOCK:
			return 0;

		default:
			return 0;
	}
}
