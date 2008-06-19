#ifndef WIN32

#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_LINUX_KD_H
	#define HAVE_RAW_KEYBOARD
	#include <linux/kd.h>
#endif

#ifdef HAVE_LINUX_VT_H
	#define HAVE_VT_SWITCH
	#include <linux/vt.h>
#endif

#ifdef __EMX__
	#define INCL_DOS
	#define INCL_DOSERRORS
	#define INCL_DOSMONITORS
	#define INCL_KBD
	#include <os2.h>
	#define HAVE_OS2_KEYBOARD
#endif

#include "kbd.h"
#include "console.h"

#define KBD_RAW 0
#define KBD_STD 1

#define KBD_BUFFER_SIZE 256  /* size of buffer for incoming keys */

#ifdef HAVE_RAW_KEYBOARD
	static int oldkbmode;   /* original keyboard mode */
#endif
int keyboard_type;

/* RAW-MODE KEYBOARD */
/* keyboard bitmaps: current and previous */
static unsigned char keyboard[128]; /* 0=not pressed, !0=pressed */
static unsigned char old_keyboard[128]; /* 0=not pressed, !0=pressed */

/* NON-RAW KEYBOARD: */
int current_key;
int shift_pressed;

#ifdef HAVE_OS2_KEYBOARD

#define OS2_ALIGN(var) (_THUNK_PTR_STRUCT_OK(&var##1) ? &var##1 : &var##2)

HMONITOR os2_hmon;

struct os2_key_packet {
	USHORT mon_flag;
	UCHAR chr;
	UCHAR scan;
	UCHAR dbcs;
	UCHAR dbcs_shift;
	USHORT shift_state;
	ULONG ms;
	USHORT dd_flag;
};

struct os2_buffer {
	USHORT cb;
	struct os2_key_packet packet;
	char pad[20];
};

struct os2_key_packet *os2_key;
struct os2_buffer *os2_in;
struct os2_buffer *os2_out;

int os2_read_mon(unsigned char *data, int len)
{
	static int prefix = 0;
	int bytes_read = 0;
	int r;
	USHORT count = sizeof(struct os2_key_packet);
	while (bytes_read < len) {
		int scan;
		r = DosMonRead((void *)os2_in, IO_NOWAIT, (void *)os2_key, &count);
		if (r == ERROR_MON_BUFFER_EMPTY) break;
		if (r) {
			fprintf(stderr, "DosMonRead: %d\n", r);
			sleep(1);
			break;
		}
		/*fprintf(stderr, "monflag: %04x, scan %02x, shift %04x\n", os2_key->mon_flag, os2_key->scan, os2_key->shift_state);*/
		scan = os2_key->mon_flag >> 8;
		/*fprintf(stderr, "scan: %02x\n", scan); fflush(stderr);*/
		if (scan == 0xE0) {
			prefix = 1;
			c:continue;
		}
		if (prefix) {
			prefix = 0;
			switch (scan & 0x7f) {
			case 29:	scan = scan & 0x80 | 97; break;
			case 71:	scan = scan & 0x80 | 102; break;
			case 72:	scan = scan & 0x80 | 103; break;
			case 73:	scan = scan & 0x80 | 104; break;
			case 75:	scan = scan & 0x80 | 105; break;
			case 77:	scan = scan & 0x80 | 106; break;
			case 79:	scan = scan & 0x80 | 107; break;
			case 80:	scan = scan & 0x80 | 108; break;
			case 81:	scan = scan & 0x80 | 109; break;
			case 82:	scan = scan & 0x80 | 110; break;
			case 83:	scan = scan & 0x80 | 111; break;
			default:	
		/*fprintf(stderr, "scan: %02x\n", scan); fflush(stderr);*/
					goto c;
			}
		}
		data[bytes_read++] = scan;
	}
	return bytes_read;
}

#endif

/* reads byte from input */
int my_getchar(void)
{
	unsigned char a;
	read(0,&a,1);
	return a;
}


/* test if there's something on input */
int kbhit(void)
{
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(0,&fds);
	tv.tv_sec=0;
	tv.tv_usec=0;

	return !!(select(1,&fds,NULL,NULL,&tv));
}


void test_shift(int k)
{
	switch(k)
	{
		case '~':
		case '!':
		case '@':
		case '#':
		case '$':
		case '%':
		case '^':
		case '&':
		case '*':
		case '(':
		case ')':
		case '_':
		case '+':
		case '|':
		case 'Q':
		case 'W':
		case 'E':
		case 'R':
		case 'T':
		case 'Y':
		case 'U':
		case 'I':
		case 'O':
		case 'P':
		case '{':
		case '}':
		case 'A':
		case 'S':
		case 'D':
		case 'F':
		case 'G':
		case 'H':
		case 'J':
		case 'K':
		case 'L':
		case ':':
		case '"':
		case 'Z':
		case 'X':
		case 'C':
		case 'V':
		case 'B':
		case 'N':
		case 'M':
		case '<':
		case '>':
		case '?':
		shift_pressed=1;
	}
}


/* non-raw keyboard */
int getkey(void)
{
	int k;
	k=my_getchar();
	shift_pressed=0;
	switch(k)
	{
		case 0:
		switch(my_getchar())
		{
			case 72:
			return K_UP;

			case 75:
			return K_LEFT;

			case 77:
			return K_RIGHT;

			case 80:
			return K_DOWN;

			default:
			return 0;
		}
		
		case 8:
		return K_BACKSPACE;

		case 9:
		return K_TAB;

		case 13:
		return K_ENTER;

		case 27:
		if (kbhit())
			switch(my_getchar())
			{
				case 91:
				switch(my_getchar())
				{
					case 65:
					return K_UP;
	
					case 66:
					return K_DOWN;
		
					case 67:
					return K_RIGHT;
	
					case 68:
					return K_LEFT;
				}
			}
		return K_ESCAPE;

		case 127:
		return K_BACKSPACE;

		default:
		test_shift(k);
		return tolower(k);
	}
}


/* on error returns 1 */
void kbd_init(void)
{
#ifdef HAVE_RAW_KEYBOARD

	keyboard_type=KBD_RAW;
	
	/* raw keyboard */
	if (ioctl(0,KDGKBMODE,&oldkbmode))goto std;
	if (ioctl(0,KDSKBMODE,K_MEDIUMRAW))goto std;

	/* GREAT EXPLANATION TO A SMALL FCNTL: */
	/* ----------------------------------- */

	/* this fcntl is a great cheat */
	/* it switches STDOUT!!!!! to noblocking mode */
	/* as i know 0 is STDIN so why it changes STDOUT settings?????? */
	/* (does anyone know why???? (if you know send me a mail - you win million:-) (stupid joke...))) */
	/* it causes scrambled picture on xterm and telnet */
	/* because terminal doesn't manage and loses characters */
	/* fortunatelly this fcntl is necessary only with raw keyboard */
	/* and to switch keyboard to raw mode you must physically sit on the machine */
	/* and when you sit on the machine, terminal is fast enough to manage ;-) */
	fcntl(0,F_SETFL,fcntl(0,F_GETFL)|O_NONBLOCK);

	memset(keyboard,0,128);
	memset(old_keyboard,0,128);
	return;
std:
#endif

#ifdef HAVE_OS2_KEYBOARD
	keyboard_type=KBD_RAW;
	{
		static struct os2_key_packet os2_key1, os2_key2;
		static struct os2_buffer os2_in1, os2_in2, os2_out1, os2_out2;
		int r;
		if ((r = DosMonOpen("KBD$", &os2_hmon))) {
			fprintf(stderr, "DosMonOpen: %d\n", r);
			sleep(1);
			goto std;
		}
		os2_key = OS2_ALIGN(os2_key);
		os2_in = OS2_ALIGN(os2_in);
		os2_out = OS2_ALIGN(os2_out);
		os2_in->cb = sizeof(struct os2_buffer) - sizeof(USHORT);
		os2_out->cb = sizeof(struct os2_buffer) - sizeof(USHORT);
		if ((r = DosMonReg(os2_hmon, (void *)os2_in, (void *)os2_out, MONITOR_END, -1))) {
			if (r = ERROR_MONITORS_NOT_SUPPORTED) {
				fprintf(stderr, "RAW keyboard is disabled. Please run 0verkill in full-screen session.%c%c%c", (char)7, (char)7, (char)7);
				sleep(5);
			} else {
				fprintf(stderr, "DosMonReg: %d\n", r);
				sleep(1);
			}
			goto std1;
		}
	}
	memset(keyboard,0,128);
	memset(old_keyboard,0,128);
	return;
std1:	DosMonClose(os2_hmon);
std:
#endif

	/* standard keyboard */
	keyboard_type=KBD_STD;
	/* nothing to initialize */
	current_key=0;
	return;
}


void kbd_close(void)
{
	switch(keyboard_type)
	{
#ifdef HAVE_RAW_KEYBOARD
		case KBD_RAW:
		ioctl(0,KDSKBMODE,oldkbmode);
		return;
#endif

#ifdef HAVE_OS2_KEYBOARD
		case KBD_RAW:
		DosMonClose(os2_hmon);
		return;
#endif

		case KBD_STD:
		/* nothing to close */
		return;
	}
}


/* convert key from scancode to key constant */
int remap_out(int k)
{
	int remap_table[128]={
	0,K_ESCAPE,'1','2','3','4','5','6','7','8','9','0','-','=',K_BACKSPACE,K_TAB,
	'q','w','e','r','t','y','u','i','o','p','[',']',K_ENTER,K_LEFT_CTRL,'a','s',
	'd','f','g','h','j','k','l',';','\'','`',K_LEFT_SHIFT,'\\','z','x','c','v',
	'b','n','m',',','.','/',K_RIGHT_SHIFT,K_NUM_ASTERISK,K_LEFT_ALT,' ',K_CAPS_LOCK,K_F1,K_F2,K_F3,K_F4,K_F5,
	K_F6,K_F7,K_F8,K_F9,K_F10,K_NUM_LOCK,K_SCROLL_LOCK,K_NUM7,K_NUM8,K_NUM9,K_NUM_MINUS,K_NUM4,K_NUM5,K_NUM6,K_NUM_PLUS,K_NUM1,
	K_NUM2,K_NUM3,K_NUM0,K_NUM_DOT,0,0,0,K_F11,K_F12,0,0,0,0,0,0,0,
	K_NUM_ENTER,K_RIGHT_CTRL,K_NUM_SLASH,K_SYSRQ,K_RIGHT_ALT,0,K_HOME,K_UP,K_PGUP,K_LEFT,K_RIGHT,K_END,K_DOWN,K_PGDOWN,K_INSERT,K_DELETE,
	0,0,0,0,0,0,0,K_PAUSE,0,0,0,0,0,0,0,0
	};
	
	return remap_table[k];
}


/* convert key constant to scancode */
int remap_in(int k)
{
	switch (k)
	{
		case K_ESCAPE: return 1;
		case '1': return 2;
		case '2': return 3;
		case '3': return 4;
		case '4': return 5;
		case '5': return 6;
		case '6': return 7;
		case '7': return 8;
		case '8': return 9;
		case '9': return 10;
		case '0': return 11;
		case '-': return 12;
		case '=': return 13;
		case K_BACKSPACE: return 14;
		case K_TAB: return 15;
		case 'q': return 16;
		case 'w': return 17;
		case 'e': return 18;
		case 'r': return 19;
		case 't': return 20;
		case 'y': return 21;
		case 'u': return 22;
		case 'i': return 23;
		case 'o': return 24;
		case 'p': return 25;
		case '[': return 26;
		case ']': return 27;
		case K_ENTER: return 28;
		case K_LEFT_CTRL: return 29;
		case 'a': return 30;
		case 's': return 31;
		case 'd': return 32;
		case 'f': return 33;
		case 'g': return 34;
		case 'h': return 35;
		case 'j': return 36;
		case 'k': return 37;
		case 'l': return 38;
		case ';': return 39;
		case '\'': return 40;
		case '`': return 41;
		case K_LEFT_SHIFT: return 42;
		case '\\': return 43;
		case 'z': return 44;
		case 'x': return 45;
		case 'c': return 46;
		case 'v': return 47;
		case 'b': return 48;
		case 'n': return 49;
		case 'm': return 50;
		case ',': return 51;
		case '.': return 52;
		case '/': return 53;
		case K_RIGHT_SHIFT: return 54;
		case K_NUM_ASTERISK: return 55;
		case K_LEFT_ALT: return 56;
		case ' ': return 57;
		case K_CAPS_LOCK: return 58;
		case K_F1: return 59;
		case K_F2: return 60;
		case K_F3: return 61;
		case K_F4: return 62;
		case K_F5: return 63;
		case K_F6: return 64;
		case K_F7: return 65;
		case K_F8: return 66;
		case K_F9: return 67;
		case K_F10: return 68;
		case K_NUM_LOCK: return 69;
		case K_SCROLL_LOCK: return 70;
		case K_NUM7: return 71;
		case K_NUM8: return 72;
		case K_NUM9: return 73;
		case K_NUM_MINUS: return 74;
		case K_NUM4: return 75;
		case K_NUM5: return 76;
		case K_NUM6: return 77;
		case K_NUM_PLUS: return 78;
		case K_NUM1: return 79;
		case K_NUM2: return 80;
		case K_NUM3: return 81;
		case K_NUM0: return 82;
		case K_NUM_DOT: return 83;
		case K_F11: return 87;
		case K_F12: return 88;
		case K_NUM_ENTER: return 96;
		case K_RIGHT_CTRL: return 97;
		case K_NUM_SLASH: return 98;
		case K_SYSRQ: return 99;
		case K_RIGHT_ALT: return 100;
		case K_HOME: return 102;
		case K_UP: return 103;
		case K_PGUP: return 104;
		case K_LEFT: return 105;
		case K_RIGHT: return 106;
		case K_END: return 107;
		case K_DOWN: return 108;
		case K_PGDOWN: return 109;
		case K_INSERT: return 110;
		case K_DELETE: return 111;
		case K_PAUSE: return 119;
		default: return 0;
	}
	
}


/* returns zero if nothing was read, non-zero otherwise */
int kbd_update(void)
{
	unsigned char buffer[KBD_BUFFER_SIZE];
	int bytesread,a;

	switch(keyboard_type)
	{
		case KBD_RAW:
		memcpy(old_keyboard,keyboard,128);
#ifndef HAVE_OS2_KEYBOARD
		bytesread=read(0,buffer,KBD_BUFFER_SIZE);
#else
		bytesread=os2_read_mon(buffer, KBD_BUFFER_SIZE);
#endif
		if (bytesread<=0)return 0;
read_again:
		for (a=0;a<bytesread;a++)
			keyboard[(buffer[a])&127]=!((buffer[a])&128);
		if (bytesread==KBD_BUFFER_SIZE)
		{
#ifndef HAVE_OS2_KEYBOARD
			bytesread=read(0,buffer,KBD_BUFFER_SIZE);
#else
			bytesread=os2_read_mon(buffer, KBD_BUFFER_SIZE);
#endif
			if (bytesread<=0)return 1;
			goto read_again;
		}
	
		/* CTRL-C */
		if ((keyboard[29]||keyboard[97])&&keyboard[46])
			raise(SIGINT);
	
#ifdef HAVE_VT_SWITCH
		/* console switching */
		if (keyboard[56]||keyboard[100])  /* alt */
		{
			for(a=59;a<=68;a++)  /* function keys */
				if (keyboard[a])
				{
					memset(keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
					memset(old_keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
					ioctl(0,VT_ACTIVATE,a-58);  /* VT switch */
					break;
				}
			if (keyboard[87])
			{
				memset(keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
				memset(old_keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
				ioctl(0,VT_ACTIVATE,11);  /* VT switch */
			}
			else if (keyboard[88])
			{
				memset(keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
				memset(old_keyboard,0,128); /* terminal is inactive=>keys can't be pressed */
				ioctl(0,VT_ACTIVATE,12);  /* VT switch */
			}
		}
#endif
		return 1;
	
		case KBD_STD:
		current_key=0;
		if (!kbhit())return 0;
		current_key=getkey();
		return 1;
	}
	return 0;
}


/* returns 1 if given key is pressed, 0 otherwise */
int kbd_is_pressed(int key)
{
	switch(keyboard_type)
	{
		case KBD_RAW:
		return keyboard[remap_in(key)];

		case KBD_STD:
		if (key==K_LEFT_SHIFT||key==K_RIGHT_SHIFT)return shift_pressed;
		return (current_key==key);
	}
	return 0;
}


/* same as kbd_is_pressed but tests rising edge of the key */
int kbd_was_pressed(int key)
{
	switch(keyboard_type)
	{
		case KBD_RAW:
		return !old_keyboard[remap_in(key)]&&keyboard[remap_in(key)];

		case KBD_STD:
		return kbd_is_pressed(key);
	}
	return 0;
}


void kbd_wait_for_key(void)
{
	fd_set fds;

#ifdef __EMX__
	if (keyboard_type == KBD_RAW) return;
#endif
	FD_ZERO(&fds);
	FD_SET(0,&fds);
	select(1,&fds,NULL,NULL,0);
}

#else
	#include "winkbd.c"
#endif
