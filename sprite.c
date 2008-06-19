#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include "config.h"
#endif

#include "sprite.h"
#include "blit.h"
#include "data.h"
#include "console.h"
#include "cfg.h"
#include "error.h"



/* allocate memory for screenbuffer */
void init_sprites(void)
{
	init_blit();
}

void shutdown_sprites(void)
{
	shutdown_blit();
}


/* write visible window of playing area into screenbuffer */
void show_window(int x,int y)
{
	int a,loffs,roffs;
	int prec2=y*AREA_X;
 
	if (x+SCREEN_X<=0)return;
	if (y+SCREEN_Y<=0)return;

	loffs=x<0?-x:0;
	roffs=x+SCREEN_X>AREA_X?x+SCREEN_X-AREA_X:0;
#ifdef TRI_D
	switch (tri_d)
	{
		case 0:  /* normal part */
		for(a=(y<0?-y:0);a<SCREEN_Y&&(y+a)<AREA_Y;a++)
		{
			int prec=a*SCREEN_X;
			int prec1=a*AREA_X;

			memcpy(screen+prec+loffs,area+loffs+x+prec2+prec1,SCREEN_X-loffs-roffs);
			memcpy(screen_a+prec+loffs,area_a+loffs+x+prec2+prec1,SCREEN_X-loffs-roffs);
		}
		break;

		case 1:  /* 3d part */
		memset(screen2_a,0,SCREEN_X*SCREEN_Y);
		memset(screen2,0,SCREEN_X*SCREEN_Y);
		for(a=(y<0?-y:0);a<SCREEN_Y&&(y+a)<AREA_Y;a++)
		{
			int b;
			int prec=a*SCREEN_X;
			int prec1=a*AREA_X;

			for (b=0;b<SCREEN_X-loffs-roffs;b++)
			{
				int offs;
				int prec4=loffs+x+prec2+prec1+b;

				switch((area_a[prec4])&240)
				{
					case TYPE_BACKGROUND:
					offs=-1;
					break;

					case TYPE_FOREGROUND:
					case TYPE_JUMP_FOREGROUND:
					offs=1;
					break;

					default:
					offs=0;
					break;
				}
				
				if (offs!=1&&(!area[prec4]||area[prec4]==' '))continue;

				if ((b+offs+loffs)<0||(loffs+b+offs)>=SCREEN_X)continue;
				screen2[prec+loffs+b+offs]=area[prec4];
				screen2_a[prec+loffs+b+offs]=area_a[prec4];
			}
		}
		break;
	}
#else
	for(a=(y<0?-y:0);a<SCREEN_Y&&(y+a)<AREA_Y;a++)
	{
		int prec=a*SCREEN_X;
		int prec1=a*AREA_X;

		memcpy(screen+prec+loffs,area+loffs+x+prec2+prec1,SCREEN_X-loffs-roffs);
		memcpy(screen_a+prec+loffs,area_a+loffs+x+prec2+prec1,SCREEN_X-loffs-roffs);
	}
#endif
}


/* write one scanline of a bitmap to given memory */
#ifdef HAVE_INLINE
	inline void 
#else
	void
#endif
put_line(int xs, int ys,unsigned char *scrn,unsigned char *attr,int x,int y,struct line *l,unsigned char type,unsigned char plastic)
{
	int a,b=xs*y;
	if (y>=ys||y<0)return;
	for (a=0;a<l->len&&x+a<xs;a++)
	{
		if (!l->attr[a])continue;
		if (	plastic&&
			((attr[x+a+b]&240)==TYPE_FOREGROUND||
			(attr[x+a+b]&240)==TYPE_JUMP_FOREGROUND)
		    ) continue;
		if (x+a<0)continue;
		scrn[x+a+b]=l->bmp[a];
		attr[x+a+b]=(l->attr[a])|type;
	}
}

/* put image to given memory */
#ifdef HAVE_INLINE
	inline void 
#else
	void
#endif
_put_sprite(int xs, int ys, unsigned char *scrn,unsigned char *attr,int x,int y,struct pos *p,unsigned char type,unsigned char plastic)
{
	int a;
	for (a=0;a<p->n;a++)
		put_line(xs,ys,scrn,attr,x+p->xo,a+y+p->yo,p->lines+a,type,plastic);
}

/* put image into screenbuffer */
#ifdef HAVE_INLINE
	inline void
#else
	void 
#endif
put_sprite(int x, int y, struct pos *p,unsigned char plastic)
{
	unsigned char *sc,*at;
#ifdef TRI_D
	sc=(TRI_D_ON&&tri_d)?screen2:screen;
	at=(TRI_D_ON&&tri_d)?screen2_a:screen_a;
#else
	sc=screen;
	at=screen_a;
#endif
	_put_sprite(SCREEN_X,SCREEN_Y,sc,at,x,y,p,0,plastic);
}


/* ancillary function for converting color from file notation to value 0-15 */
int _conv_color(int c)
{
	int a=tolower(c);
	if (a>='a'&&a<='f')return 10+a-'a';
	if (a>='0'&&a<='9')return a-'0';
	/* space or other character is blank attr */
	return 0;
}


/* load sprite from a file */
void load_sprite(unsigned char * filename,struct sprite *s)
{
#define CURP (s->n_positions-1)
#define CURL (s->positions[CURP].n-1)
	FILE *f;
	int x,a;
	static unsigned char buffer[8192];
	char *p,*q;
	int step=0;  /*0=expecting 'p', 1=expecting 'l', 2=expecting 'a', 3=expecting 'p' or 's' or 'l', 4=expecting end */

	s->positions=DUMMY;
	s->n_positions=0;
 
	if (!(f=fopen(filename,"rb")))
	{
		unsigned char msg[256];
		snprintf(msg,256,"Error opening file \"%s\"!\n",filename);
		ERROR(msg);
		EXIT(1);
	}
	while(fgets(buffer,8191,f))
	{
		x=strlen(buffer);
		if (buffer[x-1]==10)buffer[x-1]=0;
		switch(*buffer)
		{
			case 0:
			goto skip;

			case '#': /* comment */
			break;

			case 'p':
			if (step!=0&&step!=3)
			{
				unsigned char msg[256];
				snprintf(msg,256,"Syntax error in file \"%s\".\n",filename);
				ERROR(msg);
				EXIT(1);
			}
			step=1;
			s->n_positions++;
			s->positions=mem_realloc(s->positions,s->n_positions*sizeof(struct pos));
			if (!s->positions)
			{
				ERROR("Memory allocation error!\n");
				EXIT(1);
			}
			s->positions[CURP].n=0;
			s->positions[CURP].lines=0;
			s->n_steps=0;
			s->steps=0;
			s->positions[CURP].xo=strtol(buffer+1,&p,0);
			s->positions[CURP].yo=strtol(p+1,&q,0);
			break;
   
			case 'l':
			if (step!=1&&step!=3)
			{
				unsigned char msg[256];
				snprintf(msg,256,"Syntax error in file \"%s\".\n",filename);
				ERROR(msg);
				EXIT(1);
			}
			step=2;
			if (!(s->positions[CURP].n))s->positions[CURP].lines=DUMMY;
			s->positions[CURP].n++;
			s->positions[CURP].lines=mem_realloc(s->positions[CURP].lines,s->positions[CURP].n*sizeof(struct line));
			if (!s->positions[CURP].lines)
			{
				ERROR("Memory allocation error!\n");
				EXIT(1);
			}
			s->positions[CURP].lines[CURL].len=strlen(buffer+1);
			if (!s->positions[CURP].lines[CURL].len)break;  /* empty line, we have nothing to do */
			s->positions[CURP].lines[CURL].bmp=mem_alloc(s->positions[CURP].lines[CURL].len);
			if (!s->positions[CURP].lines[CURL].bmp)
			{
				ERROR("Memory allocation error!\n");
				EXIT(1);
			}
			memcpy(s->positions[CURP].lines[CURL].bmp,buffer+1,s->positions[CURP].lines[CURL].len);
			break;
   
			case 'a':
			if (step!=2)
			{
				unsigned char msg[256];
				snprintf(msg,256,"Syntax error in file \"%s\".\n",filename);
				ERROR(msg);
				EXIT(1);
			}
			step=3;
			/* UGH, co to tady je??????? */
			if (!s->positions[CURP].lines)
			{
				ERROR("Memory allocation error!\n");
				EXIT(1);
			}
			if (!s->positions[CURP].lines[CURL].len)break;  /* empty line, we have nothing to do */
			s->positions[CURP].lines[CURL].attr=mem_alloc(s->positions[CURP].lines[CURL].len);
			if (!s->positions[CURP].lines[CURL].attr)
			{
				ERROR("Memory allocation error!\n");
				EXIT(1);
			}
			x=strlen(buffer+1);
			if (x>s->positions[CURP].lines[CURL].len)x=s->positions[CURP].lines[CURL].len;
			for (a=0;a<x;a++)
				s->positions[CURP].lines[CURL].attr[a]=_conv_color(buffer[1+a]);
			memset(s->positions[CURP].lines[CURL].attr+x,0,s->positions[CURP].lines[CURL].len-x);
			break;
   
			case 's':
			if (step!=3)
			{
				unsigned char msg[256];
				snprintf(msg,256,"Syntax error in file \"%s\".\n",filename);
				ERROR(msg);
				EXIT(1);
			}
			step=4;
			for(q=buffer,p=buffer+1;*q&&*q!=10;*q==','?p=q+1:0)
			{
				x=strtol(p,&q,0);
				if (x<0||x>CURP)
				{
					unsigned char txt[256];

					snprintf(txt,256,"Error loading sprite \"%s\". Undefined position %d.\n",filename,x);
					ERROR(txt);
					EXIT(1);
				}
				if (!(s->n_steps))s->steps=DUMMY;
				s->n_steps++;
				s->steps=mem_realloc(s->steps,s->n_steps*sizeof(unsigned short));
				if (!s->steps)
				{
					ERROR("Memory allocation error!\n");
					EXIT(1);
				}
				s->steps[s->n_steps-1]=x;
			}
			break;

			default:
			{
				unsigned char msg[256];
				snprintf(msg,256,"Syntax error in file \"%s\"!\n",filename);
				ERROR(msg);
				EXIT(1);
			}
		}
skip:  ;
	}
	if (step!=4)
	{
		unsigned char msg[256];
		snprintf(msg,256,"Unexpected end of file in \"%s\".\n",filename);
		ERROR(msg);
		EXIT(1);
	}
	fclose(f);
}


static void free_line(struct line* line)
{
	if(line->len)
	{
		mem_free(line->attr);
		mem_free(line->bmp);
	}
}


static void free_pos(struct pos *pos)
{
	int a;
	for (a=0;a<(pos->n);a++)
		free_line((pos->lines)+a);
	mem_free(pos->lines);
}


void free_sprite(struct sprite*sp)
{
	int a;

	for (a=0;a<(sp->n_positions);a++)
		free_pos((sp->positions)+a);
	
	if (sp->n_steps){mem_free(sp->steps);mem_free(sp->positions);}
}
