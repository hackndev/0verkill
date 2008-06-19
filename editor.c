#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "sprite.h"
#include "console.h"
#include "blit.h"
#include "data.h"
#include "cfg.h"
#include "hash.h"
#include "time.h"
#include "error.h"

/* defines for help */
#define HELP_WIDTH 78
#define HELP_HEIGHT 22
#define HELP_X ((SCREEN_X-2-HELP_WIDTH)>>1)
#define HELP_Y ((SCREEN_Y-2-HELP_HEIGHT)>>1)


unsigned long id;   /* id of last object */
int xoffset=0,yoffset=0; /* coordinates of upper left corner of the screen */
int x=0,y=0;  /* cursor coordinates on the screen */
int oldx=0,oldy=0;  /* old cursor position */

struct object_list objects;
struct object_list *last_obj;
int level_number;


void catch_signal(void)
{
	c_shutdown();
	exit(0);
}


void catch_sigsegv(void)
{
	c_shutdown();
	raise(SIGSEGV);
	exit(0);
}


void set_sigint(void)
{
	struct sigaction sa_old;
	struct sigaction sa_new;

	sa_new.sa_handler =(void*) catch_signal;
	sigemptyset(&sa_new.sa_mask);
	sa_new.sa_flags = 0;
	sigaction(SIGINT,&sa_new,&sa_old);
	sigaction(SIGQUIT,&sa_new,&sa_old);
	sigaction(SIGTERM,&sa_new,&sa_old);
	sigaction(SIGABRT,&sa_new,&sa_old);
	sigaction(SIGILL,&sa_new,&sa_old);
	sigaction(SIGFPE,&sa_new,&sa_old);
	sigaction(SIGBUS,&sa_new,&sa_old);

	sa_new.sa_handler =(void*) catch_sigsegv;
	sigemptyset(&sa_new.sa_mask);
	sa_new.sa_flags = 0;
	sigaction(SIGSEGV,&sa_new,&sa_old);
}


/* shows the help window */
void help(void)
{
	draw_frame(HELP_X,HELP_Y,HELP_WIDTH,HELP_HEIGHT,14);
	print2screen(HELP_X+(HELP_WIDTH-4>>1),HELP_Y+1,12,"HELP");
	print2screen(HELP_X+(HELP_WIDTH-9>>1),HELP_Y+3,11,"SCROLLING");
	
	/* cursor moving help */
	print2screen(HELP_X+8,HELP_Y+4,4,"CURSOR MOVING:");
	print2screen(HELP_X+13,HELP_Y+5,8,"UP");
	print2screen(HELP_X+12,HELP_Y+6,8,"[/\\]");
	print2screen(HELP_X+2,HELP_Y+7,8,"LEFT [<-] [\\/] [->] RIGHT");
	print2screen(HELP_X+12,HELP_Y+8,8,"DOWN");
	
	/* screen moving help */
	print2screen(HELP_X+31,HELP_Y+4,4,"SCROLLING (NUMPAD!):");
	print2screen(HELP_X+39,HELP_Y+5,8,"UP");
	print2screen(HELP_X+39,HELP_Y+6,8,"[8]");
	print2screen(HELP_X+31,HELP_Y+7,8,"LEFT [4]   [6] RIGHT");
	print2screen(HELP_X+39,HELP_Y+8,8,"[2]");
	print2screen(HELP_X+38,HELP_Y+9,8,"DOWN");

	/* item moving help */
	print2screen(HELP_X+60,HELP_Y+4,4,"ITEM MOVING:");
	print2screen(HELP_X+65,HELP_Y+5,8,"UP");
	print2screen(HELP_X+64,HELP_Y+6,8,"[W]");
	print2screen(HELP_X+55,HELP_Y+7,8,"LEFT [A] [S] [D] RIGHT");
	print2screen(HELP_X+64,HELP_Y+8,8,"DOWN");
	
	/* now the keys */
	print2screen(HELP_X+(HELP_WIDTH-4>>1),HELP_Y+11,11,"KEYS");
	print2screen(HELP_X+(HELP_WIDTH-11>>1),HELP_Y+12,8,"BUILD - [N]");
	print2screen(HELP_X+(HELP_WIDTH-12>>1),HELP_Y+13,8,"DELETE - [X]");
	print2screen(HELP_X+(HELP_WIDTH-19>>1),HELP_Y+14,8,"TYPE CHANGE - [TAB], [T] (+ [SHIFT] =reverse)");
	print2screen(HELP_X+(HELP_WIDTH-11>>1),HELP_Y+15,8,"SAVE - [F2]");
	print2screen(HELP_X+(HELP_WIDTH-11>>1),HELP_Y+16,8,"HELP - [F1]");
	print2screen(HELP_X+(HELP_WIDTH-26>>1),HELP_Y+17,8,"ITEM CHANGE - ([+] and [-])");

	blit_screen(1);
	sleep(2);
}



/* load static data */
void load_room(char * filename)
{
	FILE * stream;
	static char line[1024];
	char *p,*q,*name;
	int n,x,y,t;

	if (!(stream=fopen(filename,"rb")))
	{
		char msg[256];
		sprintf(msg,"Can't open file \"%s\"!\n",filename);
		ERROR(msg);
		EXIT(1);
	}
	while(fgets(line,1024,stream))
	{
		p=line;
		_skip_ws(&p);
		for (name=p;(*p)!=' '&&(*p)!=9&&(*p)!=10&&(*p);p++);
		if (!(*p))continue;
		*p=0;p++;
		_skip_ws(&p);
		t=*p;
		p++;
		_skip_ws(&p);
		x=strtol(p,&q,0);
		_skip_ws(&q);
		y=strtol(q,&p,0);
		if (find_sprite(name,&n))
		{
			char msg[256];
			sprintf(msg,"Unknown bitmap name \"%s\"!\n",name);
			ERROR(msg);
			EXIT(1);
		}
		new_obj(id,t,0,n,0,0,x,y,0,0,0);
		id++;
	}
	fclose(stream);
}



/* draw view */
void draw_scene(void)
{
	struct object_list *p;
	
	for(p=&objects;p->next;p=p->next)
	{

		p->next->member.anim_pos++;
		p->next->member.anim_pos%=sprites[p->next->member.sprite].n_steps;
		put_sprite(
			p->next->member.x-xoffset,
			p->next->member.y-yoffset,
			sprites[p->next->member.sprite].positions+sprites[p->next->member.sprite].steps[p->next->member.anim_pos],0
			);
	}
}


/* find object under cursor */
struct it * find_object(void)
{
	struct object_list *p;
#define O p->next->member
	
	for(p=&objects;p->next;p=p->next)
	{
		if (
			O.x<=x+xoffset&&
			O.x+sprites[O.sprite].positions[0].lines[0].len-1>=x+xoffset&&
			O.y<=y+yoffset&&
			O.y+sprites[O.sprite].positions[0].n-1>=y+yoffset
			)
			return &(p->next->member);
	}
	return 0;
}


/* next type when 't' is pressed */
int next_type(int type)
{
/* static objects: b, f, j, i, w */
/* dynamic objects: N, F, B, S, U, R, 1, 2, 3, 4, 5, M, A, I, K */
	switch(type)
	{
		case 'b': return 'f';
		case 'f': return 'j';
		case 'j': return 'i';
		case 'i': return 'w';
		case 'w': return 'N';
		case 'N': return 'F';
		case 'F': return 'B';
		case 'B': return 'S';
		case 'S': return 'U';
		case 'U': return 'R';
		case 'R': return '1';
		case '1': return '2';
		case '2': return '3';
		case '3': return '4';
		case '4': return '5';
		case '5': return 'M';
		case 'M': return 'A';
		case 'A': return 'I';
		case 'I': return 'K';

		default: return 'b';
	}
}


/* prev type when 'T' is pressed */
int prev_type(int type)
{
/* static objects: b, f, j, i, w */
/* dynamic objects: N, F, B, S, U, R, 1, 2, 3, 4, 5, M, A, I, K */
/* but now in reverse order */
	switch(type)
	{
		case 'b': return 'K';
		case 'f': return 'b';
		case 'j': return 'f';
		case 'i': return 'j';
		case 'w': return 'i';
		case 'N': return 'w';
		case 'F': return 'N';
		case 'B': return 'F';
		case 'S': return 'B';
		case 'U': return 'S';
		case 'R': return 'U';
		case '1': return 'R';
		case '2': return '1';
		case '3': return '2';
		case '4': return '3';
		case '5': return '4';
		case 'M': return '5';
		case 'A': return 'M';
		case 'I': return 'A';

		default: return 'I';
	}
}


/* save level */
void save_data(void)
{
	FILE *data, *dynamic;
	struct object_list *p;
	static unsigned char txt[1024];
	unsigned char *LEVEL=load_level(level_number);

	snprintf(txt,1024,"%s%s%s",DATA_PATH,LEVEL,STATIC_DATA_SUFFIX);
	data=fopen(txt,"w");
	if (!data)
	{
		unsigned char msg[256];
		snprintf(msg,256,"Can't create file \"%s\"\n",txt);
		ERROR(msg);
		EXIT(1);
	}
	
	snprintf(txt,1024,"%s%s%s",DATA_PATH,LEVEL,DYNAMIC_DATA_SUFFIX);
	mem_free(LEVEL);
	dynamic=fopen(txt,"w");
	if (!dynamic)
	{
		unsigned char msg[256];
		snprintf(msg,256,"Can't create file \"%s\"\n",txt);
		ERROR(msg);
		EXIT(1);
	}
	
#define O p->next->member
	
	for(p=&objects;p->next;p=p->next)
	{
		sprintf(txt,"%s %c %d %d\n",sprite_names[O.sprite],O.type,O.x,O.y);
		switch (O.type)
		{
			case 'b':
			case 'w':
			case 'i':
			case 'j':
			case 'f':
			fputs(txt,data);
			break;

			default:
			fputs(txt,dynamic);
			break;
		}
	}
	fclose(data);
	fclose(dynamic);
}



/* change size of the screen */
void sigwinch_handler(int signum)
{
	signum=signum;
	resize_screen();
#ifdef WIN32
	c_shutdown();
	c_init(SCREEN_X,SCREEN_Y);
#endif
	c_cls();
	c_refresh();
#ifndef WIN32
#ifdef SIGWINCH
	signal(SIGWINCH,sigwinch_handler);
#endif
#endif
}


void clear_memory(void)
{
	struct object_list *o;

	for (o=&objects;o->next;)
		delete_obj(o->next->member.id);

	free_area();
	free_sprites(0);
	shutdown_sprites();
}


int main(int argc, char**argv)
{
	int update=1;
	unsigned char bg=' ';
	unsigned char color=0;
	
	unsigned int cur_obj=0;
	unsigned char cur_type='w';
	
	struct it* obj=0;
	unsigned char txt[256];
	unsigned long_long last_time;
	unsigned char *LEVEL;

	set_sigint();

	while(1)
	{
		int a;
		char *c;
		
		a=getopt(argc, argv, "hl:");
		switch(a)
		{
			case EOF:
			goto done;

			case '?':
			case ':':
			exit(1);

			case 'h':
			printf(
				"0verkill editor\n"
				"Usage: editor [-h] [-l <level number>]\n"
				);
			exit(0);

			case 'l':
			level_number=strtoul(optarg,&c,10);
			if (*c){ERROR("Error: Not a number.\n");EXIT(1);}
			if (errno==ERANGE)
			{
				if (!level_number){ERROR("Error: Number underflow.\n");EXIT(1);}
				else {ERROR("Error: Number overflow.\n");EXIT(1);}
			}
			break;
		}
	}
done:
	
	last_obj=&objects;

#ifdef SIGWINCH
	signal(SIGWINCH,sigwinch_handler);
#endif
	c_get_size(&SCREEN_X,&SCREEN_Y);
	hash_table_init();
	init_sprites();
	init_area();
	LEVEL=load_level(level_number);
	if (!LEVEL){fprintf(stderr,"Can't load level number %d\n",level_number);exit(1);}
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,LEVEL_SPRITES_SUFFIX);
	load_sprites(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,STATIC_DATA_SUFFIX);
	load_room(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,DYNAMIC_DATA_SUFFIX);
	mem_free(LEVEL);
	load_room(txt);

	c_init(SCREEN_X,SCREEN_Y);
	c_cursor(C_HIDE);
	update=1;
again:
	last_time=get_time();

	clear_screen();
	draw_scene();
	blit_screen(1);
	
	if (!update)
	{
		c_goto(oldx,oldy);
		c_setcolor(color);
		c_putc(bg?bg:' ');
	}
	color=screen_a[x+y*SCREEN_X];
	bg=screen[x+y*SCREEN_X];

	c_goto(x,y);
	c_setcolor(15);
	c_print("+");

	obj=find_object();
	c_clear(0,SCREEN_Y-1,SCREEN_X-2,SCREEN_Y-1);
	c_goto(0,SCREEN_Y-1);
	c_setcolor(11);
	c_print("X: ");
	sprintf(txt,"% 4d   ",x+xoffset);
	c_setcolor(7);
	c_print(txt);
	c_setcolor(11);
	c_print("Y: ");
	sprintf(txt,"% 4d   ",y+yoffset);
	c_setcolor(7);
	c_print(txt);
	c_setcolor(11);
	c_print("OBJECT: ");
	sprintf(txt,"%-.20s   ",obj?(char*)(sprite_names[obj->sprite]):"----");
	c_setcolor(7);
	c_print(txt);
	c_setcolor(11);
	c_print("TYPE: ");
	sprintf(txt,"%c   ",obj?obj->type:'-');
	c_setcolor(7);
	c_print(txt);
	update=0;
	oldx=x;
	oldy=y;
	c_refresh();
	
	c_update_kbd();

	if (c_pressed(K_NUM4)||c_pressed('4'))
	{
		xoffset-=SCREEN_X>>2;
		if (xoffset<0)xoffset=0;
		else update=1;
	}
		
	if (c_pressed(K_NUM6)||c_pressed('6'))
	{
		xoffset+=SCREEN_X>>2;
		if (xoffset>AREA_X-1-(SCREEN_X>>1))xoffset=AREA_X-1-(SCREEN_X>>1);
		update=1;
	}
		
	if (c_pressed(K_NUM8)||c_pressed('8'))
	{
		yoffset-=SCREEN_Y>>2;
		if (yoffset<0)yoffset=0;
		update=1;
	}
		
	if (c_pressed(K_NUM2)||c_pressed('2'))
	{
		yoffset+=SCREEN_Y>>2;
		if (yoffset>AREA_Y-1-(SCREEN_Y>>1))yoffset=AREA_Y-1-(SCREEN_Y>>1);
		update=1;
	}
		
	if (c_pressed('q')||c_pressed(K_ESCAPE))
	{
		c_shutdown();
		clear_memory();
		check_memory_leaks();
		exit(0);
	}
	
	if (c_pressed(K_RIGHT))
	{
		x++;
		if (x>SCREEN_X-1||x+xoffset>AREA_X-1)x=0;
	}

	if (c_pressed(K_LEFT))
	{
		x--;
		if (x<0)x=SCREEN_X-1;
		if (x+xoffset>AREA_X-1)x=AREA_X-1-xoffset;
	}

	if (c_pressed(K_DOWN))
	{
		y++;
		if (y>SCREEN_Y-1||y+yoffset>AREA_Y-1)y=0;
	}

	if (c_pressed(K_UP))
	{
		y--;
		if (y<0)y=SCREEN_Y-1;
		if (y+yoffset>AREA_Y-1)y=AREA_Y-1-yoffset;
	}

	if (c_pressed(K_F1))
	{
		help();
		clear_screen();
		draw_scene();
		blit_screen(1);
	}

	if (c_was_pressed('x'))
		if (obj)
		{
			delete_obj(obj->id);
			update=1;
		}

	if (c_was_pressed('=')||c_was_pressed(K_NUM_PLUS)|c_was_pressed(K_PGUP))
		if (obj)
		{
			obj->sprite++;
			cur_obj=obj->sprite;
			obj->sprite%=n_sprites;
			update=1;
		}
	
	if (c_was_pressed('a')||((c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))&&c_pressed('a')))
		if (obj)
		{
			obj->x--;
			x--;
			if (obj->x<0)obj->x=0;
			update=1;
		}

	if (c_was_pressed('d')||((c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))&&c_pressed('d')))
		if (obj)
		{
			obj->x++;
			x++;
			if (obj->x>AREA_X-1)obj->x=AREA_X-1;
			update=1;
		}

	if (c_was_pressed('w')||((c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))&&c_pressed('w')))
		if (obj)
		{
			obj->y--;
			y--;
			if (obj->y<0)obj->y=0;
			update=1;
		}

	if (c_was_pressed('s')||((c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))&&c_pressed('s')))
		if (obj)
		{
			obj->y++;
			y++;
			if (obj->y>AREA_Y-1)obj->y=AREA_Y-1;
			update=1;
		}
	
	if (c_was_pressed('-')||c_was_pressed(K_NUM_MINUS)||c_was_pressed(K_PGDOWN))
		if (obj)
		{
			if (!obj->sprite)obj->sprite=n_sprites-1;
			else obj->sprite--;
			cur_obj=obj->sprite;
			update=1;
		}

	if ((c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))&&(c_was_pressed('t')||c_was_pressed(K_TAB)))
		if (obj)
		{
			obj->type=prev_type(obj->type);
			cur_type=obj->type;
			update=1;
		}

	if (!c_pressed(K_LEFT_SHIFT)&&!c_pressed(K_RIGHT_SHIFT)&&(c_was_pressed('t')||c_was_pressed(K_TAB)))
		if (obj)
		{
			obj->type=next_type(obj->type);
			cur_type=obj->type;
			update=1;
		}

	if (c_was_pressed('n')||c_was_pressed(K_ENTER))
	{
		new_obj(id,cur_type,0,cur_obj,0,0,x+xoffset,y+yoffset,0,0,0);
		id++;
		update=1;
	}

	if (c_was_pressed(K_F2))
		save_data();

	sleep_until(last_time+1000000/35);
	goto again;
}
