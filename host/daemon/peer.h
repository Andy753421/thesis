/* Constants */
#define NUM_TYPES 3

/* Types */
typedef enum {
	TYP_UDP,
	TYP_TCP,
	TYP_WEB,
} type_t;

/* Functions */
void peer_init(void);

int  peer_add(int fd, int type);

void peer_del(int id, int *old, int *new);

int  peer_send(int id);

int  peer_recv(int id);

void peer_exit(void);
