#include "blit.h"

#define HELP_WIDTH 46    /* 42-(42/42) ..... 42 is really UNIVERSAL number ;-) */
#define HELP_HEIGHT 16
#define HELP_X ((SCREEN_X-2-HELP_WIDTH)>>1)
#define HELP_Y ((SCREEN_Y-2-HELP_HEIGHT)>>1)

/* prints help window */
void print_help_window(void)
{
	draw_frame(HELP_X,HELP_Y,HELP_WIDTH,HELP_HEIGHT,14);
	print2screen(HELP_X+(HELP_WIDTH-4>>1),HELP_Y+1,11,"HELP");
	print2screen(HELP_X+3,HELP_Y+3,10,"ARROWS");
	print2screen(HELP_X+12,HELP_Y+3,7,"move");
	print2screen(HELP_X+3,HELP_Y+4,10,"F1,O");
	print2screen(HELP_X+12,HELP_Y+4,7,"toggle online help");
	print2screen(HELP_X+3,HELP_Y+5,10,"Q");
	print2screen(HELP_X+12,HELP_Y+5,7,"quit and save");
	print2screen(HELP_X+3,HELP_Y+6,10,"ENTER");
	print2screen(HELP_X+12,HELP_Y+6,7,"switch command/drawing mode");
	print2screen(HELP_X+3,HELP_Y+7,10,"[");
	print2screen(HELP_X+12,HELP_Y+7,7,"copy previous position into actual");
	print2screen(HELP_X+3,HELP_Y+8,10,"]");
	print2screen(HELP_X+12,HELP_Y+8,7,"copy next position into actual");
	print2screen(HELP_X+3,HELP_Y+9,10,"+");
	print2screen(HELP_X+12,HELP_Y+9,7,"increase position");
	print2screen(HELP_X+3,HELP_Y+10,10,"-");
	print2screen(HELP_X+12,HELP_Y+10,7,"decrease position");
	print2screen(HELP_X+3,HELP_Y+11,10,",");
	print2screen(HELP_X+12,HELP_Y+11,7,"shift actual position left");
	print2screen(HELP_X+3,HELP_Y+12,10,".");
	print2screen(HELP_X+12,HELP_Y+12,7,"shift actual position right");
	print2screen(HELP_X+3,HELP_Y+13,10,"G");
	print2screen(HELP_X+12,HELP_Y+13,7,"start animation");
	print2screen(HELP_X+3,HELP_Y+14,10,"H");
	print2screen(HELP_X+12,HELP_Y+14,7,"stop animation");
	print2screen(HELP_X+3,HELP_Y+15,10,"TAB");
	print2screen(HELP_X+12,HELP_Y+15,7,"toggle help");
}
