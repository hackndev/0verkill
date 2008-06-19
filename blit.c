/* screenbuffer functions */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include "config.h"
#endif
#include "console.h"
#include "cfg.h"
#include "error.h"


/* screen dimensions */
int SCREEN_X,SCREEN_Y;

int SCREEN_XOFFSET,SCREEN_YOFFSET;


/* screenbuffer */
unsigned char *screen;
unsigned char *screen_old;

#ifdef TRI_D
unsigned char *screen2;
unsigned char *screen2_old;
#endif



/* attributes:
	lower 4 bits=foreground
	bits 4-7=background
*/
unsigned char *screen_a;
unsigned char *screen_a_old;

#ifdef TRI_D
unsigned char *screen2_a;
unsigned char *screen2_a_old;
#endif


/* allocate memory for screenbuffer */
/* requires SCREEN_X and SCREEN_Y as screen size */
void init_blit(void)
{
	screen=mem_alloc(SCREEN_X*SCREEN_Y);
	if (!screen){ERROR("Not enough memory!\n");EXIT(1);}
	screen_a=mem_alloc(SCREEN_X*SCREEN_Y);
	if (!screen_a){ERROR("Not enough memory!\n");EXIT(1);}
	screen_old=mem_alloc(SCREEN_X*SCREEN_Y);
	if (!screen_old){ERROR("Not enough memory!\n");EXIT(1);}
	screen_a_old=mem_alloc(SCREEN_X*SCREEN_Y);
	if (!screen_a_old){ERROR("Not enough memory!\n");EXIT(1);}
	memset(screen_a_old,0,SCREEN_X*SCREEN_Y);
	memset(screen_old,0,SCREEN_X*SCREEN_Y);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		screen2=mem_alloc(SCREEN_X*SCREEN_Y);
		if (!screen2){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_a=mem_alloc(SCREEN_X*SCREEN_Y);
		if (!screen2_a){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_old=mem_alloc(SCREEN_X*SCREEN_Y);
		if (!screen2_old){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_a_old=mem_alloc(SCREEN_X*SCREEN_Y);
		if (!screen2_a_old){ERROR("Not enough memory!\n");EXIT(1);}
		memset(screen2_a_old,0,SCREEN_X*SCREEN_Y);
		memset(screen2_old,0,SCREEN_X*SCREEN_Y);
	}
#endif
	
        SCREEN_XOFFSET=(SCREEN_X-PLAYER_WIDTH)>>1;
        SCREEN_YOFFSET=(SCREEN_Y-PLAYER_HEIGHT)>>1;
}

void shutdown_blit(void)
{
	mem_free(screen);
	mem_free(screen_a);
	mem_free(screen_old);
	mem_free(screen_a_old);
}


void clear_screen(void)
{
	memset(screen_a,0,SCREEN_X*SCREEN_Y);
	memset(screen,0,SCREEN_X*SCREEN_Y);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		memset(screen2_a,0,SCREEN_X*SCREEN_Y);
		memset(screen2,0,SCREEN_X*SCREEN_Y);
	}
#endif
}


/* resize and clear screen buffers to new dimensions stored in SCREEN_X and SCREEN_Y*/
void resize_screen(void)
{
	c_get_size(&SCREEN_X,&SCREEN_Y);
	screen=mem_realloc(screen,SCREEN_X*SCREEN_Y);
	if (!screen){ERROR("Not enough memory!\n");EXIT(1);}
	screen_a=mem_realloc(screen_a,SCREEN_X*SCREEN_Y);
	if (!screen_a){ERROR("Not enough memory!\n");EXIT(1);}
	screen_old=mem_realloc(screen_old,SCREEN_X*SCREEN_Y);
	if (!screen_old){ERROR("Not enough memory!\n");EXIT(1);}
	screen_a_old=mem_realloc(screen_a_old,SCREEN_X*SCREEN_Y);
	if (!screen_a_old){ERROR("Not enough memory!\n");EXIT(1);}
	memset(screen_a_old,0,SCREEN_X*SCREEN_Y);
	memset(screen_old,0,SCREEN_X*SCREEN_Y);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		screen2=mem_realloc(screen2,SCREEN_X*SCREEN_Y);
		if (!screen2){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_a=mem_realloc(screen2_a,SCREEN_X*SCREEN_Y);
		if (!screen2_a){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_old=mem_realloc(screen2_old,SCREEN_X*SCREEN_Y);
		if (!screen2_old){ERROR("Not enough memory!\n");EXIT(1);}
		screen2_a_old=mem_realloc(screen2_a_old,SCREEN_X*SCREEN_Y);
		if (!screen2_a_old){ERROR("Not enough memory!\n");EXIT(1);}
		memset(screen2_a_old,0,SCREEN_X*SCREEN_Y);
		memset(screen2_old,0,SCREEN_X*SCREEN_Y);
	}
#endif
	
	SCREEN_XOFFSET=(SCREEN_X-PLAYER_WIDTH)>>1;
	SCREEN_YOFFSET=(SCREEN_Y-PLAYER_HEIGHT)>>1;

	clear_screen();
}


void redraw_screen(void)
{
	memset(screen_a_old,0,SCREEN_X*SCREEN_Y);
	memset(screen_old,0,SCREEN_X*SCREEN_Y);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		memset(screen2_a_old,0,SCREEN_X*SCREEN_Y);
		memset(screen2_old,0,SCREEN_X*SCREEN_Y);
	}
#endif

	clear_screen();
}


/* print text into screen buffer on given position, with given color */
void print2screen(int x,int y,unsigned char color,char *message)
{
	int offs=x+SCREEN_X*y;
	int l=strlen(message);

	color&=15;

	if (y>=SCREEN_Y||x>=SCREEN_X)return;
	if (l+offs>SCREEN_X*SCREEN_Y)l=SCREEN_X*SCREEN_Y-offs;
	
	memset(screen_a+offs,color,l);
	memcpy(screen+offs,message,l);

#ifdef TRI_D
	if (TRI_D_ON)
	{
		memset(screen2_a+offs,color,l);
		memcpy(screen2+offs,message,l);
	}
#endif
}


/* draw screenbuffer to the screen */
#ifdef HAVE_INLINE
	inline void 
#else
	void
#endif
blit_screen(unsigned char ignore_bg)
{
	int x,y;
	int yof;
	unsigned char a;
	int changed=1;
	int status_flag;
	unsigned char *sc, *sc_old, *at, *at_old;
	unsigned char last_color;
	unsigned char attribute,attribute_old;
	
#ifdef TRI_D
	sc=(TRI_D_ON&&tri_d)?screen2:screen;
	sc_old=(TRI_D_ON&&tri_d)?screen2_old:screen_old;
	at=(TRI_D_ON&&tri_d)?screen2_a:screen_a;
	at_old=(TRI_D_ON&&tri_d)?screen2_a_old:screen_a_old;
#else
	sc=screen;
	sc_old=screen_old;
	at=screen_a;
	at_old=screen_a_old;
#endif

	last_color=*at;
	if (ignore_bg)last_color&=15;
	c_setcolor_bg(last_color,(last_color)>>4);
	for (y=0;y<SCREEN_Y;y++)
	{
		c_goto(0,y);
		yof=SCREEN_X*y;
		for (x=0;x<SCREEN_X;x++)
		{ 
			attribute=at[x+yof];
			attribute_old=at_old[x+yof];
			if (ignore_bg){attribute&=15;attribute_old&=15;}

			if (x == SCREEN_X - 1 && y == SCREEN_Y - 1) break;
			if (sc[x+yof]==sc_old[x+yof]&&attribute==attribute_old){changed=0;continue;}
			if (!changed)c_goto(x,y);
			changed=1;

			a=(last_color^attribute);
			status_flag=	(!!(a&7))|   /* foreground */
					(((a>>3)&1)<<1)|   /* foreground highlight */
					((!!((a>>4)&7))<<2);  /* background */

			switch(status_flag)  /* what changed: */
			{
				case 0:  /* nothing */
				break;

				case 1:  /* foreground */
				c_setcolor_3b(attribute);
				break;

				case 6:  /* higlight and background */
				case 2:  /* highlight */
				if (attribute&8)  /* hlt on */
					c_sethlt_bg((attribute>>3)&1,attribute>>4);
				else
					c_setcolor_bg(attribute,attribute>>4);
				break;

				case 7: /* background, foreground and highlight */
				case 3: /* foreground and higlight */
				c_setcolor_bg(attribute,attribute>>4);
				break;
				
				case 4:  /* background */
				c_setbgcolor(attribute>>4);
				break;

				case 5:  /* background and foreground */
				c_setcolor_3b_bg(attribute,attribute>>4);
				break;
			}
					
			last_color=attribute;
			c_putc(sc[x+SCREEN_X*y]?sc[x+SCREEN_X*y]:32);
		}
	}
	memcpy(sc_old,sc,SCREEN_X*SCREEN_Y);
	memcpy(at_old,at,SCREEN_X*SCREEN_Y);
	c_refresh();
}


/* draw frame to the screen */
/* x,y = upper left corner */
/* w,h = width and height of the content */
void draw_frame(int x,int y,int w,int h,unsigned char color)
{
	int a;
	char *t;
	
	color&=15;


	t=mem_alloc(w+3);
	if (!t)return;
	t[w+2]=0;
	t[w]=0;
	t[w-1]=0;
	print2screen(x,y,color,"+");
	print2screen(x+w+1,y,color,"+");
	print2screen(x+w+1,y+h+1,color,"+");
	print2screen(x,y+h+1,color,"+");
	for (a=0;a<w;a++)
		t[a]='-';
	print2screen(x+1,y,color,t);
	print2screen(x+1,y+h+1,color,t);
	memset(t,' ',w+1);
	t[0]='|';
	t[w+1]='|';
	for (a=1;a<=h;a++)
		print2screen(x,y+a,color,t);
	mem_free(t);
}
