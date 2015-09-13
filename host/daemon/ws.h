/* Constants */
#define MAX_DATA    1500

/* Enums */
enum {
	WS_MORE, // Keep parsing
	WS_DONE, // Close the socket
};

/* Types */
typedef struct ws_t {
	// Common stuff
	int      sock;
	int      mode;
	int      idx;

	// WebSocket attributes
	uint8_t  ws_finish;
	uint8_t  ws_opcode;
	uint8_t  ws_masked;
	uint8_t  ws_mask[4];
	uint64_t ws_length;
	char     ws_data[MAX_DATA];
} ws_t;

/* Functions */
int ws_parse(ws_t *ws, char ch, peer_t  *peer);

int ws_send(ws_t *ws, void *buf, int len);
