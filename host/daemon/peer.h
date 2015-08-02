/* Types */
typedef enum {
	CMD_NONE,
	CMD_WRITE,
	CMD_CLOSE,
} cmd_t;

typedef enum {
	TYP_RAW,
	TYP_WS,
} typ_t;

/* Functions */
void peer_init(void);

int  peer_add(int fd);

void peer_del(int id, int *old, int *new);

int  peer_send(int id);

int  peer_recv(int id);

void peer_exit(void);
