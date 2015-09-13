typedef struct {
	void (*ready)(void *data);
	void  *data;
	int    sock;
} poll_t;

typedef struct peer_t {
	void (*send)(void *data, void *buf, int len);
	void  *data;
	struct peer_t *prev;
	struct peer_t *next;
} peer_t;

void poll_add(poll_t *data);
void poll_del(poll_t *data);

void peer_add(peer_t *peer);
void peer_del(peer_t *peer);
void peer_send(peer_t *peer, void *buf, int len);
