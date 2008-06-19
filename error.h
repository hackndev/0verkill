/* this code is stolen from the links browser, it was written by Mikulas Patocka */

#ifndef __ERROR_H
#define __ERROR_H


#include "cfg.h"
#include "config.h"

#define DUMMY ((void *)-1L)

void do_not_optimize_here(void *p);
void check_memory_leaks(void);
void error(unsigned char *, ...);
void debug_msg(unsigned char *, ...);
void int_error(unsigned char *, ...);
extern int errline;
extern unsigned char *errfile;

#ifdef HAVE_CALLOC
#define x_calloc(x) calloc((x), 1)
#else
static inline void *x_calloc(size_t x)
{
	void *p;
	if ((p = malloc(x))) memset(p, 0, x);
	return p;
}
#endif

#define internal errfile = __FILE__, errline = __LINE__, int_error
#define debug errfile = __FILE__, errline = __LINE__, debug_msg


#ifdef LEAK_DEBUG

void *debug_mem_alloc(unsigned char *, int, size_t);
void *debug_mem_calloc(unsigned char *, int, size_t);
void debug_mem_free(unsigned char *, int, void *);
void *debug_mem_realloc(unsigned char *, int, void *, size_t);
void set_mem_comment(void *, unsigned char *, int);

#define mem_alloc(x) debug_mem_alloc(__FILE__, __LINE__, x)
#define mem_calloc(x) debug_mem_calloc(__FILE__, __LINE__, x)
#define mem_free(x) debug_mem_free(__FILE__, __LINE__, x)
#define mem_realloc(x, y) debug_mem_realloc(__FILE__, __LINE__, x, y)

#else

static inline void *mem_alloc(size_t size)
{
	void *p;
	if (!size) return DUMMY;
	if (!(p = malloc(size))) {
		error("ERROR: out of memory (malloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void *mem_calloc(size_t size)
{
	void *p;
	if (!size) return DUMMY;
	if (!(p = x_calloc(size))) {
		error("ERROR: out of memory (calloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void mem_free(void *p)
{
	if (p == DUMMY) return;
	if (!p) {
		internal("mem_free(NULL)");
		return;
	}
	free(p);
}

static inline void *mem_realloc(void *p, size_t size)
{
	if (p == DUMMY) return mem_alloc(size);
	if (!p) {
		internal("mem_realloc(NULL, %d)", size);
		return NULL;
	}
	if (!size) {
		mem_free(p);
		return DUMMY;
	}
	if (!(p = realloc(p, size))) {
		error("ERROR: out of memory (realloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void *debug_mem_alloc(unsigned char *f, int l, size_t s) { f=f; l=l; return mem_alloc(s); }
static inline void *debug_mem_calloc(unsigned char *f, int l, size_t s) { f=f; l=l; return mem_calloc(s); }
static inline void debug_mem_free(unsigned char *f, int l, void *p) { f=f; l=l; mem_free(p); }
static inline void *debug_mem_realloc(unsigned char *f, int l, void *p, size_t s) { f=f; l=l; return mem_realloc(p, s); }
static inline void set_mem_comment(void *p, unsigned char *c, int l) {p=p; c=c; l=l;}

#endif



#endif /* __ERROR_H */
