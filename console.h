/* CONSOLE INTERFACE */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "kbd.h"

/* cursor modes */
#define C_NORMAL 0
#define C_HIDE 1

extern int console_ok;

extern void c_init(int w,int h);
extern void c_shutdown(void);
extern void c_cls(void);
extern void c_print(char *text);
extern void c_putc(char c);
extern void c_goto(int x, int y);
extern void c_clear(int x1,int y1,int x2, int y2);
extern void c_cursor(int c);
extern void c_bell(void);
extern void c_setcolor(unsigned char a);
extern void c_setcolor_bg(unsigned char fg,unsigned char bg);
extern void c_setcolor_3b(unsigned char a);
extern void c_setcolor_3b_bg(unsigned char fg,unsigned char bg);
extern void c_sethlt(unsigned char a);
extern void c_sethlt_bg(unsigned char hlt,unsigned char bg);
extern void c_setbgcolor(unsigned char a);
extern void c_refresh(void);
extern void c_get_size(int *x,int *y);
extern int c_pressed(int k);
extern int c_was_pressed(int k);
extern void c_wait_for_key(void);
extern void c_update_kbd(void);


#endif
