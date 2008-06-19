/* CONSOLE INTERFACE */

#ifndef WIN32

#include <ctype.h>
#include <termios.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "cfg.h"
#include "console.h"
#include "kbd.h"
#include "error.h"

#define SCREEN_BUFFERING


/* ALL COORDINATES START FROM 0 */

int console_ok=1;
struct termios term_setting;
int screen_width;


#ifdef SCREEN_BUFFERING
char *screen_buffer;
int screen_buffer_pos=0;
int screen_buffer_size;
#endif

#ifdef HAVE_INLINE
inline void
#else
void
#endif
my_putc(char c)
{

#ifdef SCREEN_BUFFERING
	if (screen_buffer_pos+1>screen_buffer_size)
	{
		screen_buffer_size<<=1;
		screen_buffer=mem_realloc(screen_buffer,screen_buffer_size);
		if (!screen_buffer){c_shutdown();fprintf(stderr,"Error: Can't reallocate screen buffer.\n");EXIT(1);}
	}
	screen_buffer[screen_buffer_pos]=c;
	screen_buffer_pos++;
#else
	fputc(c,stdout);
#endif
}


#ifdef HAVE_INLINE
inline void
#else
void
#endif
my_print(char *str)
{
#ifdef SCREEN_BUFFERING
	int l=strlen(str);

	if (screen_buffer_pos+l>screen_buffer_size)
	{
		while(screen_buffer_pos+l>screen_buffer_size)
			screen_buffer_size<<=1;
		screen_buffer=mem_realloc(screen_buffer,screen_buffer_size);
		if (!screen_buffer){c_shutdown();fprintf(stderr,"Error: Can't reallocate screen buffer.\n");EXIT(1);}
	}
	memcpy(screen_buffer+screen_buffer_pos,str,l);
	screen_buffer_pos+=l;
#else
	fputs(str,stdout);
#endif
}


void c_refresh(void)
{
#ifdef SCREEN_BUFFERING
	write(1,screen_buffer,screen_buffer_pos);
	screen_buffer_pos=0;
#else
	fflush(stdout);
#endif
}


/* initialize console */
void c_init(int w,int h)
{
	struct termios t;

	console_ok=0;

#ifdef SCREEN_BUFFERING
	screen_buffer_size=w*h;
	if (!(screen_buffer=mem_alloc(screen_buffer_size))){fprintf(stderr,"Error: Not enough memory for screen buffer.\n");EXIT(1);}
#else
	w=w;h=h;
#endif
	
	tcgetattr(0,&term_setting);
	memcpy(&t, &term_setting, sizeof(struct termios));
	t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	t.c_oflag &= ~OPOST;
	t.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	t.c_cflag &= ~(CSIZE|PARENB);
	t.c_cflag |= CS8;

	tcsetattr(0, TCSANOW, &t);
	kbd_init();

	c_cls();
	my_print("\033[;H");
	c_refresh();
}


/* close console */
void c_shutdown(void)
{
#ifdef SCREEN_BUFFERING
	mem_free(screen_buffer);
#endif

	kbd_close();
	tcsetattr(0,TCSANOW,&term_setting);
	c_cursor(C_NORMAL);
	c_setcolor(7);
	c_cls();
	my_print("\033[;H");
	c_refresh();
	console_ok=1;
}


/* move cursor to [x,y] */
void c_goto(int x,int y)
{
	char txt[16];

#ifdef TRI_D
	if (TRI_D_ON&&tri_d)x+=screen_width;
#endif
	snprintf(txt,16,"\033[%d;%dH",y+1,x+1);
	my_print(txt);
}


/* set complete foreground color */
void c_setcolor(unsigned char a)
{
	char txt[16];
	a&=15;
	snprintf(txt,16,"\033[%c;%dm",(a>>3)+'0',30+(a&7));
	my_print(txt);
}


/* set complete foreground color and background color */
void c_setcolor_bg(unsigned char fg,unsigned char bg)
{
	char txt[16];
	fg&=15;
	if (!(fg>>3)&&!bg) snprintf(txt,16,"\033[%c;%dm",(fg>>3)+'0',30+(fg&7));
	else snprintf(txt,16,"\033[%c;%d;%dm",(fg>>3)+'0',40+(bg&7),30+(fg&7));
	my_print(txt);
}


/* set background color */
void c_setbgcolor(unsigned char a)
{
	char txt[16];
	snprintf(txt,16,"\033[%dm",40+(a&7));
	my_print(txt);
}


/* set highlight color */
void c_sethlt(unsigned char a)
{
	my_print(a?"\033[1m":"\033[0m");
}


/* set highlight and background color */
void c_sethlt_bg(unsigned char hlt,unsigned char bg)
{
	char txt[16];
	
	snprintf(txt,16,"\033[%d;%dm",hlt&1,40+(bg&7));
	my_print(txt);
}


/* set 3 bit foreground color */
void c_setcolor_3b(unsigned char a)
{
	char txt[8];
	snprintf(txt,16,"\033[%dm",30+(a&7));
	my_print(txt);
}


/* set 3 bit foreground color and background color*/
void c_setcolor_3b_bg(unsigned char fg,unsigned char bg)
{
	char txt[16];

	snprintf(txt,16,"\033[%d;%dm",30+(fg&7),40+(bg&7));
	my_print(txt);
}


/* print on the cursor position */
void c_print(char * text)
{
	my_print(text);
}


/* print char on the cursor position */
void c_putc(char c)
{
	my_putc(c);
}


/* clear the screen */
void c_cls(void)
{
	my_print("\033[2J");
}


/* clear rectangle on the screen */
/* presumtions: x2>=x1 && y2>=y1 */
void c_clear(int x1,int y1,int x2,int y2)
{
	char *line;
	int y;
 
 	line=mem_alloc(x2-x1+1);
	if (!line){fprintf(stderr,"Error: Not enough memory.\n");EXIT(1);}
	for (y=0;y<x2-x1+1;y++)
		line[y]=' ';
	line[y]=0;
	for(y=y1;y<=y2;y++)
	{
		c_goto(x1,y);c_print(line);
	}
	mem_free(line);
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
	switch (type)
	{
		case C_NORMAL:
		my_print("\033[?25h");
		break;

		case C_HIDE:
		my_print("\033[?25l");
		break;
	}
}


/* ring the bell */
void c_bell(void)
{
	my_print("\007");
}


/* get screen dimensions */
void c_get_size(int *x, int *y)
{
#ifndef __EMX__
	struct winsize ws;
        if (ioctl(1, TIOCGWINSZ, &ws) != -1)
	{
		*x=ws.ws_col;
		*y=ws.ws_row;
	}
	else
	{
		*x=80;
		*y=24;
	}
#ifdef TRI_D
	if (TRI_D_ON)
		(*x)>>=1;
	screen_width=*x;
#endif
	return;
#else
	int s[2];
	_scrsize(s);
	*x = s[0];
	*y = s[1];
#endif
}

#else
	#include "winconsole.c"
#endif
