#include "cfg.h"
#include "kbd.h"

BYTE	keyboard[256];
DWORD	controlStates[256];
DWORD	lastControlState;

HANDLE	hMyConsoleInput;
/* DWORD	idProcess; */

void kbd_init(void) {
/*	GetWindowThreadProcessId(GetForegroundWindow(), &idProcess); */
	hMyConsoleInput=GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hMyConsoleInput, ENABLE_WINDOW_INPUT);

 	memset(keyboard, 0, sizeof(keyboard));
 	memset(controlStates, 0, sizeof(controlStates));
	lastControlState=0;
	kbd_update();
}

void kbd_close(void) {
}

#ifdef CLIENT
void sigwinch_handler(int signum);
#endif

int kbd_update(void) {
	INPUT_RECORD	ir[50];
	DWORD			rd=0;
	int				chg=0;
	if ( WaitForSingleObject(hMyConsoleInput, 0)!=WAIT_TIMEOUT ) {
		if ( ReadConsoleInput(hMyConsoleInput, ir, 50, &rd) ) {
			DWORD	i;
			for(i=0; i<rd; i++) {
				if ( ir[i].EventType==KEY_EVENT ) {
					DWORD	vkc=ir[i].Event.KeyEvent.wVirtualKeyCode;
					if ( ir[i].Event.KeyEvent.bKeyDown ) {
						if ( !(keyboard[vkc]&1) )
							keyboard[vkc]|=2;
						keyboard[vkc]|=1;
						lastControlState=controlStates[vkc]=ir[i].Event.KeyEvent.dwControlKeyState;
					}
					else {
						keyboard[vkc]&=~1;
						lastControlState=ir[i].Event.KeyEvent.dwControlKeyState;
					}
				}
				else if ( ir[i].EventType==WINDOW_BUFFER_SIZE_EVENT ) {
					SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), ir[i].Event.WindowBufferSizeEvent.dwSize);
					#ifdef CLIENT
					sigwinch_handler(0);
					#endif
				}
			};
			/* process ext. keys according to lastControlState */
			if ( lastControlState&RIGHT_CTRL_PRESSED ) {
				if ( !(keyboard[VK_RCONTROL]&1) )
					keyboard[VK_RCONTROL]|=2;
				keyboard[VK_RCONTROL]|=1;
			}
			else
				keyboard[VK_RCONTROL]&=~1;
			if ( lastControlState&LEFT_CTRL_PRESSED ) {
				if ( !(keyboard[VK_LCONTROL]&1) )
					keyboard[VK_LCONTROL]|=2;
				keyboard[VK_LCONTROL]|=1;
			}
			else
				keyboard[VK_LCONTROL]&=~1;

			if ( lastControlState&RIGHT_ALT_PRESSED ) {
				if ( !(keyboard[VK_RMENU]&1) )
					keyboard[VK_RMENU]|=2;
				keyboard[VK_RMENU]|=1;
			}
			else
				keyboard[VK_RMENU]&=~1;
			if ( lastControlState&LEFT_ALT_PRESSED ) {
				if ( !(keyboard[VK_LMENU]&1) )
					keyboard[VK_LMENU]|=2;
				keyboard[VK_LMENU]|=1;
			}
			else
				keyboard[VK_LMENU]&=~1;
		}
	};
	return chg;
}

SHORT IntToVKey(int k) {
	if ( k>255 ) {
		SHORT	vKey;
		switch( k ) {
		case K_ESCAPE: vKey=VK_ESCAPE; break;
		case K_BACKSPACE: vKey=VK_BACK; break;
		case K_TAB: vKey=VK_TAB; break;
		case K_ENTER: vKey=VK_RETURN; break;
		case K_RIGHT_SHIFT: vKey=VK_RSHIFT; break;
		case K_LEFT_SHIFT: vKey=VK_LSHIFT; break;
		case K_RIGHT_CTRL: vKey=VK_RCONTROL; break;
		case K_LEFT_CTRL: vKey=VK_LCONTROL; break;
		case K_LEFT_ALT: vKey=VK_LMENU; break;
		case K_RIGHT_ALT: vKey=VK_RMENU; break;
		case K_NUM_ASTERISK: vKey=VK_MULTIPLY; break;
		case K_CAPS_LOCK: vKey=VK_CAPITAL; break;
		case K_NUM_LOCK: vKey=VK_NUMLOCK; break;
		case K_SCROLL_LOCK: vKey=VK_SCROLL; break;
		case K_PAUSE: vKey=VK_PAUSE; break;
		case K_SYSRQ: vKey=VK_SNAPSHOT; break;
		case K_F1: vKey=VK_F1; break;
		case K_F2: vKey=VK_F2; break;
		case K_F3: vKey=VK_F3; break;
		case K_F4: vKey=VK_F4; break;
		case K_F5: vKey=VK_F5; break;
		case K_F6: vKey=VK_F6; break;
		case K_F7: vKey=VK_F7; break;
		case K_F8: vKey=VK_F8; break;
		case K_F9: vKey=VK_F9; break;
		case K_F10: vKey=VK_F10; break;
		case K_F11: vKey=VK_F11; break;
		case K_F12: vKey=VK_F12; break;
		case K_NUM1: vKey=VK_NUMPAD1; break;
		case K_NUM2: vKey=VK_NUMPAD2; break;
		case K_NUM3: vKey=VK_NUMPAD3; break;
		case K_NUM4: vKey=VK_NUMPAD4; break;
		case K_NUM5: vKey=VK_NUMPAD5; break;
		case K_NUM6: vKey=VK_NUMPAD6; break;
		case K_NUM7: vKey=VK_NUMPAD7; break;
		case K_NUM8: vKey=VK_NUMPAD8; break;
		case K_NUM9: vKey=VK_NUMPAD9; break;
		case K_NUM0: vKey=VK_NUMPAD0; break;
		case K_NUM_MINUS: vKey=VK_SUBTRACT; break;
		case K_NUM_PLUS: vKey=VK_ADD; break;
		case K_NUM_DOT: vKey=VK_DECIMAL; break;
		case K_NUM_ENTER: vKey=VK_RETURN; break;
		case K_NUM_SLASH: vKey=VK_DIVIDE; break;
		case K_LEFT: vKey=VK_LEFT; break;
		case K_RIGHT: vKey=VK_RIGHT; break;
		case K_UP: vKey=VK_UP; break;
		case K_DOWN: vKey=VK_DOWN; break;
		case K_HOME: vKey=VK_HOME; break;
		case K_END: vKey=VK_END; break;
		case K_INSERT: vKey=VK_INSERT; break;
		case K_DELETE: vKey=VK_DELETE; break;
		case K_PGUP: vKey=VK_PRIOR; break;
		case K_PGDOWN: vKey=VK_NEXT; break;
		default: vKey=-1;
		}
		return vKey;
	}
	else
		return VkKeyScanEx((unsigned char)(k&0xFF), GetKeyboardLayout(0));
}

int kbd_is_pressed(int k) {
	/* return GetKeyState(IntToVKey(k))&0x8000; */
	SHORT	key=IntToVKey(k);
	int		n;
	if ( key==-1 )
		return 0;
	if (( n=(keyboard[key&0xFF]&1) )&&( k>=32 )&&( k<=127 )) {
		if ( key&0x100 ) {	/* shift must be down */
			if ( !(controlStates[key&0xFF]&SHIFT_PRESSED) )
				n=0;
		}
		else {		/* shift must be up */
			if ( controlStates[key&0xFF]&SHIFT_PRESSED )
				n=0;
		}
		if ( key&0x200 ) {	/* ctrl must be down */
			if (!(( controlStates[key&0xFF]&RIGHT_CTRL_PRESSED )||
					( controlStates[key&0xFF]&LEFT_CTRL_PRESSED )))
				n=0;
		}
		else {		/* ctrl must be up */
			if (( controlStates[key&0xFF]&RIGHT_CTRL_PRESSED )||
				( controlStates[key&0xFF]&LEFT_CTRL_PRESSED ))
				n=0;
		}
		if ( key&0x400 ) {	/* alt must be down */
			if (!(( controlStates[key&0xFF]&RIGHT_ALT_PRESSED )||
				( controlStates[key&0xFF]&LEFT_ALT_PRESSED )))
				n=0;
		}
		else {		/* alt must be up */
			if (( controlStates[key&0xFF]&RIGHT_ALT_PRESSED )||
				( controlStates[key&0xFF]&LEFT_ALT_PRESSED ))
				n=0;
		}
	};
	return n;
}

int kbd_was_pressed(int k) {
	/* return GetKeyState(IntToVKey(k))&0x8000; */
	SHORT	key=IntToVKey(k);
	int		n;
	if ( key==-1 )
		return 0;
	if (( n=(keyboard[key&0xFF]&2) )&&( k>=32 )&&( k<=127 )) {
		if ( key&0x100 ) {	/* shift must be down */
			if ( !(controlStates[key&0xFF]&SHIFT_PRESSED) )
				n=0;
		}
		else {		/* shift must be up */
			if ( controlStates[key&0xFF]&SHIFT_PRESSED )
				n=0;
		}
		if ( key&0x200 ) {	/* ctrl must be down */
			if (!(( controlStates[key&0xFF]&RIGHT_CTRL_PRESSED )||
					( controlStates[key&0xFF]&LEFT_CTRL_PRESSED )))
				n=0;
		}
		else {		/* ctrl must be up */
			if (( controlStates[key&0xFF]&RIGHT_CTRL_PRESSED )||
				( controlStates[key&0xFF]&LEFT_CTRL_PRESSED ))
				n=0;
		}
		if ( key&0x400 ) {	/* alt must be down */
			if (!(( controlStates[key&0xFF]&RIGHT_ALT_PRESSED )||
				( controlStates[key&0xFF]&LEFT_ALT_PRESSED )))
				n=0;
		}
		else {		/* alt must be up */
			if (( controlStates[key&0xFF]&RIGHT_ALT_PRESSED )||
				( controlStates[key&0xFF]&LEFT_ALT_PRESSED ))
				n=0;
		}
	};
	if ( n )
		keyboard[key&0xFF]&=~2;
	return n;
}

void kbd_wait_for_key(void) {
	WaitForSingleObject(hMyConsoleInput, INFINITE);
}
