/* Event types */
typedef enum {
	EVT_TIMEOUT,
	EVT_MASTER,
	EVT_SLAVE,
} evt_t;

/* Functions */
void poll_init(void);

void poll_add(int fd, int type, int data);

void poll_mod(int fd, int type, int data);

void poll_del(int fd);

int  poll_wait(int *data, int *events);

void poll_exit(void);
