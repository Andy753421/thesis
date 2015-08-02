/* Functions */
void sock_bc(const char *host, int port, int *fd);

void sock_mc(const char *host, int port, int *fd);

void sock_tcp(const char *host, int port, int *fd);

int  sock_accept(int fd);

void sock_close(int fd);

void sock_exit(void);
