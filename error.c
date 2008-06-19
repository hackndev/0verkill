/* this code is stolen from the links browser, it was written by Mikulas Patocka */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "config.h"
#include "cfg.h"
#include "error.h"

struct memory_list {
	int n;
	void *p[1];
};

struct list_head {
	void *next;
	void *prev;
};

#ifndef HAVE_TYPEOF

struct xlist_head {
	struct xlist_head *next;
	struct xlist_head *prev;
};

#endif


#define del_from_list(x) {((struct list_head *)(x)->next)->prev=(x)->prev; ((struct list_head *)(x)->prev)->next=(x)->next;}
#define add_at_pos(p,x) do {(x)->next=(p)->next; (x)->prev=(p); (p)->next=(x); (x)->next->prev=(x);} while(0)

#ifdef HAVE_TYPEOF
	#define foreach(e,l) for ((e)=(l).next; (e)!=(typeof(e))&(l); (e)=(e)->next)
	#define add_to_list(l,x) add_at_pos((typeof(x))&(l),(x))
#else
	#define foreach(e,l) for ((e)=(l).next; (e)!=(void *)&(l); (e)=(e)->next)
	#define add_to_list(l,x) add_at_pos((struct xlist_head *)&(l),(struct xlist_head *)(x))
#endif


/* inline */

#ifdef LEAK_DEBUG

extern long mem_amount;
extern long last_mem_amount;

#define ALLOC_MAGIC		0xa110c
#define ALLOC_FREE_MAGIC	0xf4ee
#define ALLOC_REALLOC_MAGIC	0x4ea110c

#ifndef LEAK_DEBUG_LIST
struct alloc_header {
	int magic;
	int size;
};
#else
struct alloc_header {
	struct alloc_header *next;
	struct alloc_header *prev;
	int magic;
	int size;
	int line;
	unsigned char *file;
	unsigned char *comment;
};
#endif

#define L_D_S ((sizeof(struct alloc_header) + 7) & ~7)

#endif

/*-----------------------------------------------------------------------------*/

void do_not_optimize_here(void *p)
{
	p=p;
	/* stop GCC optimization - avoid bugs in it */
}

#ifdef LEAK_DEBUG
long mem_amount = 0;
long last_mem_amount = -1;
#ifdef LEAK_DEBUG_LIST
struct list_head memory_list = { &memory_list, &memory_list };
#endif
#endif

static inline void force_dump(void)
{
	fprintf(stderr, "\n\033[1m%s\033[0m\n", "Forcing core dump");
	fflush(stderr);
	raise(SIGSEGV);
}

void check_memory_leaks(void)
{
#ifdef LEAK_DEBUG
	if (mem_amount) {
		fprintf(stderr, "\n\033[1mMemory leak by %ld bytes\033[0m\n", mem_amount);
#ifdef LEAK_DEBUG_LIST
		fprintf(stderr, "\nList of blocks: ");
		{
			int r = 0;
			struct alloc_header *ah;
			foreach (ah, memory_list) {
				fprintf(stderr, "%s%p:%d @ %s:%d", r ? ", ": "", (char *)ah + L_D_S, ah->size, ah->file, ah->line), r = 1;
				if (ah->comment) fprintf(stderr, ":\"%s\"", ah->comment);
			}
			fprintf(stderr, "\n");
		}
#endif
		force_dump();
	}
#endif
}

void er(int b, unsigned char *m, va_list l)
{
	if (b) fprintf(stderr, "%c", (char)7);
	vfprintf(stderr, m, l);
	fprintf(stderr, "\n");
	sleep(1);
}

void error(unsigned char *m, ...)
{
	va_list l;
	va_start(l, m);
	er(1, m, l);
}

int errline;
unsigned char *errfile;

unsigned char errbuf[4096];

void int_error(unsigned char *m, ...)
{
	va_list l;
	va_start(l, m);
	sprintf(errbuf, "\033[1mINTERNAL ERROR\033[0m at %s:%d: ", errfile, errline);
	strcat(errbuf, m);
	er(1, errbuf, l);
	force_dump();
}

void debug_msg(unsigned char *m, ...)
{
	va_list l;
	va_start(l, m);
	sprintf(errbuf, "DEBUG MESSAGE at %s:%d: ", errfile, errline);
	strcat(errbuf, m);
	er(0, errbuf, l);
}

#ifdef LEAK_DEBUG

void *debug_mem_alloc(unsigned char *file, int line, size_t size)
{
	void *p;
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (!size) return DUMMY;
#ifdef LEAK_DEBUG
	mem_amount += size;
	size += L_D_S;
#endif
	if (!(p = malloc(size))) {
		error("ERROR: out of memory (malloc returned NULL)\n");
		return NULL;
	}
#ifdef LEAK_DEBUG
	ah = p;
	p = (char *)p + L_D_S;
	ah->size = size - L_D_S;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->file = file;
	ah->line = line;
	ah->comment = NULL;
	add_to_list(memory_list, ah);
#endif
#endif
	memset(p, 'A', size - L_D_S);
	return p;
}

void *debug_mem_calloc(unsigned char *file, int line, size_t size)
{
	void *p;
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (!size) return DUMMY;
#ifdef LEAK_DEBUG
	mem_amount += size;
	size += L_D_S;
#endif
	if (!(p = x_calloc(size))) {
		error("ERROR: out of memory (calloc returned NULL)\n");
		return NULL;
	}
#ifdef LEAK_DEBUG
	ah = p;
	p = (char *)p + L_D_S;
	ah->size = size - L_D_S;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->file = file;
	ah->line = line;
	ah->comment = NULL;
	add_to_list(memory_list, ah);
#endif
#endif
	return p;
}

void debug_mem_free(unsigned char *file, int line, void *p)
{
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (p == DUMMY) return;
	if (!p) {
		errfile = file, errline = line, int_error("mem_free(NULL)");
		return;
	}
#ifdef LEAK_DEBUG
	p = (char *)p - L_D_S;
	ah = p;
	if (ah->magic != ALLOC_MAGIC) {
		errfile = file, errline = line, int_error("mem_free: magic doesn't match: %08x", ah->magic);
		return;
	}
	ah->magic = ALLOC_FREE_MAGIC;
#ifdef LEAK_DEBUG_LIST
	del_from_list(ah);
	if (ah->comment) free(ah->comment);
#endif
	mem_amount -= ah->size;
#endif
	free(p);
}

void *debug_mem_realloc(unsigned char *file, int line, void *p, size_t size)
{
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (p == DUMMY) return debug_mem_alloc(file, line, size);
	if (!p) {
		errfile = file, errline = line, int_error("mem_realloc(NULL, %d)", size);
		return NULL;
	}
	if (!size) {
		debug_mem_free(file, line, p);
		return DUMMY;
	}
#ifdef LEAK_DEBUG
	p = (char *)p - L_D_S;
	ah = p;
	if (ah->magic != ALLOC_MAGIC) {
		errfile = file, errline = line, int_error("mem_realloc: magic doesn't match: %08x", ah->magic);
		return NULL;
	}
	ah->magic = ALLOC_REALLOC_MAGIC;
#endif
	if (!(p = realloc(p, size + L_D_S))) {
		error("ERROR: out of memory (realloc returned NULL)\n");
		return NULL;
	}
#ifdef LEAK_DEBUG
	ah = p;
	mem_amount += size - ah->size;
	ah->size = size;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->prev->next = ah;
	ah->next->prev = ah;
#endif
#endif
	return (char *)p + L_D_S;
}

void set_mem_comment(void *p, unsigned char *c, int l)
{
#ifdef LEAK_DEBUG_LIST
	struct alloc_header *ah = (struct alloc_header *)((char *)p - L_D_S);
	if (ah->comment) free(ah->comment);
	if ((ah->comment = malloc(l + 1))) memcpy(ah->comment, c, l), ah->comment[l] = 0;
#endif
}

#endif

