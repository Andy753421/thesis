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

#include "util.h"
#include "peer.h"

/* Constants */
#define MAX_PEERS   (UDP_PEERS + TCP_PEERS + WEB_PEERS)
#define MAX_READ    1500

/* Local types data */
typedef struct {
	int   cache; // cache
	int   sock;  // peer socket
	int   type;  // peer type
	void *data;  // peer data
} peer_t;

/* Local data */
static peer_t  peers[MAX_PEERS];
static idx_t   cache[MAX_PEERS];
static alloc_t alloc;

/* Transmit / receive */
void peer_init(void)
{
	alloc_init(&alloc, cache, MAX_PEERS);
	udp_init();
	tcp_init();
	web_init();
}

int peer_add(int sock, int type)
{
	void *data = type==TYP_UDP ? (void*)udp_add(sock) :
		     type==TYP_TCP ? (void*)tcp_add(sock) :
		     type==TYP_WEB ? (void*)web_add(sock) : NULL;
	if (data == NULL) {
		debug("  Aborted -> %d[%d]", -1, sock);
		return -1;
	}

	int id = alloc_get(&alloc);
	peers[id].sock  = sock;
	peers[id].type  = type;
	peers[id].data  = data;
	debug("Opened peer -- %d[%d]", id, sock);
	return id;
}

int peer_del(int id)
{
	debug("Closed peer -- %d[%d]", id, peers[id].sock);

	switch (peers[id].type) {
		case TYP_UDP: udp_del(peers[id].data); break;
		case TYP_TCP: tcp_del(peers[id].data); break;
		case TYP_WEB: web_del(peers[id].data); break;
	}

	int sock = peers[id].sock;
	alloc_del(&alloc, id);
	return sock;
}

int peer_get(int ii)
{
	return cache[ii].buf_i;
}

int peer_recv(int id, char **buf, int *len)
{
	static char rbuf[MAX_READ];
	static int  rlen;

	int   sock = peers[id].sock;
	int   type = peers[id].type;
	void *data = peers[id].data;

	if ((rlen = recvfrom(sock, rbuf, sizeof(rbuf), 0, NULL, NULL)) <= 0) {
		debug("  Dead %-4d bytes <- %d[%d]", rlen, id, sock);
		return -1;
	}

	// Update buffer
	int cnt = alloc.used;
	*buf = rbuf;
	*len = rlen;

	// Send to peer
	int status = type==TYP_UDP ? udp_parse(data, buf, len) :
		     type==TYP_TCP ? tcp_parse(data, buf, len) :
		     type==TYP_WEB ? web_parse(data, buf, len) : 0;
	if (status < 0) {
		debug("  Closed %d <~ %d[%d]", status, id, sock);
		return status;
	}
	else if (status == 0) {
		debug("  Waiting %d status <~ %d[%d]", *len, id, sock);
		return status;
	}

	// Return;
	debug("  Read %-4d bytes <- %d[%d]", *len, id, sock);
	return cnt;
}

int peer_send(int id, char *buf, int len)
{
	int   sock = peers[id].sock;
	int   type = peers[id].type;
	void *data = peers[id].data;

	//if (src == dst) {
	//	debug("  From %d bytes <- %d[%d]", len, id, dst);
	//	return 0;
	//}

	int status = type==TYP_UDP ? udp_send(data, buf, len) :
		     type==TYP_TCP ? tcp_send(data, buf, len) :
		     type==TYP_WEB ? web_send(data, buf, len) : 0;
	if (status < 0) {
		debug("  Dead %-4d bytes -> %d[%d]", len, id, sock);
		return status;
	}

	debug("  Sent %-4d bytes -> %d[%d]", len, id, sock);
	return status;
}

void peer_exit(void)
{
	for (int ii = 0; ii < alloc.used; ii++) {
		int id   = cache[id].buf_i;
		int sock = peers[id].sock;
		shutdown(sock, SHUT_RDWR);
		close(sock);
	}
}
