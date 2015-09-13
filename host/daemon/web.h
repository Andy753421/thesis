typedef struct web_t {
	int    sock;
	poll_t poll;
} web_t;

void web_server(web_t *web, const char *host, int port);
void web_close(web_t *web);
