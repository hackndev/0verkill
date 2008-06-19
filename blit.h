#ifndef __BLIT_H
#define __BLIT_H

#include "cfg.h"

#ifndef WIN32
#include "config.h"
#endif

/* screen dimensions */
extern int SCREEN_X,SCREEN_Y;

/* hero's offset on the screen */
extern int SCREEN_XOFFSET,SCREEN_YOFFSET;

/* screen buffer (attributes and pixels) */
extern unsigned char *screen_a,*screen;
#ifdef TRI_D
extern unsigned char *screen2_a,*screen2;
#endif
/* previous screen buffer (attributes and pixels) */
extern unsigned char *screen_a_old,*screen_old;
#ifdef TRI_D
extern unsigned char *screen2_a_old,*screen2_old;
#endif


/* flush screenbuffer to the screen */
#ifdef HAVE_INLINE
	extern inline void 
#else
	extern void
#endif
blit_screen(unsigned char ignore_bg);

/* clear screen buffer */
extern void clear_screen(void);
extern void init_blit(void);
extern void shutdown_blit(void);
extern void redraw_screen(void);
/* resize and clear screenbuffers */
extern void resize_screen(void);
/* print text on given position and with given color into screenbuffer  */
extern void print2screen(int x,int y,unsigned char color,char* message);
/* draw frame into screen buffer */
extern void draw_frame(int x,int y,int w,int h,unsigned char color);

#endif
