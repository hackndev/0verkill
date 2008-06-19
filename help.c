#include "blit.h"

#define HELP_WIDTH 41    /* 42-(42/42) ..... 42 is really UNIVERSAL number ;-) */
#define HELP_HEIGHT 19
#define HELP_X ((SCREEN_X-2-HELP_WIDTH)>>1)
#define HELP_Y ((SCREEN_Y-2-HELP_HEIGHT)>>1)

/* prints help window */
void print_help_window(void)
{
	draw_frame(HELP_X,HELP_Y,HELP_WIDTH,HELP_HEIGHT,14);
	print2screen(HELP_X+(HELP_WIDTH-4>>1),HELP_Y+1,11,"HELP");
	print2screen(HELP_X+3,HELP_Y+3,10,"LEFT,RIGHT");
	print2screen(HELP_X+21,HELP_Y+3,7,"move");
	print2screen(HELP_X+3,HELP_Y+4,10,"UP");
	print2screen(HELP_X+21,HELP_Y+4,7,"jump");
	print2screen(HELP_X+3,HELP_Y+5,10,"SHIFT+DIRECTION");
	print2screen(HELP_X+21,HELP_Y+5,7,"run");
	print2screen(HELP_X+3,HELP_Y+6,10,"CAPSLOCK,A");
	print2screen(HELP_X+21,HELP_Y+6,7,"toggle autorun");
	print2screen(HELP_X+3,HELP_Y+7,10,"DOWN+DIRECTION");
	print2screen(HELP_X+21,HELP_Y+7,7,"creep");
	print2screen(HELP_X+3,HELP_Y+8,10,"C");
	print2screen(HELP_X+21,HELP_Y+8,7,"toggle creep");
	print2screen(HELP_X+3,HELP_Y+9,10,"Z,CTRL");
	print2screen(HELP_X+21,HELP_Y+9,7,"fire");
	print2screen(HELP_X+3,HELP_Y+10,10,"1,2,3,4,5");
	print2screen(HELP_X+21,HELP_Y+10,7,"change weapon");
	print2screen(HELP_X+3,HELP_Y+11,10,"ENTER");
	print2screen(HELP_X+21,HELP_Y+11,7,"chat");
	print2screen(HELP_X+3,HELP_Y+12,10,"F10,Q");
	print2screen(HELP_X+21,HELP_Y+12,7,"quit");
	print2screen(HELP_X+3,HELP_Y+13,10,"F12");
	print2screen(HELP_X+21,HELP_Y+13,7,"abort game");
	print2screen(HELP_X+3,HELP_Y+14,10,"R");
	print2screen(HELP_X+21,HELP_Y+14,7,"redraw screen");
	print2screen(HELP_X+3,HELP_Y+15,10,"H");
	print2screen(HELP_X+21,HELP_Y+15,7,"toggle help");
	print2screen(HELP_X+3,HELP_Y+16,10,"TAB");
	print2screen(HELP_X+21,HELP_Y+16,7,"toggle top players");
	print2screen(HELP_X+3,HELP_Y+17,10,"SPACE");
	print2screen(HELP_X+21,HELP_Y+17,7,"respawn dead player");
	print2screen(HELP_X+3,HELP_Y+18,10,"D");
	print2screen(HELP_X+21,HELP_Y+18,7,"climb down ladders");
}
