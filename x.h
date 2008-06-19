#ifndef __X_H
#define __X_H

#include <X11/Xlib.h>
#include "cfg.h"

/* all four following are in characters */

/* minimal size of the window */
#define X_MIN_WIDTH 80  
#define X_MIN_HEIGHT 25

/* default size of the window */
#define DEFAULT_X_SIZE 80
#define DEFAULT_Y_SIZE 25


extern Display *display;
#ifdef TRI_D
extern Window window2;
#endif
extern Window window;
extern int x_width,x_height;  /* current width of the window (in characters) */
extern int FONT_X_SIZE;
extern int FONT_Y_SIZE;
extern char *x_font_name;
extern char *x_display_name;

#endif
