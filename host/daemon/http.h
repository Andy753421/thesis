/* Constants */
#define MAX_METHOD  8
#define MAX_PATH    256
#define MAX_VERSION 8
#define MAX_FIELD   128
#define MAX_VALUE   128

/* Enums */
enum {
	HTTP_MORE, // Keep parsing
	HTTP_DONE, // Close the socket
	HTTP_POST, // Socket is a post request
	HTTP_SOCK, // Socket is a web socket
};

/* Types */
typedef struct http_t {
	// Common stuff
	int  sock;
	int  mode;
	int  idx;

	// Http Request
	char req_method[MAX_METHOD+1];
	char req_path[MAX_PATH+1];
	char req_version[MAX_VERSION+1];

	// Http Headers
	int  hdr_connect;
	int  hdr_upgrade;
	int  hdr_accept;
	char hdr_key[28];
	char hdr_field[MAX_FIELD+1];
	char hdr_value[MAX_VALUE+1];
} http_t;

/* Functions */
int http_parse(http_t *http, char ch);
