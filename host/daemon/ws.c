#define _GNU_SOURCE

#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "util.h"
#include "web.h"
#include "ws.h"

/* Constants */
#define MAX_DATA    1500

/* Web Sockets Flags */
#define OP_FINISH   0x80
#define OP_MASK     0x80

/* Web Socket Opcodes */
enum {
	OP_MORE   = 0x00,
	OP_TEXT   = 0x01,
	OP_BINARY = 0x02,
	OP_CLOSE  = 0x08,
	OP_PING   = 0x09,
	OP_PONG   = 0x0A,
} opcode_t;

/* HTTP state */
typedef enum {
	MD_OPCODE,
	MD_LENGTH,
	MD_EXTEND,
	MD_MASKED,
	MD_DATA,
} md_t;

/* Debug data */
static const char *str_opcode[0xF] = {
	[ 0] "more  ",
	[ 1] "text  ",
	[ 2] "binary",
	[ 8] "close ",
	[ 9] "ping  ",
	[10] "pong  ",
};

/* Web Socket Functions */
int ws_parse(ws_t *ws, char ch, peer_t *peer)
{
	switch (ws->mode) {
		case MD_OPCODE:
			trace("    ws_parse: opcode - fin=%d, op=%s",
					(ch&0x80) !=  0,
					str_opcode[ch&0x0F] ?: "[opcode error]");

			ws->ws_finish = (ch&0x80)!=0;
			ws->ws_opcode = (ch&0x0F);
			ws->mode      = MD_LENGTH;
			return 0;

		case MD_LENGTH:
			trace("    ws_parse: length - masked=%d, length=%d",
					(ch&0x80) != 0,
					(ch&0x7F));

			ws->ws_length = (ch&0x7F);
			ws->ws_masked = (ch&0x80)!=0;


			if (ws->ws_length == 126) {
				ws->ws_length = 0;
				ws->idx       = 2;
				ws->mode      = MD_EXTEND;
			}
			else if (ws->ws_length == 127) {
				ws->ws_length = 0;
				ws->idx       = 8;
				ws->mode      = MD_EXTEND;
			}
			else if (ws->ws_masked) {
				ws->mode      = MD_MASKED;
			}
			else {
				ws->mode      = MD_DATA;
			}
			return 0;

		case MD_EXTEND:
			trace("    ws_parse: extend - %d=%02hhx", ws->idx, ch);

			ws->idx       -= 1;
			ws->ws_length |= ((uint64_t)ch) << (ws->idx*8);

			if (ws->idx == 0) {
				if (ws->ws_masked)
					ws->mode = MD_MASKED;
				else
					ws->mode = MD_DATA;
			}
			return 0;

		case MD_MASKED:
			trace("    ws_parse: masked - %d=%02hhx", ws->idx, ch);

			ws->ws_mask[ws->idx++] = ch;
			if (ws->idx >= 4) {
				ws->idx  = 0;
				ws->mode = MD_DATA;
			}
			return 0;

		case MD_DATA:
			if (ws->idx < MAX_DATA) {
				uint8_t mask = ws->ws_mask[ws->idx%4];
				ws->ws_data[ws->idx] = ch ^ mask;
			}
			ws->idx++;
			if (ws->idx < ws->ws_length)
				return 0;

			trace("    ws_parse: data   - '%.*s'",
					ws->ws_length, ws->ws_data);
			ws->idx  = 0;
			ws->mode = MD_OPCODE;

			switch (ws->ws_opcode) {
				case OP_TEXT:
				case OP_BINARY:
					peer_send(peer, ws->ws_data, ws->ws_length);
					return 0;
				case OP_CLOSE:
					return WS_DONE;
			}

		default:
			return 0;
	}
}

/* Local functions */
int ws_send(ws_t *ws, void *buf, int len)
{
	int      bytes    = 0;
	uint8_t  head[10] = { OP_FINISH|OP_TEXT };

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

	if (write(ws->sock, head, bytes) != bytes)
		return 0;
	if (write(ws->sock, buf, len) != len)
		return 0;
	return len;
}
