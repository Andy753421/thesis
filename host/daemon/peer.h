/* Constants */
#define NUM_TYPES 3

/* Peer constants */
#define TCP_PEERS 100
#define UDP_PEERS 100
#define WEB_PEERS 100

/* Types */
typedef enum {
	TYP_UDP,
	TYP_TCP,
	TYP_WEB,
} type_t;

/* Peer types */
typedef struct udp_t udp_t;
typedef struct tcp_t tcp_t;
typedef struct web_t web_t;

/* Functions */
void peer_init(void);

// Create new peer - returns id
int  peer_add(int sock, int type);

// Delete peer - returns sock
int  peer_del(int id);

// Get peer id - returns id
int  peer_get(int ii);

// Send data to peer - return count
int  peer_send(int id, char *buf, int len);

// Send data to peer - return send count
int  peer_recv(int id, char **buf, int *len);

// Close all peers
void peer_exit(void);

/* Peer functions */
void udp_init(void);
void tcp_init(void);
void web_init(void);

udp_t *udp_add(int sock);
tcp_t *tcp_add(int sock);
web_t *web_add(int sock);

void udp_del(udp_t *udp);
void tcp_del(tcp_t *tcp);
void web_del(web_t *web);

int udp_send(udp_t *udp, char *buf, int len);
int tcp_send(tcp_t *tcp, char *buf, int len);
int web_send(web_t *web, char *buf, int len);

int udp_parse(udp_t *udp, char **buf, int *len);
int tcp_parse(tcp_t *tcp, char **buf, int *len);
int web_parse(web_t *web, char **buf, int *len);

/* Swap */
static inline void swap2(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b =  t;
}

static inline void swap4(int *cache, int *a, int *b)
{
	swap2(&cache[*a], &cache[*b]);
	swap2(a, b);
}
