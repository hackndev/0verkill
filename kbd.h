#ifndef __KBD_H
#define __KBD_H


/* special key values */
#define K_ESCAPE		300
#define K_BACKSPACE		301
#define K_TAB			302
#define K_ENTER			303
#define K_RIGHT_SHIFT		304
#define K_RIGHT_ALT		305
#define K_RIGHT_CTRL		306
#define K_LEFT_CTRL		307
#define K_LEFT_ALT		308
#define K_LEFT_SHIFT		309
#define K_NUM_ASTERISK		311
#define K_CAPS_LOCK		312
#define K_NUM_LOCK		313
#define K_SCROLL_LOCK		314
#define K_PAUSE			315
#define K_SYSRQ			316
#define K_F1			317
#define K_F2			318
#define K_F3			319
#define K_F4			320
#define K_F5			321
#define K_F6			322
#define K_F7			323
#define K_F8			324
#define K_F9			325
#define K_F10			326
#define K_F11			327
#define K_F12			328
#define K_NUM1			329
#define K_NUM2			330
#define K_NUM3			331
#define K_NUM4			332
#define K_NUM5			333
#define K_NUM6			334
#define K_NUM7			335
#define K_NUM8			336
#define K_NUM9			337
#define K_NUM0			338
#define K_NUM_MINUS		339
#define K_NUM_PLUS		340
#define K_NUM_DOT		341
#define K_NUM_ENTER		342
#define K_NUM_SLASH		343
#define K_LEFT			344
#define K_RIGHT			345
#define K_UP			346
#define K_DOWN			347
#define K_HOME			348
#define K_END			349
#define K_INSERT		350
#define K_DELETE		351
#define K_PGUP			352
#define K_PGDOWN		353


extern void kbd_init(void);
extern void kbd_close(void);
extern int kbd_update(void);
extern int kbd_is_pressed(int k);
/* key was pressed and is hold */
extern int kbd_was_pressed(int k);
extern void kbd_wait_for_key(void);

#endif
