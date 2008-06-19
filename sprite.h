#ifndef __SPRITE_H
#define __SPRITE_H

#ifndef WIN32
#include "config.h"
#endif

/* one scanline of bitmap */
struct line
{
	int len;
	unsigned char *attr;
	unsigned char *bmp;
};


/* one sprite animation position */
struct pos
{
	int xo,yo;
	int n;
	struct line *lines;
};


/* sprite structure */
struct sprite
{
	int n_positions; /* Number of defined positions */
	struct pos *positions;
	int n_steps;
	unsigned short *steps;
};


/* load sprite from a file */
extern void load_sprite(char *,struct sprite *);
/* put sprite on given position */
#ifdef HAVE_INLINE
	extern inline void
#else
	extern void
#endif
put_sprite(int,int,struct pos *,unsigned char);

/* put image into given memory */
#ifdef HAVE_INLINE
	extern inline void 
#else
	extern void
#endif
_put_sprite(int,int,unsigned char *,unsigned char *,int,int,struct pos *,unsigned char,unsigned char);

/* copy window of static map visible by player into screenbuffer */
/* accepts x and y coordinate of upper left corner of the window */
extern void show_window(int x,int y);
/* initalize sprites */
extern void init_sprites(void);
extern void shutdown_sprites(void);
extern void free_sprite(struct sprite *);

#endif
