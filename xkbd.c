/* portable X keyboard patch by Peter Berg Larsen <pebl@diku.dk> */

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <signal.h>
#include <string.h>

#include "x.h"
#include "kbd.h"
#include "console.h"
#include "cfg.h"
#include "time.h"

#ifndef  XK_MISCELLANY  /* Escape and other Keysymdefinitions */
# define XK_MISCELLANY
#endif
#ifndef  XK_LATIN1      /* Space, a, b etc */
# define XK_LATIN1
#endif
#include <X11/keysymdef.h>


static unsigned char keyboard[128]; /* 0=not pressed, !0=pressed */
static unsigned char old_keyboard[128]; /* 0=not pressed, !0=pressed */
extern Atom x_delete_window_atom,x_wm_protocols_atom;


void kbd_init(void)
{
	/* nothing to initialize */
}


void kbd_close(void)
{
	/* nothing to close */
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
	K_NUM2,K_NUM3,K_NUM0,K_NUM_DOT,0,0,0,K_F11,K_F12,K_HOME,K_UP,K_PGUP,K_LEFT,0,K_RIGHT,K_END,
	K_DOWN,K_PGDOWN,K_INSERT,K_DELETE,K_NUM_ENTER,K_RIGHT_CTRL,K_PAUSE,K_SYSRQ,K_NUM_SLASH,K_RIGHT_ALT,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	
	return remap_table[k&127];
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
		case K_NUM_ENTER: return 100;
		case K_RIGHT_CTRL: return 101;
		case K_NUM_SLASH: return 104;
		case K_SYSRQ: return 103;
		case K_RIGHT_ALT: return 105;
		case K_HOME: return 89;
		case K_UP: return 90;
		case K_PGUP: return 91;
		case K_LEFT: return 92;
		case K_RIGHT: return 94;
		case K_END: return 95;
		case K_DOWN: return 96;
		case K_PGDOWN: return 97;
		case K_INSERT: return 98;
		case K_DELETE: return 99;
		case K_PAUSE: return 102;
		default: return 0;
	}
	
}

void sigwinch_handler(int);
void signal_handler(int);


/* 31/10/2000 22:31. pebl*/
int keycode2keysym (XKeyEvent* e)
{
	KeySym ks;
	int  nbytes;
	static char str[256+1];

	nbytes = XLookupString (e, str, 256, &ks, NULL);
	if (ks == NoSymbol)
		return 0;

	switch (ks)
	{
		case XK_Escape:       return 1;
		case XK_1:            return 2;
		case XK_exclam:       return 2;
		case XK_2:            return 3;
		case XK_at:           return 3;
		case XK_3:            return 4;
		case XK_numbersign:   return 4;
		case XK_4:            return 5;
		case XK_dollar:       return 5;
		case XK_5:            return 6;
		case XK_percent:      return 6;
		case XK_6:            return 7;
		case XK_asciicircum:  return 7;
		case XK_7:            return 8;
		case XK_ampersand:    return 8;
		case XK_8:            return 9;
		case XK_asterisk:     return 9;
		case XK_9:            return 10;
		case XK_parenleft:    return 10;
		case XK_0:            return 11;
		case XK_parenright:   return 11;
		case XK_minus:        return 12;
		case XK_underscore:   return 12;
		case XK_equal:        return 13;
		case XK_plus:         return 13;
		case XK_BackSpace:    return 14;
		case XK_Tab:          return 15;
		case XK_q:            return 16;
		case XK_Q:            return 16;
		case XK_w:            return 17;
		case XK_W:            return 17;
		case XK_e:            return 18;
		case XK_E:            return 18;
		case XK_r:            return 19;
		case XK_R:            return 19;
		case XK_t:            return 20;
		case XK_T:            return 20;
		case XK_y:            return 21;
		case XK_Y:            return 21;
		case XK_u:            return 22;
		case XK_U:            return 22;
		case XK_i:            return 23;
		case XK_I:            return 23;
		case XK_o:            return 24;
		case XK_O:            return 24;
		case XK_p:            return 25;
		case XK_P:            return 25;
		case XK_bracketleft:  return 26;
		case XK_braceleft:    return 26;
		case XK_bracketright: return 27;
		case XK_braceright:   return 27;
		case XK_Return:       return 28;
		case XK_Control_L:    return 29;
		case XK_a:            return 30;
		case XK_A:            return 30;
		case XK_s:            return 31;
		case XK_S:            return 31;
		case XK_D:            return 32;
		case XK_d:            return 32;
		case XK_f:            return 33;
		case XK_F:            return 33;
		case XK_g:            return 34;
		case XK_G:            return 34;
		case XK_h:            return 35;
		case XK_H:            return 35;
		case XK_j:            return 36;
		case XK_J:            return 36;
		case XK_k:            return 37;
		case XK_K:            return 37;
		case XK_l:            return 38;
		case XK_L:            return 38;
		case XK_semicolon:    return 39;
		case XK_colon:        return 39;
		case XK_apostrophe:   return 40;
		case XK_quotedbl:     return 40;
		case XK_grave:        return 41;
		case XK_asciitilde:   return 41;
		case XK_Shift_L:      return 42;
		case XK_backslash:    return 43;
		case XK_bar:          return 43;
		case XK_z:            return 44;
		case XK_Z:            return 44;
		case XK_x:            return 45;
		case XK_X:            return 45;
		case XK_c:            return 46;
		case XK_C:            return 46;
		case XK_v:            return 47;
		case XK_V:            return 47;
		case XK_b:            return 48;
		case XK_B:            return 48;
		case XK_n:            return 49;
		case XK_N:            return 49;
		case XK_m:            return 50;
		case XK_M:            return 50;
		case XK_comma:        return 51;
		case XK_less:         return 51;
		case XK_period:       return 52;
		case XK_greater:      return 52;
		case XK_slash:        return 53;
		case XK_question:     return 53;
		case XK_Shift_R:      return 54;
		case XK_KP_Multiply:  return 55;
		case XK_Alt_L:        return 56;
		case XK_space:        return 57;
		case XK_Caps_Lock:    return 58;
		case XK_F1:           return 59;
		case XK_F2:           return 60;
		case XK_F3:           return 61;
		case XK_F4:           return 62;
		case XK_F5:           return 63;
		case XK_F6:           return 64;
		case XK_F7:           return 65;
		case XK_F8:           return 66;
		case XK_F9:           return 67;
		case XK_F10:          return 68;
		case XK_Num_Lock:     return 69;
		case XK_Scroll_Lock:  return 70;
		case XK_KP_7:         return 71;
		case XK_KP_8:         return 72;
		case XK_KP_9:         return 73;
		case XK_KP_Subtract:  return 74;
		case XK_KP_4:         return 75;
		case XK_KP_5:         return 76;
		case XK_KP_6:         return 77;
		case XK_KP_Add:       return 78;
		case XK_KP_1:         return 79;
		case XK_KP_2:         return 80;
		case XK_KP_3:         return 81;
		case XK_KP_0:         return 82;
		case XK_KP_Separator: return 83;
		case XK_F11:          return 87;
		case XK_F12:          return 88;
		case XK_Home:         return 89;
		case XK_Up:           return 90;
		case XK_Page_Up:      return 91;
		case XK_Left:         return 92;
		case XK_Right:        return 94;
		case XK_End:          return 95;
		case XK_Down:         return 96;
		case XK_Page_Down:    return 97;
		case XK_Insert:       return 98;
		case XK_Delete:       return 99;
		case XK_KP_Enter:     return 100;
		case XK_Control_R:    return 101;
		case XK_Pause:        return 102;
		case XK_Sys_Req:      return 103;
		case XK_KP_Divide:    return 104;
		case XK_Alt_R:        return 105; 
		default: return 0;
	 }
}

/* returns zero if nothing was read, non-zero otherwise */
/* maintains also all window events */
int kbd_update(void)
{
	int retval=0,keysym=0;
	int n,a;
	XEvent event;
	memcpy(old_keyboard,keyboard,128);
	
	while((n=XPending(display)))
	{
		for (a=0;a<n;a++)
		{
			XNextEvent(display,&event);
			switch (event.type)
			{
				case KeyPress:
				retval=1;
				keysym = keycode2keysym((XKeyEvent*)&event);  /* 31/10/2000 22:31. pebl */
				if (keysym > 127)break;
				keyboard[keysym]=1;
				break;
	
				case KeyRelease:
				retval=1;
				keysym = keycode2keysym((XKeyEvent*)&event);  /* 31/10/2000 22:31. pebl */
				if (keysym>127)break;
				keyboard[keysym]=0;
				break;
	
				case ClientMessage:
				if (
					event.xclient.format!=32||
					event.xclient.message_type!=x_wm_protocols_atom||
					event.xclient.data.l[0]!=(signed)x_delete_window_atom
					)
					break;
	
				/* This event is destroy window event from window manager */
	
				case DestroyNotify:
#ifdef SIGQUIT
				raise(SIGQUIT);
#else
				signal_handler(3);
#endif
				break;
					
				
				case ConfigureNotify:  /* window was resized */
				x_width=event.xconfigure.width/FONT_X_SIZE;
				x_height=event.xconfigure.height/FONT_Y_SIZE;
	
				case Expose: /* window was exposed - redraw it */
#ifdef SIGWINCH
				raise(SIGWINCH);
#else
				sigwinch_handler(-1);
#endif
				break;
			}
		}
	}

	/* CTRL-C */
	if ((keyboard[29]||keyboard[97])&&keyboard[46])
		raise(SIGINT);

	return retval;
}


/* returns 1 if given key is pressed, 0 otherwise */
int kbd_is_pressed(int key)
{
	int a=remap_in(key);
	return a?keyboard[a]:0;
}


/* same as kbd_is_pressed but tests rising edge of the key */
int kbd_was_pressed(int key)
{
	int a=remap_in(key);
	if (!a)return 0;
	return !old_keyboard[remap_in(key)]&&keyboard[remap_in(key)];
}


void kbd_wait_for_key(void)
{
	XEvent e;
#ifdef TRI_D
	if (TRI_D_ON)
		while (
		XCheckWindowEvent(display,window2,KeyPressMask|KeyReleaseMask,&e)==False&&
		XCheckWindowEvent(display,window,KeyPressMask|KeyReleaseMask,&e)==False
		)my_sleep(50000);
	else
		XWindowEvent(display,window,KeyPressMask|KeyReleaseMask,&e);
#else
	XWindowEvent(display,window,KeyPressMask|KeyReleaseMask,&e);
#endif
	XPutBackEvent(display,&e);
}
