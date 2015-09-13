typedef struct tcp_t {
	int    sock;
	poll_t poll;
} tcp_t;

void tcp_server(tcp_t *tcp, const char *host, int port);
void tcp_close(tcp_t *tcp);
