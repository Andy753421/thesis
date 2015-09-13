/* Helper Data */
extern int verbose;

/* Debug Functions */
void trace(const char *fmt, ...);

void debug(const char *fmt, ...);

void error(const char *str);

/* Data functions */
int base64(const void *in, int ilen, void *out, int olen);

unsigned short net16(unsigned short in);

unsigned long long net64(unsigned long long in);
