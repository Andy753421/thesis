/* Functions */
void sock_init(const char *host, int port, int *fd);

int  sock_accept(void);

void sock_close(int fd);

void sock_exit(void);
