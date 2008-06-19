/* X INTERFACE */

#include "config.h"

#ifdef HAVE_LIBXPM
	#define ICON
#endif
	
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>

#ifdef ICON
	#include <X11/xpm.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "cfg.h"
#include "kbd.h"
#include "x.h"

#ifdef ICON
	#include "icon.h"
#endif


#define BORDER_WIDTH 4


int console_ok=1;

Display *display;
char * x_display_name;
Window window;
#ifdef TRI_D
	Window window2;
#endif

int FONT_X_SIZE,FONT_Y_SIZE;
char *x_font_name;

GC gc;
Atom x_delete_window_atom,x_wm_protocols_atom;

char * x_color_name[16]={
	"black",
	"red4",
	"green4",
	"brown4",
	"blue4",
	"dark violet",
	"cyan4",
	"gray40",
	"gray20",
	"red1",
	"green1",
	"yellow",
	"blue1",
	"magenta1",
	"cyan1",
	"white"
};
XColor x_color[16];

int x_screen;
int x_current_x,x_current_y;
int x_current_color=0;
int x_current_bgcolor=0;
int x_width=DEFAULT_X_SIZE,x_height=DEFAULT_Y_SIZE;
int display_height,display_width;
XFontStruct* x_font;

XGCValues gcv;

#ifdef ICON
	Pixmap icon_pixmap,icon_mask;
#endif

void c_refresh(void)
{
	XFlush(display);
}


/* initialize console */
void c_init(int w,int h)
{
	XColor dummy;
	XSizeHints size_hints;

#ifdef ICON
	XWMHints wm_hints;
#endif
	int a;
	char *fontname=x_font_name?x_font_name:DEFAULT_FONT_NAME;
	
	w=w;h=h;

	if (!x_display_name) x_display_name = getenv("DISPLAY");
	
	/* initiate connection with the X server */
        display=XOpenDisplay(x_display_name);
        if (!display){fprintf(stderr,"Error: Can't open display %s\n",x_display_name);exit(1);}
        
	x_screen=DefaultScreen(display);
        
	display_height=DisplayHeight(display,x_screen);
        display_width=DisplayWidth(display,x_screen);
	
	/* load font */
	x_font=XLoadQueryFont(display,fontname);
	if (!x_font){fprintf(stderr,"Error: Can't load font \"%s\".\n",fontname);XCloseDisplay(display);exit(1);}
	FONT_X_SIZE=x_font->max_bounds.width;
	FONT_Y_SIZE=x_font->max_bounds.ascent+x_font->max_bounds.descent;

	x_delete_window_atom=XInternAtom(display,"WM_DELETE_WINDOW", False);
	x_wm_protocols_atom=XInternAtom(display,"WM_PROTOCOLS", False);

	/* create window and set properties */
        window=XCreateSimpleWindow(display,RootWindow(display,x_screen),0,0,x_width*FONT_X_SIZE,x_height*FONT_Y_SIZE,BORDER_WIDTH,BlackPixel(display,x_screen),BlackPixel(display,x_screen));
        if (!window){fprintf(stderr,"Error: Can't create window.\n");XCloseDisplay(display);exit(1);}
        XStoreName(display,window,"0verkill");
	
#ifdef TRI_D
	if (TRI_D_ON)
	{
	        window2=XCreateSimpleWindow(display,RootWindow(display,x_screen),0,0,x_width*FONT_X_SIZE,x_height*FONT_Y_SIZE,BORDER_WIDTH,BlackPixel(display,x_screen),BlackPixel(display,x_screen));
        	if (!window2){fprintf(stderr,"Error: Can't create window.\n");XCloseDisplay(display);exit(1);}
	        XStoreName(display,window2,"0verkill 3D");
	}
#endif
	
	size_hints.min_width=X_MIN_WIDTH*FONT_X_SIZE;
	size_hints.min_height=X_MIN_HEIGHT*FONT_Y_SIZE;
	size_hints.width_inc=FONT_X_SIZE;
	size_hints.height_inc=FONT_Y_SIZE;
	size_hints.flags=PMinSize|PResizeInc;

	XSetNormalHints(display,window,&size_hints);

#ifdef TRI_D
	if (TRI_D_ON)
		XSetNormalHints(display,window2,&size_hints);
#endif

#ifdef ICON
	XCreatePixmapFromData(display,window,icon_xpm,&icon_pixmap,&icon_mask,0);
#ifdef TRI_D
	if (TRI_D_ON)
		XCreatePixmapFromData(display,window2,icon_xpm,&icon_pixmap,&icon_mask,0);
#endif
	wm_hints.flags=IconPixmapHint|IconMaskHint;
	wm_hints.icon_pixmap=icon_pixmap;
	wm_hints.icon_mask=icon_mask;

	XSetWMHints(display,window,&wm_hints);
#ifdef TRI_D
	if (TRI_D_ON)
		XSetWMHints(display,window2,&wm_hints);
#endif
#endif
	
	XSetWMProtocols(display,window,&x_delete_window_atom,1);
        XSelectInput(display,window,ExposureMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		XSetWMProtocols(display,window2,&x_delete_window_atom,1);
        	XSelectInput(display,window2,ExposureMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask);
	}
#endif

	/* create 16 graphic contexts - one for each color */
	gcv.font=x_font->fid;
	gcv.foreground=BlackPixel(display,x_screen);
	gcv.background=BlackPixel(display,x_screen);
        gc=XCreateGC(display,window,GCFont+GCBackground,&gcv);
	for (a=0;a<16;a++)
		XAllocNamedColor(display,DefaultColormap(display,x_screen),x_color_name[a],x_color+a,&dummy);
	c_refresh();

	/* map window */
        XMapWindow(display,window);

#ifdef TRI_D
	if (TRI_D_ON)
	        XMapWindow(display,window2);
#endif
	
	c_refresh();

	/* initialize keyboard */
	kbd_init();

	console_ok=0;
}


/* close console */
void c_shutdown(void)
{
	kbd_close();
	XDestroyWindow(display,window);
#ifdef TRI_D
	if (TRI_D_ON)
		XDestroyWindow(display,window2);
#endif
	XFreeFont(display,x_font);
#ifdef ICON
	XFreePixmap(display,icon_pixmap);
	XFreePixmap(display,icon_mask);
#endif
	XCloseDisplay(display);
	console_ok=1;
}


/* move cursor to [x,y] */
void c_goto(int x,int y)
{
	x_current_x=x;
	x_current_y=y;
}


/* set foreground color */
void c_setcolor(unsigned char a)
{
	x_current_color=(a&15);
}


/* set foreground and background color */
void c_setcolor_bg(unsigned char a,unsigned char b)
{
	x_current_color=(a&15);
	x_current_bgcolor=(b&7);
}


/* set background color */
void c_setbgcolor(unsigned char a)
{
	x_current_bgcolor=a&7;
}


/* set highlight color and background */
void c_sethlt_bg(unsigned char a,unsigned char b)
{
	x_current_color=(x_current_color&7)|(!!a)<<3;
	x_current_bgcolor=b&7;
}


/* set highlight color */
void c_sethlt(unsigned char a)
{
	x_current_color=(x_current_color&7)|(!!a)<<3;
}


/* set 3 bit foreground color and background color */
void c_setcolor_3b_bg(unsigned char a,unsigned char b)
{
	x_current_color=(x_current_color&8)|(a&7);
	x_current_bgcolor=b&7;
}


/* set 3 bit foreground color */
void c_setcolor_3b(unsigned char a)
{
	x_current_color=(x_current_color&8)|(a&7);
}


/* print on the cursor position */
void c_print(char * text)
{
	int l=strlen(text);

	XSetForeground(display,gc,(x_color[x_current_color]).pixel);
	XSetBackground(display,gc,(x_color[x_current_bgcolor]).pixel);
	XDrawImageString(
		display,
#ifdef TRI_D
		(TRI_D_ON&&tri_d)?window2:window,
#else
		window,
#endif
		gc,
		x_current_x*FONT_X_SIZE,
		(x_current_y+1)*FONT_Y_SIZE,
		text,
		l
	);
	x_current_x+=l;
}


/* print char on the cursor position */
void c_putc(char c)
{
	char s[2]={c,0};
	c_print(s);
}


/* clear the screen */
void c_cls(void)
{
	XClearWindow(display,window);
#ifdef TRI_D
	if (TRI_D_ON)
		XClearWindow(display,window2);
#endif
}


/* clear rectangle on the screen */
/* presumtions: x2>=x1 && y2>=y1 */
void c_clear(int x1,int y1,int x2,int y2)
{
	int w=x2-x1+1;
	int h=y2-y1+1;
	
	XClearArea(display,window,x1*FONT_X_SIZE,y1*FONT_Y_SIZE,w*FONT_X_SIZE,h*FONT_Y_SIZE,False);
#ifdef TRI_D
	if (TRI_D_ON)
		XClearArea(display,window2,x1*FONT_X_SIZE,y1*FONT_Y_SIZE,w*FONT_X_SIZE,h*FONT_Y_SIZE,False);
#endif
}


void c_update_kbd(void)
{
	kbd_update();
}


int c_pressed(int k)
{
	return kbd_is_pressed(k);
}


int c_was_pressed(int k)
{
	return kbd_was_pressed(k);
}


void c_wait_for_key(void)
{
	kbd_wait_for_key();
}


/* set cursor shape */
void c_cursor(int type)
{
	type=type;
}


/* ring the bell */
void c_bell(void)
{
	XBell(display,100);
}


/* get screen dimensions */
void c_get_size(int *x, int *y)
{
	(*x)=x_width;
	(*y)=x_height;
}
