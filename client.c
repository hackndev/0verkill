/* When compiling with X support don't forget to define XWINDOW symbol */

#ifndef WIN32
#include "config.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <math.h>

#ifdef HAVE_FLOAT_H
	#include <float.h>
#endif

#ifdef HAVE_SIGINFO_H
	#include <siginfo.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <ctype.h>

#if (!defined(WIN32))
	#ifdef TIME_WITH_SYS_TIME
		#include <sys/time.h>
		#include <time.h>
	#else
		#ifdef TM_IN_SYS_TIME
			#include <sys/time.h>
		#else
			#include <time.h>
		#endif
	#endif
#else
	#include <time.h>
#endif

#include <errno.h>
#include <stdlib.h>
#ifndef WIN32
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#ifndef __USE_GNU
		#define __USE_GNU
	#endif
#else
	#include <winsock.h>
#endif

#include <string.h>

#include "blit.h"
#include "sprite.h"
#include "data.h"
#include "cfg.h"
#include "net.h"
#include "hash.h"
#include "console.h"
#include "time.h"
#include "math.h"
#include "help.h"
#include "getopt.h"
#include "error.h"

#ifdef XWINDOW
	#include "x.h"
#endif

#undef PAUSE

#ifdef WIN32
	int consoleApp=1;
#endif

/* my health, armor, frags, deaths, ammo, ... and ID */
unsigned char health,armor;
unsigned int frags,deaths;
unsigned short ammo[ARMS];
unsigned char current_weapon;
unsigned char weapons;
unsigned char creep;
unsigned char autorun,autocreep;
int my_id;

/* connection with server */
int connected;

int level_number=-1;

/* networking */
int fd;  /* socket */
struct sockaddr_in server;  /* server address */

/* objects */
struct object_list objects;
struct object_list *last_obj;
struct it* hero;

/* important sprites */
int hit_sprite;
int title_sprite;
int bulge_sprite;
int shrapnel_sprite[N_SHRAPNELS];

int level_sprites_start;  /* number of first sprite in the level (all other level sprites follow) */

unsigned long_long game_start_offset; /* time difference between game start on this machine and on server */


/* top players table */
struct
{
	char name[MAX_NAME_LEN+1];
	int frags,deaths;
	unsigned char color;
}top_players[TOP_PLAYERS_N];

/* # of active players in the game */
int active_players;


struct  /* keyboard status */
{
	unsigned char right,left,jump,creep,speed,fire,weapon,down_ladder;
}keyboard_status;

/* message */
struct msgline_type
{
	unsigned long_long time;
	char *msg;
}msg_line[N_MESSAGES];

int last_message;
unsigned char error_message[1024];

unsigned char set_size;  /* -s option was on the command line */

/* convert key to key with shift */
int my_toupper(int c)
{
        switch(c)
        {
                case '1': return '!';
                case '2': return '@';
                case '3': return '#';
                case '4': return '$';
                case '5': return '%';
                case '6': return '^';
                case '7': return '&';
                case '8': return '*';
                case '9': return '(';
                case '0': return ')';
                case '-': return '_';
                case '=': return '+';
                case '\\': return '|';
                case '[': return '{';
                case ']': return '}';
                case ';': return ':';
                case '\'': return '"';
                case ',': return '<';
                case '.': return '>';
                case '/': return '?';
                case '`': return '~';
                default: return toupper(c);
        }
}



void wait_for_enter(void)
{
	c_update_kbd();
	while (!c_was_pressed(K_ENTER))
	{
		c_wait_for_key();
		c_update_kbd();
	}
}


/* load configure file from player's home directory */
void load_cfg(char *host,char *name,int *color)
{
	FILE *stream;
	int a;
	unsigned char txt[256];

#ifndef WIN32
	sprintf(txt,"%s/%s",getenv("HOME"),CFG_FILE);
#else
	sprintf(txt,"./%s",CFG_FILE);
#endif
	stream=fopen(txt,"r");
	if (!stream)return;
	
	if (!fgets(txt,MAX_HOST_LEN+2,stream))
		return;
	a=strlen(txt);
	if (txt[a-1]==10)txt[a-1]=0;
	memcpy(host,txt,strlen(txt)+1);
	
	if (!fgets(txt,MAX_NAME_LEN+2,stream))
		return;
	a=strlen(txt);
	if (txt[a-1]==10)txt[a-1]=0;
	memcpy(name,txt,strlen(txt)+1);
	if (!fgets(txt,256,stream))
		return;
	*color=strtol(txt,0,10);
	fclose(stream);
}


/* save configure file to player's home */
void save_cfg(char *host,char *name,int color)
{
	FILE *stream;
	unsigned char txt[256];

#ifndef WIN32
	sprintf(txt,"%s/%s",getenv("HOME"),CFG_FILE);
#else
	sprintf(txt,"./%s",CFG_FILE);
#endif
	stream=fopen(txt,"w");
	if (!stream)return;
	fprintf(stream,"%s\n%s\n%d\n",host,name,color);
	fclose(stream);
}


void scroll_messages(void)
{
	int a;

	if (last_message<0)return;
	for (a=0;a<=last_message-1;a++)
	{
		msg_line[a].time=msg_line[a+1].time;
		msg_line[a].msg=mem_realloc(msg_line[a].msg,strlen(msg_line[a+1].msg)+1);
		memcpy(msg_line[a].msg,msg_line[a+1].msg,strlen(msg_line[a+1].msg)+1);
	}
	mem_free(msg_line[last_message].msg);
	msg_line[last_message].msg=0;
	last_message--;
}


void add_message(char *message)
{
	if (last_message==N_MESSAGES-1)scroll_messages();
	msg_line[last_message+1].time=get_time()+MESSAGE_TTL;
	msg_line[last_message+1].msg=mem_alloc(strlen(message)+1);
	if (!msg_line[last_message+1].msg)return;	/* not such a fatal errror */
	memcpy(msg_line[last_message+1].msg,message,strlen(message)+1);
	last_message++;
}


void print_messages(void)
{
	int a;
	
	for (a=0;a<=last_message;a++)
		print2screen(0,a,MESSAGE_COLOR,msg_line[a].msg);
}


/* throw out old messages */
void update_messages(unsigned long_long time)
{
	int a,b;

	for(a=0,b=0;a<=last_message;a++)
		if (msg_line[b].time<=time)scroll_messages();
		else b++;
}


/* destroys all objects and messages */
void clean_memory(void)
{
	int a;
	struct object_list *o;
	
	/* delete all objects except hero */
	for (o=&objects;o->next;)
		if ((hero->id)!=(o->next->member.id))delete_obj(o->next->member.id);
		else o=o->next;

	/* delete messages */
	for (a=0;a<=last_message;a++)
		mem_free(msg_line[a].msg);
	msg_line[last_message].msg=0;
	last_message=-1;
}


/* shut down the client */
void shut_down(int x)
{
	struct object_list *o;
	int a;

	c_shutdown();
	free_sprites(0);
	free_area();
	if (hero)delete_obj(hero->id);

	
	/* delete all objects except hero */
	for (o=&objects;o->next;)
		delete_obj(o->next->member.id);

	/* delete messages */
	for (a=0;a<=last_message;a++)
		mem_free(msg_line[a].msg);
	msg_line[last_message].msg=0;

	shutdown_sprites();
	check_memory_leaks();
	if (x)EXIT(0);
}


/* find address of server and fill the server address structure */
char * find_server(char *name,unsigned short port)
{
	struct hostent *h;
	
	h=gethostbyname(name);
	if (!h)return "Error: Can't resolve server address.";

	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	server.sin_addr=*((struct in_addr*)(h->h_addr_list[0]));
	return 0;
}


/* initialize socket */
char * init_socket(void)
{
	fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(fd<0)return "Can't get socket.\n";
	return 0;
}


#define MAX_COUNT 32

/* send quit request to server */
void send_quit(void)
{
	unsigned char p;
	fd_set rfds;
	struct timeval tv;
	int a=sizeof(server);
	int count=0;

	tv.tv_sec=2;
	tv.tv_usec=0;
	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);

send_again:
	p=P_QUIT_REQUEST;
	count ++;
	send_packet(&p,1,(struct sockaddr *)(&server),my_id,0);
	if (!select(fd+1,&rfds,0,0,&tv)&&count<=MAX_COUNT)goto send_again;
	recv_packet(&p,1,(struct sockaddr*)(&server),&a,1,my_id,0);
	if (p!=P_PLAYER_DELETED&&count<=MAX_COUNT)goto send_again;
}

#undef MAX_COUNT


/* initiate connection with server */
char * contact_server(int color,unsigned char *name)
{
	static unsigned char packet[256];
	int l=strlen(name)+1;
	int a,r;
	int min,maj;

        fd_set fds;
        struct timeval tv;
        tv.tv_sec=4;
        tv.tv_usec=0;
        FD_ZERO(&fds);
        FD_SET(fd,&fds);

	packet[0]=P_NEW_PLAYER;
	packet[1]=0;
	packet[2]=VERSION_MAJOR;
	packet[3]=VERSION_MINOR;
	packet[4]=color;
	memcpy(packet+5,name,l);

	send_packet(packet,l+5,(struct sockaddr*)(&server),my_id,0);


        if (!select(fd+1,&fds,NULL,NULL,&tv))return "No reply within 4 seconds. Press ENTER.";

	if ((r=recv_packet(packet,256,0,0,1,0,0))<0)
	{
		if (errno==EINTR)return "Server hung up. Press ENTER.";
		else return "Connection error. Press ENTER.";
	}

	switch(*packet)
	{
		case P_PLAYER_REFUSED:
		switch(packet[1])
		{
			case E_INCOMPATIBLE_VERSION:
			return "Incompatible client version. Connection refused. Press Enter.";

			default:
			return "Connection refused. Press ENTER.";
		}
		
		case P_PLAYER_ACCEPTED:
		my_id=get_int(packet+33);
		if (r<39){send_quit();return "Incompatible server version. Givin' up. Press Enter.";}
		maj=packet[37];
		min=packet[38];
		if (maj!=VERSION_MAJOR||min<MIN_SERVER_VERSION_MINOR)
		{send_quit();return "Incompatible server version. Givin' up. Press Enter.";}
		game_start_offset=get_time();
		game_start_offset-=get_long_long(packet+25);
		health=100;
		armor=0;
		for(a=0;a<ARMS;a++)
			ammo[a]=0;
		ammo[0]=weapon[0].basic_ammo;
		current_weapon=0;
		weapons=17;  /* gun and grenades */
		hero=new_obj(
			get_int(packet+1),   /* ID */
			T_PLAYER,   /* type */
			0,  /* time to live */
			get_int16(packet+5),   /* sprite */
			0,        /* position */
			get_int16(packet+23),        /* status */
			get_float(packet+7),     /* X */
			get_float(packet+11),    /* Y */
			get_float(packet+15),    /* XSPEED */
			get_float(packet+19),    /* YSPEED */
			0
			);
		break;
		
		default:
		return "Connection error. Press ENTER.";
	}
	return 0;
}


/* send me top X players */
void send_info_request(void)
{
	unsigned char packet;
	packet=P_INFO;
	send_packet(&packet,1,(struct sockaddr*)(&server),my_id,0);
}


/* I want to be born again */
void send_reenter_game(void)
{
	unsigned char packet;
	packet=P_REENTER_GAME;
	send_packet(&packet,1,(struct sockaddr*)(&server),my_id,0);
}


/* send end of game to server */
void end_game(void)
{
	unsigned char packet;
	packet=P_END;
	send_packet(&packet,1,(struct sockaddr*)(&server),my_id,0);
}


/* send chat message */
void send_message(char *msg)
{
	static unsigned char packet[MAX_MESSAGE_LENGTH+2];
	int a;

	a=strlen(msg)+1;
	packet[0]=P_MESSAGE;
	memcpy(packet+1,msg,a);
	send_packet(packet,a+1,(struct sockaddr *)(&server),my_id,0);
}


/* send end of game to server */
void send_keyboard(void)
{
	unsigned char packet[3];
	packet[0]=P_KEYBOARD;
	packet[1]=	keyboard_status.right|
			(keyboard_status.left<<1)|
			(keyboard_status.jump<<2)|
			(keyboard_status.creep<<3)|
			(keyboard_status.speed<<4)|
			(keyboard_status.fire<<5)|
			(keyboard_status.down_ladder<<6);
	packet[2]=	keyboard_status.weapon;
	send_packet(packet,3,(struct sockaddr*)(&server),my_id,0);
}


void reset_keyboard(void)
{
	keyboard_status.left=0;
	keyboard_status.right=0;
	keyboard_status.speed=0;
	keyboard_status.jump=0;
	keyboard_status.creep=0;
	keyboard_status.fire=0;
	keyboard_status.weapon=0;
	keyboard_status.down_ladder=0;
}

/* recompute object positions */
void update_game(void)
{
	struct object_list *p;
	int w,h;
	unsigned char stop_x,stop_y,sy;
	unsigned long_long t;
	my_double x,y,x1,y1,DELTA_TIME;

	for(p=&objects;p->next;p=p->next)
	{
		if (p->next->member.type==T_NOTHING)continue;
		if ((p->next->member.status)&1024)continue;   /* dead player */
		/* decrement time to live */
		if (p->next->member.ttl>0)
		{
			p->next->member.ttl--;
			if (!p->next->member.ttl)
			{
				if ((p->next->member.type)==T_PLAYER)p->next->member.status&=~16;
				else
				{
					if (p->next->member.type!=T_GRENADE){  /* client's waiting for P_EXPLODE_GRENADE and doesn't delete the grenade yet */
		                        p=p->prev; 
					delete_obj(p->next->next->member.id);
					continue;}
				}
			}
		}
		/* maintain only objects that you are allowed to maintain */
		if (!(obj_attr[p->next->member.type].maintainer&1))continue;
		
		
		/* if not falling slow down x motion */
		if (!(p->next->member.status&8))p->next->member.xspeed=mul(p->next->member.xspeed,obj_attr[p->next->member.type].slow_down_x);
		
		/* fall */
		if (obj_attr[p->next->member.type].fall)
		{
			p->next->member.status|=8;
			p->next->member.yspeed+=FALL_ACCEL;
			/* but not too fast */
			if (p->next->member.yspeed>MAX_Y_SPEED)p->next->member.yspeed=MAX_Y_SPEED;
		}
			
                get_dimensions(p->next->member.type,p->next->member.status,sprites[p->next->member.sprite].positions,&w,&h);
                x=p->next->member.x;
                y=p->next->member.y;
		t=get_time();
		DELTA_TIME=float2double(((double)(long_long)(t-p->next->member.last_updated))/MICROSECONDS);
                update_position(
                        &(p->next->member),
                        p->next->member.x+mul(p->next->member.xspeed,DELTA_TIME),
                        p->next->member.y+mul(p->next->member.yspeed,DELTA_TIME),
                        w,h,&stop_x,&stop_y);
		p->next->member.last_updated=t;
		
		/* walk up the stairs */
		if (stop_x&&p->next->member.type==T_PLAYER&&!(p->next->member.status&256))
		{
			x1=p->next->member.x;
			y1=p->next->member.y;
			p->next->member.x=x;
			p->next->member.y=y-int2double(1);
	                update_position(
        	                &(p->next->member),
                	        p->next->member.x+mul(p->next->member.xspeed,DELTA_TIME),
                        	p->next->member.y+mul(p->next->member.yspeed,DELTA_TIME),
	                        w,h,0,&sy);
			if ((p->next->member.xspeed>0&&p->next->member.x<=x1)||(p->next->member.xspeed<0&&p->next->member.x>=x1)) /* restore original values */
			{
				p->next->member.x=x1;
				p->next->member.y=y1;
			}
			else
			{
				stop_y=sy;
				stop_x=0;
			}
		}
		
		if (stop_x)p->next->member.xspeed=-mul(p->next->member.xspeed,obj_attr[p->next->member.type].bounce_x);
		if (my_abs(p->next->member.xspeed)<MIN_X_SPEED)
		{
			p->next->member.xspeed=0;
			p->next->member.status&=~1;
		}


		if (stop_y)
		{
			p->next->member.yspeed=mul(p->next->member.yspeed,obj_attr[p->next->member.type].bounce_y);
			p->next->member.yspeed=-p->next->member.yspeed;
			if (my_abs(p->next->member.yspeed)<MIN_Y_SPEED)
			{
				p->next->member.yspeed=0;
				if (stop_y==1)p->next->member.status&=~8;
			}
		}

		if ((p->next->member.type==T_SHRAPNEL||p->next->member.type==T_BULLET)&&(stop_x||stop_y))  /* bullet and shrapnel die crashing into wall */
		{
			p=p->prev;  /* deleting object makes a great mess in for cycle, so we must cheat the cycle */
			delete_obj(p->next->next->member.id);
			continue;
		}

	}
}

#define FIRE_SHOOTING weapon[current_weapon].cadence-4+HOLD_GUN_AFTER_SHOOT

/* hero's next anim position when walking to the right */
int _next_anim_right(int pos,int status, int ttl)
{
	int start,offset;
	
	if (pos<=18)start=10;  /* normal */
	else
	{
		if (pos<=46)start=38; /* holding gun */
		else 
		{
			if (pos<=55)start=47; /* shooting */
			else start=64;  /* creeping */
		}
	}
	offset=pos-start;
	
	if (status&256)start=64;  /* creeping */
	else
	{
		if (status&16)
		{
			if (ttl>=FIRE_SHOOTING) start=47;  /* shooting */
			else start=38; /* holding a gun */
		}
		else start=10; /* normal */
	}
	
	return (status&256)?((offset+1)&7)+start:start+(offset&7)+1;
}


/* hero's next anim position when walking to the left */
int _next_anim_left(int pos,int status, int ttl)
{
	int start,offset;
	
	if (pos<=8)start=0;  /* normal */
	else
	{
		if (pos<=28)start=20; /* holding gun */
		else
		{
			if (pos<=37)start=29; /* shooting */
			else start=56; /* creeping */
		}
	}
	offset=pos-start;
	
	if (status&256)start=56; /* creeping */
	else
	{
		if (status&16)
		{
			if (ttl>=FIRE_SHOOTING) start=29;  /* shooting */
			else start=20; /* holding a gun */
		}
		else start=0; /* normal */
	}
	
	return (status&256)?((offset+1)&7)+start:start+(offset&7)+1;
}


/* update hero animating position */
void update_anim(struct it* obj)
{
	if (!(obj->status&1))  /* not walking */
		switch((obj->status&6)>>1)
		{
			case 0:  /* look at me */
			obj->anim_pos=(obj->status&256)?72:9;
			break;
			case 1:   /* look left */
			if (obj->status&16)
			{
				if (obj->status&512)
				{
					if (obj->ttl>=((weapon[WEAPON_GRENADE].cadence>>1)+HOLD_GUN_AFTER_SHOOT))obj->anim_pos=73;
					else 
					{
						if (obj->ttl>=((weapon[WEAPON_GRENADE].cadence>>2)+HOLD_GUN_AFTER_SHOOT))obj->anim_pos=74;
						else
						{
							if (obj->ttl>=HOLD_GUN_AFTER_SHOOT)obj->anim_pos=75;
							else obj->anim_pos=0;
						}
					}
				}
				else
				{
					if (obj->ttl>=FIRE_SHOOTING)obj->anim_pos=29;
					else obj->anim_pos=20;
				}
			}
			else obj->anim_pos=0;
			if (obj->status&256)obj->anim_pos=56;
			break;
			case 2:   /* look right */
			if (obj->status&16)
			{
				if (obj->status&512)
				{
					if (obj->ttl>=((weapon[WEAPON_GRENADE].cadence>>1)+HOLD_GUN_AFTER_SHOOT))obj->anim_pos=76;
					else 
					{
						if (obj->ttl>=((weapon[WEAPON_GRENADE].cadence>>2)+HOLD_GUN_AFTER_SHOOT))obj->anim_pos=77;
						else
						{
							if (obj->ttl>=HOLD_GUN_AFTER_SHOOT)obj->anim_pos=78;
							else obj->anim_pos=10;
						}
					}
				}
				else
				{
					if (obj->ttl>=FIRE_SHOOTING)obj->anim_pos=47;
					else obj->anim_pos=38;
				}
			}
			else obj->anim_pos=10;
			if (obj->status&256)obj->anim_pos=64;
			break;
		}
	else  /* walking */
	{
		switch((obj->status&6)>>1)
		{
			case 1:  /* walk left */
			obj->anim_pos=_next_anim_left(obj->anim_pos,obj->status,obj->ttl);
			break;

			case 2:   /* walk right */
			obj->anim_pos=_next_anim_right(obj->anim_pos,obj->status,obj->ttl);
			break;
		}
	}
}


/* draw scene into screenbuffer*/
void draw_scene(void)
{
	struct object_list *p;
	unsigned char fore;
	
#ifdef TRI_D
	if (TRI_D_ON)
	{
		tri_d=1;
		show_window(double2int(hero->x)-SCREEN_XOFFSET,double2int(hero->y)-SCREEN_YOFFSET);
		tri_d=0;
	}
#endif
		
	show_window(double2int(hero->x)-SCREEN_XOFFSET,double2int(hero->y)-SCREEN_YOFFSET);
	for (fore=0;fore<=1;fore++)
	{
		for(p=&objects;p->next;p=p->next)
		{
			if ((obj_attr[p->next->member.type].foreground)!=fore)continue;
			if (&(p->next->member)==hero)continue;
			if (p->next->member.type==T_PLAYER)update_anim(&(p->next->member));
			else {p->next->member.anim_pos++;p->next->member.anim_pos%=sprites[p->next->member.sprite].n_steps;}
			if (!(p->next->member.status&1024)&&!(p->next->member.status&64))   /* don't draw hidden objects and dead players */
			{
#ifdef TRI_D
				if(TRI_D_ON)
				{
					tri_d=1;
					put_sprite(
						double2int(p->next->member.x-hero->x)+fore+SCREEN_XOFFSET,
						double2int(p->next->member.y-hero->y)+SCREEN_YOFFSET,
						sprites[p->next->member.sprite].positions+sprites[p->next->member.sprite].steps[p->next->member.anim_pos],
						1
						);
					tri_d=0;
				}
#endif
					
				put_sprite(
					double2int(p->next->member.x-hero->x)+SCREEN_XOFFSET,
					double2int(p->next->member.y-hero->y)+SCREEN_YOFFSET,
					sprites[p->next->member.sprite].positions+sprites[p->next->member.sprite].steps[p->next->member.anim_pos],
					1
					);
			}
			if (p->next->member.type==T_PLAYER&&p->next->member.status&128) /* hit */
			{
				p->next->member.status&=~128;
				if (!(p->next->member.status&1024))   /* don't draw blood splash to dead players */
				{
#ifdef TRI_D
					if (TRI_D_ON)
					{	
						tri_d=1;
						put_sprite(
							double2int(p->next->member.x-hero->x)+SCREEN_XOFFSET+fore+((int)(p->next->member.data)&255),
							double2int(p->next->member.y-hero->y)+SCREEN_YOFFSET+(((int)(p->next->member.data)>>8)&255),
							sprites[hit_sprite].positions+sprites[hit_sprite].steps[((int)p->next->member.data)>>16],
							1
							);
						tri_d=0;
					}
#endif
						
					put_sprite(
						double2int(p->next->member.x-hero->x)+SCREEN_XOFFSET+((int)(p->next->member.data)&255),
						double2int(p->next->member.y-hero->y)+SCREEN_YOFFSET+(((int)(p->next->member.data)>>8)&255),
						sprites[hit_sprite].positions+sprites[hit_sprite].steps[((int)p->next->member.data)>>16],
						1
						);
				}
			}
		}
		if (obj_attr[T_PLAYER].foreground!=fore)continue;
		update_anim(hero);
		if ((hero->status)&1024)continue;   /* don't draw dead hero */
#ifdef TRI_D
		if (TRI_D_ON)
		{
			tri_d=1;
			put_sprite(
				SCREEN_XOFFSET,
				SCREEN_YOFFSET,
				sprites[hero->sprite].positions+sprites[hero->sprite].steps[hero->anim_pos],
				1
				);
			tri_d=0;
		}
#endif
		put_sprite(
			SCREEN_XOFFSET,
			SCREEN_YOFFSET,
			sprites[hero->sprite].positions+sprites[hero->sprite].steps[hero->anim_pos],
			1
			);
		if (hero->status&128) /* hit */
		{
			hero->status&=~128;
#ifdef TRI_D
			if (TRI_D_ON)
			{
				tri_d=1;
				put_sprite(
					SCREEN_XOFFSET+((int)(hero->data)&255),
					SCREEN_YOFFSET+(((int)(hero->data)>>8)&255),
					sprites[hit_sprite].positions+sprites[hit_sprite].steps[((int)hero->data)>>16],
					1
					);
				tri_d=0;
			}
#endif
			put_sprite(
				SCREEN_XOFFSET+((int)(hero->data)&255),
				SCREEN_YOFFSET+(((int)(hero->data)>>8)&255),
				sprites[hit_sprite].positions+sprites[hit_sprite].steps[((int)hero->data)>>16],
				1
				);
		}
	}
}	


void change_level(void)
{
	unsigned char *LEVEL;
	unsigned char txt[256];

	clean_memory();
	free_sprites(level_sprites_start);
	reinit_area();

	LEVEL=load_level(level_number);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,LEVEL_SPRITES_SUFFIX);
	load_sprites(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,STATIC_DATA_SUFFIX);
	load_data(txt);
	mem_free(LEVEL);
}


/* returns number of read bytes */
int process_packet(unsigned char *packet,int l)
{
	int a,n=l;

	switch(*packet)
	{
		case P_CHUNK:
		for (a=1;a<l&&a<MAX_PACKET_LENGTH;a+=n)
			n=process_packet(packet+a,l-a);
		break;

		case P_NEW_OBJ:
		if (l<28)break;  /* invalid packet */
		n=28;
		new_obj(
			get_int(packet+1), /* ID */
			packet[25],  /* type */
			get_int16(packet+26), /* time to live */
			get_int16(packet+5),  /* sprite */
			0, /* anim position */
			get_int16(packet+23),  /* status */
			get_float(packet+7),  /* x */
			get_float(packet+11),  /* y */
			get_float(packet+15),  /* xspeed */
			get_float(packet+19),  /* yspeed */
			0  /* data */
		);
		break;

		case P_PLAYER_DELETED:
		n=1;
		*error_message=0;
		return -1;
		
		case P_DELETE_OBJECT:
		if (l<5)break;  /* invalid packet */
		delete_obj(get_int(packet+1));
		n=5;
		break;

		case P_UPDATE_OBJECT:
		if (l<26)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=26;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_float(packet+6);
			p->member.y=get_float(packet+10);
			p->member.xspeed=get_float(packet+14);
			p->member.yspeed=get_float(packet+18);
			p->member.status=get_int16(packet+22);
			p->member.data=0;
			p->member.ttl=get_int16(packet+24);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_POS:
		if (l<22)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=22;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_float(packet+6);
			p->member.y=get_float(packet+10);
			p->member.xspeed=get_float(packet+14);
			p->member.yspeed=get_float(packet+18);
		}
		break;

		case P_UPDATE_OBJECT_SPEED:
		if (l<14)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=14;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.xspeed=get_float(packet+6);
			p->member.yspeed=get_float(packet+10);
		}
		break;

		case P_UPDATE_OBJECT_COORDS:
		if (l<14)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=14;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_float(packet+6);
			p->member.y=get_float(packet+10);
		}
		break;

		case P_UPDATE_OBJECT_SPEED_STATUS:
		if (l<16)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=16;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.xspeed=get_float(packet+6);
			p->member.yspeed=get_float(packet+10);
			p->member.status=get_int16(packet+14);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_COORDS_STATUS:
		if (l<16)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=16;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_float(packet+6);
			p->member.y=get_float(packet+10);
			p->member.status=get_int16(packet+14);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_SPEED_STATUS_TTL:
		if (l<18)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=18;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.xspeed=get_float(packet+6);
			p->member.yspeed=get_float(packet+10);
			p->member.status=get_int16(packet+14);
			p->member.ttl=get_int16(packet+16);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_COORDS_STATUS_TTL:
		if (l<18)break;   /* invalid packet */
		{
			struct object_list *p;
			
			n=18;
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_float(packet+6);
			p->member.y=get_float(packet+10);
			p->member.status=get_int16(packet+14);
			p->member.ttl=get_int16(packet+16);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_STATUS:
		if (l<7)break;   /* invalid packet */
		{
			struct object_list *p;

			n=7;
			p=find_in_table(get_int(packet+1));
			if (!p)break;  /* ignore objects we don't have */
			p->member.status=get_int16(packet+5);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_HIT:
		if (l<8)break;    /* invalid packet */
		{
			struct object_list *p;
	
			n=8;
			p=find_in_table(get_int(packet+1));
			if (!p)break;  /* ignore objects we don't have */
			p->member.status|=128;
			p->member.data=(void*)((packet[5]<<16)+(packet[7]<<8)+(packet[6]));
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status&32)&&(p->member.status&16))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_PLAYER:
		if (l<23)break;  /* invalid packet */
		health=packet[1];
		armor=packet[2];
		for (a=0;a<ARMS;a++)
			ammo[a]=get_int16(packet+3+(a<<1));
		frags=get_int(packet+3+ARMS*2);
		deaths=get_int(packet+7+ARMS*2);
		current_weapon=packet[11+2*ARMS];
		weapons=packet[12+2*ARMS];
		n=23;
		break;

		case P_MESSAGE:
		if (l<2)break;   /* invalid packet */
		add_message(packet+1);
		n=2+strlen(packet+1);
		break;

		case P_CHANGE_LEVEL:
		{
			unsigned char txt[256];
			unsigned char *name;
			unsigned char *md5;
			int a;
			char p;

			if (l<38)break;   /* invalid packet */
			a=get_int(packet+1);
			if (level_number==a)goto level_changed;
			level_number=a;
			snprintf(txt,256, "Trying to change level to number %d",level_number);
			add_message(txt);
			name=load_level(level_number);
			if (!name){snprintf(error_message,1024,"Cannot find level number %d. Game terminated. Press ENTER.", level_number);send_quit();return -1;}
			snprintf(txt,256,"Changing level to \"%s\"",name);
			mem_free(name);
			add_message(txt);
			
			md5=md5_level(level_number);
			if (strcmp(md5,packet+5))   /* MD5s differ */
			{
				mem_free(md5);
				snprintf(error_message,1024,"Invalid MD5 sum. Can't change level. Game terminated. Press ENTER.");
				add_message("Invalid MD5 sum. Can't change level. Exiting...");
				send_quit();
				return -1;
			}
			mem_free(md5);

			/* OK we can change it */
			change_level();
level_changed:		
			
			p=P_LEVEL_ACCEPTED;
			send_packet(&p,1,(struct sockaddr *)(&server),my_id,0);
			n=38;
		}
		break;

		case P_END:
		if (l<2)snprintf(error_message,1024,"Game terminated. Press ENTER.");
		else snprintf(error_message,1024,"Game terminated by %s. Press ENTER.",packet+1);
		return -1;

		case P_BELL:
		n=1;
		c_bell();
		break;

		case P_INFO:
		if (l<=5)break;
		active_players=get_int(packet+1);
		l=6;
		for (a=0;a<packet[5]&&a<TOP_PLAYERS_N;a++)
		{
			int x;
			top_players[a].frags=get_int(packet+l);
			top_players[a].deaths=get_int(packet+l+4);
			top_players[a].color=packet[l+8];
			x=strlen(packet+l+9)+1;
			memcpy(top_players[a].name,packet+l+9,x);
			l+=x+9;
		}
		n=l;
		break;

		case P_EXPLODE_GRENADE:
		{
			unsigned int i,j;
			struct object_list *p;
			int b;
			
			if (l<9)break;
			n=9;
			i=get_int(packet+1);
			j=get_int(packet+5);
			p=find_in_table(j);
			if (!p)break;
			
			for (b=0;b<N_SHRAPNELS_EXPLODE;b++)
			{
				double angle=(double)b*2*M_PI/N_SHRAPNELS_EXPLODE;
				my_double spd=add_int(mul_int(my_and(mul_int(weapon[WEAPON_GRENADE].speed,b+1),15),16),100);
						
				new_obj(
					i,
					T_SHRAPNEL,
					SHRAPNEL_TTL,
					shrapnel_sprite[random()%N_SHRAPNELS],
					0,
					WEAPON_GRENADE,
					p->member.x,
					p->member.y,
					p->member.xspeed+mul(spd,float2double(cos(angle))),
					p->member.yspeed+mul(spd,float2double(sin(angle))),
					0); 
				i++;
			}
			delete_obj(j);

		}
		break;
	}
	return n;
}


/* read packet from socket */
int read_data(void)
{
        fd_set rfds;
        struct timeval tv;
        struct sockaddr_in client;
        static unsigned char packet[MAX_PACKET_LENGTH];
        int a=sizeof(client);
	int l;

        tv.tv_sec=0;
        tv.tv_usec=0;
        FD_ZERO(&rfds);
        FD_SET(fd,&rfds);
	while(select(fd+1,&rfds,0,0,&tv))
	{
	        if ((l=recv_packet(packet,MAX_PACKET_LENGTH,(struct sockaddr*)(&client),&a,1,my_id,0))<0)
			return 0;   /* something's strange */
		if (process_packet(packet,l)<0)return 1;

	}
	return 0;
}


/* select color for numeric value (red=lack, yellow=edium, green=fill) */
int select_color(int value,int max)
{
	return value>=(max>>1)?10:(value>=(max>>2)?11:9);
}


/* draw board at the bottom of the screen */
void draw_board(void)
{
	int offs=SCREEN_X*(SCREEN_Y-2);
	int space=(SCREEN_X-60)/5;
	char txt[16];

	memset(screen_a+offs,4,SCREEN_X);
	memset(screen+offs,'-',SCREEN_X);
	memset(screen+offs+SCREEN_X,0,SCREEN_X);
	print2screen(0,SCREEN_Y-1,7,"HEALTH");
	sprintf(txt,"% 3d%%",health);
	print2screen(6,SCREEN_Y-1,select_color(health,100),txt);

	print2screen(11+(space>>1),SCREEN_Y-2,4,",");
	print2screen(11+(space>>1),SCREEN_Y-1,4,"|");
	print2screen(11+space,SCREEN_Y-1,7,"FRAGS");
	sprintf(txt,"% 4d",frags);
	print2screen(11+space+6,SCREEN_Y-1,11,txt);

	print2screen(21+space+(space>>1),SCREEN_Y-2,4,",");
	print2screen(21+space+(space>>1),SCREEN_Y-1,4,"|");
	print2screen(21+(space<<1),SCREEN_Y-1,7,"DEATHS");
	sprintf(txt,"% 4d",deaths);
	print2screen(21+(space<<1)+7,SCREEN_Y-1,11,txt);

	print2screen(31+(space<<1)+(space>>1),SCREEN_Y-2,4,",");
	print2screen(31+(space<<1)+(space>>1),SCREEN_Y-1,4,"|");
	sprintf(txt,"%10s",weapon[current_weapon].name);
	print2screen(31+3*space,SCREEN_Y-1,11,txt);
	
	print2screen(41+(3*space)+(space>>1),SCREEN_Y-2,4,",");
	print2screen(41+(3*space)+(space>>1),SCREEN_Y-1,4,"|");
	print2screen(41+(space<<2),SCREEN_Y-1,7,"AMMO");
	sprintf(txt,"% 4d",ammo[current_weapon]);
	print2screen(41+5+(space<<2),SCREEN_Y-1,select_color(ammo[current_weapon],weapon[current_weapon].max_ammo),txt);

	print2screen(49+(space<<2)+(space>>1),SCREEN_Y-2,4,",");
	print2screen(49+(space<<2)+(space>>1),SCREEN_Y-1,4,"|");
	print2screen(49+5*space,SCREEN_Y-1,7,"ARMOR");
	sprintf(txt,"% 4d%%",armor);
	print2screen(49+5+5*space,SCREEN_Y-1,select_color(armor,100),txt);
	if ((hero->status)&64)print2screen(SCREEN_X-14,SCREEN_Y-2,11,"INVISIBILITY");
	if (autorun)print2screen(2,SCREEN_Y-2,15,"AUTORUN");
	if (autocreep)print2screen(10,SCREEN_Y-2,15,"AUTOCREEP");
}


/* read string from input, returns everytime, not when string's read */
int read_str_online(int y,char *pointer,char *message, int max_len)
{
	int a,b,c,shift=0;

	a=strlen(message);
	b=strlen(pointer);
	c=SCREEN_X*y;

#ifdef TRI_D
	if (TRI_D_ON)
	{
		memcpy(screen2+c,message,a);
		memcpy(screen2+c+a,pointer,b);
		memset(screen2_a+c,15,a);
		memset(screen2_a+c+a,7,b);
		memset(screen2_a+c+a+b,0,SCREEN_X-b-a);
	}
#endif
	
	memcpy(screen+c,message,a);
	memcpy(screen+c+a,pointer,b);
	memset(screen_a+c,15,a);
	memset(screen_a+c+a,7,b);
	memset(screen_a+c+a+b,0,SCREEN_X-b-a);

	c_update_kbd();
	
	if (c_was_pressed(K_ESCAPE))
	{
		pointer[0]=0;
		return 2;
	}
	if (c_was_pressed(K_ENTER))
	{
		return 1;
	}
	if (c_pressed(K_BACKSPACE)&&b)
	{
		b--;
		pointer[b]=0;
	}
	shift=c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT);
	for (c=' ';c<128&&b<max_len;c++)
		if (c_was_pressed(c))
		{
			pointer[b]=shift?my_toupper(c):c;
			b++;
			pointer[b]=0;
		}
	return 0;
}


/* read number from input, returns everytime, not when number's read */
int read_num_online(int y,char *pointer,char *message, int max_len)
{
	int a,b,c;

	a=strlen(message);
	b=strlen(pointer);
	c=SCREEN_X*y;

#ifdef TRI_D
	if (TRI_D_ON)
	{
		memcpy(screen2+c,message,a);
		memcpy(screen2+c+a,pointer,b);
		memset(screen2_a+c,15,a);
		memset(screen2_a+c+a,7,b);
		memset(screen2_a+c+a+b,0,SCREEN_X-b-a);
	}
#endif

	memcpy(screen+c,message,a);
	memcpy(screen+c+a,pointer,b);
	memset(screen_a+c,15,a);
	memset(screen_a+c+a,7,b);
	memset(screen_a+c+a+b,0,SCREEN_X-b-a);

	c_update_kbd();

	if (c_was_pressed(K_ESCAPE))
	{
		pointer[0]=0;
		return 2;
	}
	if (c_was_pressed(K_ENTER))
	{
		return 1;
	}
	if (c_pressed(K_BACKSPACE)&&b)
	{
		b--;
		pointer[b]=0;
	}
	for (c='0';c<='9'&&b<max_len;c++)
		if (c_was_pressed(c))
		{
			pointer[b]=c;
			b++;
			pointer[b]=0;
		}
	return 0;
}


/* load banner.dat */
void load_banner(char **banner)
{
	FILE *s;
	char line[1025];
	int a,b;
	
	s=fopen(DATA_PATH BANNER_FILE,"r");
	if (!s){shut_down(0);ERROR("Error: Can't load file \""DATA_PATH BANNER_FILE"\".\n");EXIT(1);}
	*banner=mem_alloc(1);
	**banner=0;
	if (!(*banner)){shut_down(0);ERROR("Error: Not enough memory.\n");EXIT(1);}
	while (fgets(line,1024,s))
	{
		if (line[strlen(line)-1]==10)line[strlen(line)-1]=0;
		if (line[strlen(line)-1]==13)line[strlen(line)-1]=0;  /* crlf shit */
		a=strlen(*banner);
		b=strlen(line);
		*banner=mem_realloc((*banner),a+b+SCREEN_X+1);
		memcpy((*banner)+a,line,b);
		memset((*banner)+a+b,' ',SCREEN_X);
		(*banner)[a+b+SCREEN_X]=0;
	}
	fclose(s);
}


/* draw initial screen */
void menu_screen(char *host,char *name,unsigned short *p,int *color)
{
	char txt[8];
	int sprite,anim=0,title_anim=0,bulge_anim=0;
	unsigned long_long t;
	char port[MAX_PORT_LEN+1];
	int a=0;
	char *m;
	char *color_name[15]={"red","green","brown","blue","violet","cyan","gray","black","light red","light green","yellow","light blue","magenta","light cyan","white"};
	char *banner;
	int l,banner_pos=0;
	int help=0;
	
	load_banner(&banner);
	l=strlen(banner);
	sprintf(port,"%d",*p);
	sprintf(txt,"hero%d",*color);
	if (find_sprite(txt,&sprite))
	{
		unsigned char msg[256];
		mem_free(banner);
		shut_down(0);
		snprintf(msg,256,"Error: Can't find sprite \"%s\".\n",txt);
		ERROR(msg);
		EXIT(1);
	}
	
cycle:
	t=get_time();
	clear_screen();
#ifdef TRI_D
	if (TRI_D_ON)
	{
		tri_d=1;
		put_sprite((SCREEN_X+1-TITLE_WIDTH)>>1,0,sprites[title_sprite].positions+sprites[title_sprite].steps[title_anim],0);
		put_sprite(SCREEN_XOFFSET+1+PLAYER_WIDTH+((SCREEN_X-SCREEN_XOFFSET-PLAYER_WIDTH-BULGE_WIDTH)>>1),TITLE_HEIGHT+((SCREEN_Y-3-BULGE_HEIGHT-TITLE_HEIGHT)>>1),sprites[bulge_sprite].positions+sprites[bulge_sprite].steps[bulge_anim],0);
		put_sprite(SCREEN_XOFFSET+1,TITLE_HEIGHT+((SCREEN_Y-3-PLAYER_HEIGHT-TITLE_HEIGHT)>>1),sprites[sprite].positions+sprites[sprite].steps[(anim<2?48:39)+(anim&7)],0);
		tri_d=0;
	}
#endif
	put_sprite((SCREEN_X-TITLE_WIDTH)>>1,0,sprites[title_sprite].positions+sprites[title_sprite].steps[title_anim],0);
	put_sprite(SCREEN_XOFFSET+PLAYER_WIDTH+((SCREEN_X-SCREEN_XOFFSET-PLAYER_WIDTH-BULGE_WIDTH)>>1),TITLE_HEIGHT+((SCREEN_Y-3-BULGE_HEIGHT-TITLE_HEIGHT)>>1),sprites[bulge_sprite].positions+sprites[bulge_sprite].steps[bulge_anim],0);
	put_sprite(SCREEN_XOFFSET,TITLE_HEIGHT+((SCREEN_Y-3-PLAYER_HEIGHT-TITLE_HEIGHT)>>1),sprites[sprite].positions+sprites[sprite].steps[(anim<2?48:39)+(anim&7)],0);
	print2screen(0,TITLE_HEIGHT+4,11,"N:");
	print2screen(1,TITLE_HEIGHT+4,2,"AME:");
	print2screen(6,TITLE_HEIGHT+4,7,name);
	print2screen(0,TITLE_HEIGHT+6,2,"COLOR:");
	print2screen(7,TITLE_HEIGHT+6,7,color_name[(*color-1)%15]);
	print2screen(0,TITLE_HEIGHT+0,11,"S");
	print2screen(1,TITLE_HEIGHT+0,2,"ERVER ADDRESS:");
	print2screen(16,TITLE_HEIGHT+0,7,host);
	print2screen(0,TITLE_HEIGHT+2,11,"P");
	print2screen(1,TITLE_HEIGHT+2,2,"ORT:");
	print2screen(6,TITLE_HEIGHT+2,7,port);
	print2screen((SCREEN_X-26)>>1,SCREEN_Y-3,7,"Use arrows to change color");
	print2screen((SCREEN_X-52)>>1,SCREEN_Y-2,7,"Press ENTER to connect, H for help, ESC or Q to quit");
	print2screen(0,SCREEN_Y-1,15,banner+(banner_pos>>1));
	banner_pos++;
	banner_pos%=l<<1;
	anim++;
	anim&=15;
	title_anim++;
	title_anim%=sprites[title_sprite].n_steps;
	bulge_anim++;
	bulge_anim%=sprites[bulge_sprite].n_steps;
	if (*error_message)
	{
		print2screen((SCREEN_X-strlen(error_message)>>1),SCREEN_Y-1,9,error_message);

#ifdef TRI_D
		if (TRI_D_ON)
		{
			tri_d=1;
			blit_screen(1);
			tri_d=0;
		}

#endif
		blit_screen(1);
		wait_for_enter();
		*error_message=0;
		goto cc1;
	}
	if (help)print_help_window();
	switch(a)
	{
		case 1:
		if (read_str_online(SCREEN_Y-1,name,"Enter your name: ",MAX_NAME_LEN))
			a=0;
		break;

		case 3:
		if (read_str_online(SCREEN_Y-1,host,"Enter address: ",MAX_HOST_LEN))
			a=0;
		break;

		case 2:
		if (read_num_online(SCREEN_Y-1,port,"Enter port: ",MAX_PORT_LEN))
			a=0;
		break;
	}
	c_update_kbd();

#ifdef TRI_D
	if (TRI_D_ON)
	{
		tri_d=1;
		blit_screen(1);
		tri_d=0;
	}
#endif

	blit_screen(1);
	if (!a)
	{
		if (c_was_pressed('+')||c_was_pressed('=')||c_was_pressed(K_UP)||c_was_pressed(K_RIGHT)||c_was_pressed(K_NUM_PLUS))
		{
			(*color)++;
			if (*color>30)(*color)=1;
			sprintf(txt,"hero%d",*color);
			if (find_sprite(txt,&sprite))
				{shut_down(0);mem_free(banner);fprintf(stderr,"Error: Can't find sprite \"%s\".\n",txt);}
		}
		
		if (c_was_pressed('-')||c_was_pressed(K_NUM_MINUS)||c_was_pressed(K_DOWN)||c_was_pressed(K_LEFT))
		{
			(*color)--;
			if (*color<1)(*color)=30;
			sprintf(txt,"hero%d",*color);
			if (find_sprite(txt,&sprite))
				{shut_down(0);mem_free(banner);fprintf(stderr,"Error: Can't find sprite \"%s\".\n",txt);}
		}
		
		if (c_was_pressed('h'))help^=1;
		if (c_was_pressed(K_ENTER))
		{
			save_cfg(host,name,*color);
			*p=strtol(port,0,10);
			if ((m=find_server(host,*p)))
			{
				print2screen((SCREEN_X-strlen(m)>>1),SCREEN_Y-1,9,m);
#ifdef TRI_D
				if (TRI_D_ON)
				{
					tri_d=1;
					blit_screen(1);
					tri_d=0;
				}
#endif
				blit_screen(1);
				wait_for_enter();
				goto cc1;
			}
			if ((m=init_socket()))
			{
				print2screen((SCREEN_X-strlen(m)>>1),SCREEN_Y-1,9,m);
#ifdef TRI_D
				if (TRI_D_ON)
				{
					tri_d=1;
					blit_screen(1);
					tri_d=0;
				}
#endif
				blit_screen(1);
				wait_for_enter();
				goto cc1;
			}
			if ((m=contact_server(*color,name)))
			{
				print2screen((SCREEN_X-strlen(m)>>1),SCREEN_Y-1,9,m);
#ifdef TRI_D
				if (TRI_D_ON)
				{
					tri_d=1;
					blit_screen(1);
					tri_d=0;
				}
#endif
				blit_screen(1);
				wait_for_enter();
				goto cc1;
			}
			mem_free(banner);
			return;
		}
cc1:
		if (c_was_pressed('q')||c_was_pressed(K_ESCAPE))
		{
			save_cfg(host,name,*color);
			mem_free(banner);
			shut_down(1);
		}
		
		if (c_was_pressed('n'))
			a=1;
		
		if (c_was_pressed('a')||c_was_pressed('s'))
			a=3;
		
		if (c_was_pressed('p'))
			a=2;
	}
	sleep_until(t+MENU_PERIOD_USEC);
	goto cycle;
}


/* handle fatal signal (sigabrt, sigsegv, ...) */
void signal_handler(int signum)
{
	
	if (connected)send_quit();
	shut_down(0);
#ifdef HAVE_PSIGNAL
	psignal(signum,"Exiting on signal");
#else
	fprintf(stderr, "Exiting on signal: %d\n", signum);
#endif
	EXIT(1);
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


/* print command line help */
void print_help(void)
{
        printf( 
		"0verkill client"
#ifdef XWINDOW
		" for X windows"
#endif
		".\n"
                "(c)2000 Brainsoft\n"
                "Usage: 0verkill [-h] [-3] [-s <width>x<height>]"
#ifdef XWINDOW
		" [-d <display>] [-f <font name>]"
#endif
		"\n"
	);
}


void parse_dimensions(char *txt)
{
	char *p;
	char *e;
	p=txt;
	for (p=txt;(*p)&&(*p)!='x';p++);
	if (!(*p)){ERROR("Error: Expected dimensions in form WIDTHxHEIGHT.\n");EXIT(1);}
	(*p)=0;
	p++;
	if (!strlen(txt)||!strlen(p)){fprintf(stderr,"Error: Decimal number expected.\n");EXIT(1);}
	SCREEN_X=strtoul(txt,&e,10);
	if (*e){ERROR("Error: Decimal number expected.\n");EXIT(1);}
	SCREEN_Y=strtoul(p,&e,10);
	if (*e){ERROR("Error: Decimal number expected.\n");EXIT(1);}
	set_size=1;
}


void parse_command_line(int argc,char **argv)
{
        int a;

        while(1)
        {
#ifdef TRI_D

	#ifdef XWINDOW
		a=getopt(argc,argv,"3hl:f:d:s:");
	#endif
	#ifndef XWINDOW
		a=getopt(argc,argv,"3hl:s:");
	#endif
#else
	#ifdef XWINDOW
		a=getopt(argc,argv,"hf:d:s:");
	#endif
	#ifndef XWINDOW
		a=getopt(argc,argv,"hs:");
	#endif
#endif
                switch(a)
                {
                        case EOF:
                        return;

                        case '?':
                        case ':':
                        EXIT(1);

                        case 'h':
                        print_help();
                        EXIT(0);

#ifdef TRI_D
			case '3':
			TRI_D_ON=1;
			break;
#endif

                        case 's':
			parse_dimensions(optarg);
                        break;
#ifdef XWINDOW
			case 'd':
			x_display_name=optarg;
			break;

			case 'f':
			x_font_name=optarg;
			break;
#endif
                }
        }
}


#define HALL_FAME_WIDTH (MAX_NAME_LEN+17)
#define HALL_FAME_HEIGHT ((active_players>TOP_PLAYERS_N?TOP_PLAYERS_N:active_players)+5)
#define HALL_FAME_X ((SCREEN_X-2-HALL_FAME_WIDTH)>>1)
#define HALL_FAME_Y ((SCREEN_Y-2-HALL_FAME_HEIGHT)>>1)

/* draw window with top players */
void print_hall_of_fame(void)
{
	int a;
	char txt[32];
	unsigned char color;

	if (!active_players)return;
	sprintf(txt,"TOP %d PLAYERS",TOP_PLAYERS_N);
	draw_frame(HALL_FAME_X,HALL_FAME_Y,HALL_FAME_WIDTH,HALL_FAME_HEIGHT,15);
	print2screen(HALL_FAME_X+1+((HALL_FAME_WIDTH-strlen(txt))>>1),HALL_FAME_Y+1,11,txt);
	print2screen(HALL_FAME_X+2,HALL_FAME_Y+3,15,"NAME");
	print2screen(HALL_FAME_X+2+MAX_NAME_LEN+2,HALL_FAME_Y+3,15,"FRAGS");
	print2screen(HALL_FAME_X+2+MAX_NAME_LEN+9,HALL_FAME_Y+3,15,"DEATHS");
	for (a=0;a<active_players&&a<TOP_PLAYERS_N;a++)
	{
		color=((top_players[a].color-1)%15)+1;
		print2screen(HALL_FAME_X+2,HALL_FAME_Y+4+a,color,top_players[a].name);
		sprintf(txt,"% 5d",top_players[a].frags);
		print2screen(HALL_FAME_X+2+MAX_NAME_LEN+2,HALL_FAME_Y+4+a,color,txt);
		sprintf(txt,"% 5d",top_players[a].deaths);
		print2screen(HALL_FAME_X+2+MAX_NAME_LEN+9,HALL_FAME_Y+4+a,color,txt);
	}
	sprintf(txt,"%d",active_players);
	print2screen(HALL_FAME_X+2,HALL_FAME_Y+HALL_FAME_HEIGHT,11,"Players in the game:");
	print2screen(HALL_FAME_X+23,HALL_FAME_Y+HALL_FAME_HEIGHT,7,txt);
}


void play(void)
{
#ifdef PAUSE
	int p=0;
#endif
	char string[80];
	unsigned char chat=0;
	unsigned char help=0;
	unsigned char hall_fame=0;
	unsigned long_long last_time;

	last_time=get_time();
	while(1)
	{
		last_time+=CLIENT_PERIOD_USEC;
		if (get_time()-last_time>PERIOD_USEC*100)last_time=get_time();
		if (read_data())return;  /* game terminated */
#ifdef PAUSE
		if(!p)clear_screen();
#else
		clear_screen();
#endif
		update_game();
#ifdef PAUSE
		if (!p)draw_scene();
#else
		draw_scene();
#endif
		update_messages(last_time);
		print_messages();
		if (chat)
		{
			switch (read_str_online(SCREEN_Y-3,string,"> ",78))
			{
				case 2:
				chat=0;
				break;
			
				case 1:
				chat=0;
				send_message(string);
				break;
			}
		}
		if (help)print_help_window();
		if (hall_fame)print_hall_of_fame();
		draw_board();

#ifdef TRI_D
		if (TRI_D_ON)
		{
			tri_d=1;
			blit_screen(1);
			tri_d=0;
		}
#endif

		blit_screen(1);

		creep=0;
		if (!chat)c_update_kbd();
     
#ifdef PAUSE
		if (c_was_pressed('p'))p^=1;
#endif
		if (!chat&&c_was_pressed('r'))
		{
			c_cls();
			redraw_screen();
		}
		if (!chat&&c_was_pressed('h'))help^=1;
		if (!chat&&c_was_pressed(K_TAB))
		{
			hall_fame^=1;
			if (hall_fame)send_info_request();
		}
		if (c_was_pressed(K_CAPS_LOCK)||(!chat&&c_was_pressed('a')))autorun^=1;
		if (c_was_pressed(K_F10)||(!chat&&c_was_pressed('q')))
		{
			send_quit();
			*error_message=0;
			return;
		}
		if (c_pressed(K_F12))
			end_game();

		if (!chat && c_was_pressed(K_ENTER))
		{
			memset(string,0,80);
#ifdef WIN32
			c_update_kbd();
			c_was_pressed('d');
#endif
			chat=1;
		}
		reset_keyboard();
		if (!chat&&c_was_pressed(' '))send_reenter_game();
		if (!chat&&c_was_pressed('c'))
			autocreep^=1;
		if (c_pressed(K_DOWN)||autocreep)
			keyboard_status.creep=1;
		if (c_pressed(K_UP))
			keyboard_status.jump=1;
		if (c_pressed(K_LEFT_CTRL)||c_pressed(K_RIGHT_CTRL)||(!chat&&c_pressed('z')))
			keyboard_status.fire=1;
		if (!chat && c_pressed('d'))
				keyboard_status.down_ladder=1;
		if (!chat && c_was_pressed('1'))
			keyboard_status.weapon=1;
		if (!chat && c_was_pressed('2'))
			keyboard_status.weapon=2;
		if (!chat && c_was_pressed('3'))
			keyboard_status.weapon=3;
		if (!chat && c_was_pressed('4'))
			keyboard_status.weapon=4;
		if (!chat && c_was_pressed('5'))
			keyboard_status.weapon=5;
		if (c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT)||autorun)
		keyboard_status.speed=1;
		if (c_pressed(K_LEFT))
			keyboard_status.left=1;
		if (c_pressed(K_RIGHT))
			keyboard_status.right=1;

		send_keyboard();
		sleep_until(last_time+CLIENT_PERIOD_USEC);
	}
}

/*----------------------------------------------------------------------------*/
int main(int argc,char **argv)
{
	unsigned short port=DEFAULT_PORT;
	int color=1;
	char host[MAX_HOST_LEN+1];
	char name[MAX_NAME_LEN+1];
	int a;
	unsigned char txt[256];
	
	
#ifdef WIN32
	WSADATA wd;

	WSAStartup(0x101, &wd);
	printf("Started WinSock version %X.%02X\n", wd.wVersion/0x100, wd.wVersion&0xFF);
#endif


#ifdef XWINDOW
	x_display_name=0;
	x_font_name=0;
#endif

	last_message=-1;
	autorun=0;
	autocreep=0;
	memset(host,0,MAX_HOST_LEN+1);
	memset(name,0,MAX_NAME_LEN+1);
	load_cfg(host,name,&color);
	last_obj=&objects;
	*error_message=0;

	parse_command_line(argc,argv);
	hash_table_init();
	if (!set_size)c_get_size(&SCREEN_X,&SCREEN_Y);
	init_sprites();
	init_area();
	load_sprites(DATA_PATH GAME_SPRITES_FILE);
	/* sprites are stored in memory in this order: game sprites (players,
	 * bullets, corpses, blood, meat...) followed by level sprites 
	 */

	level_sprites_start=n_sprites;
	if (find_sprite("hit",&hit_sprite)){ERROR("Error: Can't find sprite \"hit\".\n");EXIT(1);}
	if (find_sprite("title",&title_sprite)){ERROR("Error: Can't find sprite \"title\".\n");EXIT(1);}
	if (find_sprite("bulge",&bulge_sprite)){ERROR("Error: Can't find sprite \"bulge\".\n");EXIT(1);}
	for (a=0;a<N_SHRAPNELS;a++)
	{
		sprintf(txt,"shrapnel%d",a+1);
		if (find_sprite(txt,&shrapnel_sprite[a])){fprintf(stderr,"Can't find sprite \"%s\".\n",txt);EXIT(1);}
	}

	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGILL,signal_handler);
	signal(SIGABRT,signal_handler);
	signal(SIGFPE,signal_handler);
	signal(SIGSEGV,signal_handler);
#ifndef WIN32
	signal(SIGQUIT,signal_handler);
	signal(SIGBUS,signal_handler);
#ifdef SIGWINCH
	signal(SIGWINCH,sigwinch_handler);
#endif
#endif
	c_init(SCREEN_X,SCREEN_Y);
	c_cls();
	c_cursor(C_HIDE);
	c_refresh();
	while(1)
	{
		level_number=-1;
		connected=0;
		menu_screen(host,name,&port,&color);
		connected=1;
		play();
		clean_memory();
		delete_obj(hero->id);
	}
}
