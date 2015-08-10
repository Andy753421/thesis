#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

/* Global Data */
int verbose = 1;

/* Helper Functions */
void debug(const char *fmt, ...)
{
	va_list ap;
	if (verbose) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		fprintf(stdout, "\n");
		va_end(ap);
	}
}

void error(const char *str)
{
	perror(str);
	exit(-1);
}

int base64(const void *_in, int ilen, void *_out, int olen)
{
	static const char table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	const unsigned char *in  = _in;
	unsigned char       *out = _out;

	int val = 0;
	int len = ((ilen-1)/3+1)*4;

	if (olen < len)
		return 0;

	for (int i=0; i<ilen/3; i++) {
		val      = *(in++) << 020;
		val     |= *(in++) << 010;
		val     |= *(in++) << 000;
		*(out++) = table[(val>>6*3)&077];
		*(out++) = table[(val>>6*2)&077];
		*(out++) = table[(val>>6*1)&077];
		*(out++) = table[(val>>6*0)&077];
	}

	switch (ilen%3) {
		case 2:
			val    = *(in++)<<020;
			val   |= *(in++)<<010;
			*out++ = table[(val>>6*3)&077];
			*out++ = table[(val>>6*2)&077];
			*out++ = table[(val>>6*1)&077];
			*out++ = '=';
			break;
		case 1:
			val    = *(in++)<<020;
			*out++ = table[(val>>6*3)&077];
			*out++ = table[(val>>6*2)&077];
			*out++ = '=';
			*out++ = '=';
			break;
	}

	return len;
}

/* Alloc Functions */
void alloc_init(alloc_t *alloc, idx_t *idx, int size)
{
	alloc->idx  = idx;
	alloc->size = size;
	alloc->used = 0;
	for (int i = 0; i < size; i++) {
		idx[i].buf_i = i;
		idx[i].idx_i = i;
	}
}

int alloc_get(alloc_t *alloc)
{
	if (alloc->used >= alloc->size)
		return -1;

	int idx_i = alloc->used++;
	int buf_i = alloc->idx[idx_i].buf_i;

	return buf_i;
}

void alloc_del(alloc_t *alloc, int id)
{
	int del_buf_i = id;
	int del_idx_i = alloc->idx[del_buf_i].idx_i;

	int cur_idx_i = alloc->used - 1;
	int cur_buf_i = alloc->idx[cur_idx_i].buf_i;

	// Move current buffer into deleted slot
	alloc->idx[cur_buf_i].idx_i = del_idx_i;
	alloc->idx[del_idx_i].buf_i = cur_buf_i;

	// Move deleted buffer into last position
	alloc->idx[del_buf_i].idx_i = cur_idx_i;
	alloc->idx[cur_idx_i].buf_i = del_buf_i;

	alloc->used--;
}
