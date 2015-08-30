/* Macros */
#define alloc_get_ptr(alloc, buf) ({   \
	int id = alloc_get(alloc);     \
	id >= 0 ? &buf[id] : NULL;     \
})

#define alloc_del_ptr(alloc, buf, ptr) \
	alloc_del((alloc), (int)((ptr)-(buf)));

/* Alloc types */
typedef struct {
	int buf_i;
	int idx_i;
} idx_t;

typedef struct {
	idx_t *idx;
	int    size;
	int    used;
} alloc_t;

/* Helper Data */
extern int verbose;

/* Helper Functions */
void trace(const char *fmt, ...);

void debug(const char *fmt, ...);

void error(const char *str);

int base64(const void *in, int ilen, void *out, int olen);

unsigned short net16(unsigned short in);

unsigned long long net64(unsigned long long in);

/* Alloc Functions */
void alloc_init(alloc_t *alloc, idx_t *idx, int size);

int  alloc_get(alloc_t *alloc);

void alloc_del(alloc_t *alloc, int id);
