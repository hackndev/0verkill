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

#include "sprite.h"
#include "data.h"
#include "cfg.h"
#include "net.h"
#include "hash.h"
#include "time.h"
#include "math.h"
#include "getopt.h"
#include "error.h"


#ifdef WIN32
	int consoleApp=1;
#endif

#define N_NAMES 18
#define CAN_SEE_X 60
#define CAN_SEE_Y 10

/* some empty functions and variables */
void c_shutdown(void){}
void init_blit(void){}
void shutdown_blit(void){}
int SCREEN_X=1,SCREEN_Y=1;
unsigned char *screen,*screen_a;
#ifdef TRI_D
unsigned char *screen2,*screen2_a;
#endif


static int in_signal_handler=0;
static int level_sprites_start;
static int level_number=-1;

/* my health, armor, frags, deaths, ammo, ... and ID */
static unsigned char health,armor;
static unsigned int frags,deaths;
static unsigned short ammo[ARMS];
static unsigned char current_weapon;
static unsigned char weapons;
static int my_id;
/* for use with the -n option */
static char *static_name=NULL;

/* connection with server */
static int connected=0;

int console_ok=1;

/* networking */
int fd;  /* socket */
static struct sockaddr_in server;  /* server address */

/* objects */
static struct object_list objects;
struct object_list *last_obj;
static struct it* hero;

static unsigned long_long game_start_offset; /* time difference between game start on this machine and on server */


static struct  /* keyboard status */
{
	unsigned char right,left,jump,creep,speed,fire,weapon,down_ladder;
}keyboard_status;


static char *names[N_NAMES]={
	"Terminator",
	"Jack The Ripper",
	"Rambo",
	"Exhumator",
	"Assassin",
	"Arnold",
	"Necromancer",
	"Predator",
	"Rocky",
	"Harvester",
	"Lamer",
	"Killme",
	"Looser",
	"Krueger",
	"I'll kill you",
	"Zombieman",
	"Hellraiser",
	"Eraser"
};

static int direction=0;  /* 0=stop, 1=left, 2=right */
static int const1,const2,const3,const4;
static unsigned short port=DEFAULT_PORT;
static char *host;
static int priority;
/* 	0=nothing
 *	1=kill player
 *	2=find rifle
 *	3=find shotgun
 *	4=find UZI
 *	5=find grenades
 *	6=find gun ammo
 *	7=find shotgun ammo
 *	8=find rifle ammo
 *	9=find uzi ammo
 *	10=find medikit 
 */

/*-----------------------------------------------------------------------*/


#define  can_see(a,b)	(a<add_int(hero->x,CAN_SEE_X)&&a>sub_int(hero->x,CAN_SEE_X)&&b>sub_int(hero->y,CAN_SEE_Y)&&b<add_int(hero->y,CAN_SEE_Y))


static int odds(int p)
{
	return (random()%1000)<p;
}


/* free all before exit */
static void clear_memory(void)
{
	struct object_list *o;

	for (o=&objects;o->next;)
		delete_obj(o->next->member.id);
	free_area();
	shutdown_sprites();
	free_sprites(0);
}


/* shut down the client */
static void shut_down(int a)
{
	if (a)
	{
		clear_memory();
		free_packet_buffer();
		check_memory_leaks();
		EXIT(0);
	}
}


/* find address of server and fill the server address structure */
static char * find_server(char *name,unsigned short port)
{
	struct hostent *h;
	
	h=gethostbyname(name);
	if (!h)return "Error: Can't resolve server address.\n";

	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	server.sin_addr=*((struct in_addr*)(h->h_addr_list[0]));
	return 0;
}


/* initialize socket */
static char * init_socket(void)
{
	fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(fd<0)return "Can't get socket.\n";
	return 0;
}


#define MAX_COUNT 32

/* send quit request to server */
static void send_quit(void)
{
	char p;
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
static char * contact_server(int color, char *name)
{
	static char packet[256];
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
	if (l>MAX_NAME_LEN) {l=MAX_NAME_LEN; packet[5+MAX_NAME_LEN]='\0';}
	memcpy(packet+5,name,l);

	printf("Using name \"%s\".\n", packet+5);

	send_packet(packet,l+5,(struct sockaddr*)(&server),my_id,0);


        if (!select(fd+1,&fds,NULL,NULL,&tv))return "No reply within 4 seconds.\n";

	if ((r=recv_packet(packet,256,0,0,1,0,0))<0)
	{
		if (errno==EINTR)return "Server hung up.\n";
		else return "Connection error.\n";
	}

	switch(*packet)
	{
		case P_PLAYER_REFUSED:
		switch(packet[1])
		{
			case E_INCOMPATIBLE_VERSION:
			return "Incompatible client version. Connection refused.\n";

			case E_NAME_IN_USE:
			return "This name is already in use. Connection refused.\n";

			default:
			return "Connection refused.\n";
		}
		
		case P_PLAYER_ACCEPTED:
		my_id=get_int(packet+35);
		if (r<41){send_quit();return "Incompatible server version. Givin' up.\n";}
		maj=packet[39];
		min=packet[40];
		if (maj!=VERSION_MAJOR||min<MIN_SERVER_VERSION_MINOR)
		{send_quit();return "Incompatible server version. Givin' up.\n";}
		game_start_offset=get_time();
		game_start_offset-=get_long_long(packet+27);
		health=100;
		armor=0;
		for(a=0;a<ARMS;a++)
			ammo[a]=0;
		ammo[0]=weapon[0].basic_ammo;
		current_weapon=0;
		weapons =	WEAPON_MASK_GUN | 
				WEAPON_MASK_GRENADE | 
				WEAPON_MASK_CHAINSAW | 
				WEAPON_MASK_BLOODRAIN;  /* gun, grenades, chainsaw
							   and bloodrain */
		hero=new_obj(
			get_int(packet+1),   /* ID */
			T_PLAYER,   /* type */
			0,  /* time to live */
			get_int16(packet+5),   /* sprite */
			0,        /* position */
			get_int(packet+23),        /* status */
			get_int(packet+7),     /* X */
			get_int(packet+11),    /* Y */
			get_int(packet+15),    /* XSPEED */
			get_int(packet+19),    /* YSPEED */
			0
			);
		break;
		
		default:
		return "Connection error.\n";
	}
	return 0;
}


/* I want to be born again */
static void send_reenter_game(void)
{
	char packet;
	packet=P_REENTER_GAME;
	send_packet(&packet,1,(struct sockaddr*)(&server),my_id,0);
}


/* send chat message */
static void send_message(char *msg)
{
	static char packet[MAX_MESSAGE_LENGTH + 2];
	int len;

	len = strlen(msg) + 1;
	packet[0] = P_MESSAGE;
	packet[1] = 0x00;
	memcpy(packet + 2, msg, len);
	send_packet(packet, len + 2, (struct sockaddr *)(&server), my_id, 0);
}


/* send end of game to server */
static void send_keyboard(void)
{
	char packet[3];
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


static void reset_keyboard(void)
{
	keyboard_status.left=0;
	keyboard_status.right=0;
	keyboard_status.speed=1;
	keyboard_status.jump=0;
	keyboard_status.creep=0;
	keyboard_status.fire=0;
	keyboard_status.weapon=0;
	keyboard_status.down_ladder=0;
}


static void test_object(struct it *obj)
{
	if (obj==hero)return;
	if (!can_see(obj->x,obj->y))return;
	switch(obj->type)
	{
		case T_PLAYER:
		if (obj->status&S_INVISIBLE)
			break;
		if (priority>1)
			break;
		priority=1;
		keyboard_status.fire=1;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		if (my_abs(obj->x-hero->x)<int2double(30))direction=0;
		break;

		case T_BULLET:
		case T_SHRAPNEL:
		case T_BFGCELL:
		case T_CHAIN:
		if ((my_sgn(hero->x-obj->x)==my_sgn(obj->xspeed)||my_abs(hero->x-obj->x)<int2double(20))&&obj->y>=hero->y&&obj->y<hero->y+int2double(PLAYER_HEIGHT))
			keyboard_status.creep=1;
		break;

		case T_RIFLE:
		if (priority>2)break;
		if ((weapons&(1<<WEAPON_RIFLE))&&ammo[WEAPON_RIFLE])break;
		priority=2;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_SHOTGUN:
		if (priority>3)break;
		if ((weapons&(1<<WEAPON_SHOTGUN))&&ammo[WEAPON_SHOTGUN])break;
		priority=3;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_UZI:
		if (priority>4)break;
		if ((weapons&(1<<WEAPON_UZI))&&ammo[WEAPON_UZI])break;
		priority=4;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_AMMO_GRENADE:
		if (priority>5)break;
		if (!(weapons&(1<<WEAPON_GRENADE))||ammo[WEAPON_GRENADE])break;
		priority=5;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_AMMO_GUN:
		if (priority>6)break;
		if (!(weapons&(1<<WEAPON_GUN))||ammo[WEAPON_GUN])break;
		priority=6;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_AMMO_SHOTGUN:
		if (priority>7)break;
		if (!(weapons&(1<<WEAPON_SHOTGUN))||ammo[WEAPON_SHOTGUN])break;
		priority=7;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_AMMO_RIFLE:
		if (priority>8)break;
		if (!(weapons&(1<<WEAPON_RIFLE))||ammo[WEAPON_RIFLE])break;
		priority=8;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_AMMO_UZI:
		if (priority>9)break;
		if (!(weapons&(1<<WEAPON_UZI))||ammo[WEAPON_UZI])break;
		priority=9;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

		case T_MEDIKIT:
		if (priority>10)break;
		if (health>70)break;
		priority=10;
		if (obj->x>hero->x)direction=2;
		else direction=1;
		break;

	}
}


/* destroys all objects except hero (before level change) */
static void clean_memory(void)
{
	struct object_list *o;
	
	/* delete all objects except hero */
	for (o=&objects;o->next;)
		if ((hero->id)!=(o->next->member.id))delete_obj(o->next->member.id);
		else o=o->next;
}


static void change_level(void)
{
	char *LEVEL;
	char txt[256];

	clean_memory();
	free_sprites(level_sprites_start);
	reinit_area();

	LEVEL=load_level(level_number);
	snprintf(txt,256,"Loading level \"%s\".\n",LEVEL);
	ERROR(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,LEVEL_SPRITES_SUFFIX);
	load_sprites(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,STATIC_DATA_SUFFIX);
	load_data(txt);
	mem_free(LEVEL);
}


/* recompute object positions */
static void update_game(void)
{
	struct object_list *p;
	int w,h;
	unsigned char stop_x,stop_y,sy;
	unsigned long_long t;
	int x,y,x1,y1,DELTA_TIME;

	for(p=&objects;p->next;p=p->next)
	{
		if (p->next->member.type == T_NOTHING)
			continue;
		if (p->next->member.status & S_DEAD)
			continue;   /* dead player */
		/* decrement time to live */
		if (p->next->member.ttl>0)
		{
			p->next->member.ttl--;
			if (!p->next->member.ttl)
			{
				if ((p->next->member.type)==T_PLAYER)
					p->next->member.status &= ~S_SHOOTING;
				else
				{
					if (p->next->member.type!=T_GRENADE){  /* client's waiting for P_EXPLODE_GRENADE and doesn't delete the grenade yet */
		                        p=p->prev; 
					delete_obj(p->next->next->member.id);
					continue;}
				}
			}
		}
		test_object(&(p->next->member));
		/* maintain only objects that you are allowed to maintain */
		if (!(obj_attr[p->next->member.type].maintainer&1))continue;
		
		
		/* if not falling slow down x motion */
		if (!(p->next->member.status & S_FALLING))
			p->next->member.xspeed=mul(p->next->member.xspeed,obj_attr[p->next->member.type].slow_down_x);
		
		/* fall */
		if (obj_attr[p->next->member.type].fall)
		{
			p->next->member.status |= S_FALLING;
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
		if (stop_x&&p->next->member.type==T_PLAYER&&!(p->next->member.status & S_CREEP))
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
			p->next->member.status &= ~S_WALKING;
		}


		if (stop_y)
		{
			p->next->member.yspeed=mul(p->next->member.yspeed,obj_attr[p->next->member.type].bounce_y);
			p->next->member.yspeed=-p->next->member.yspeed;
			if (my_abs(p->next->member.yspeed)<MIN_Y_SPEED)
			{
				p->next->member.yspeed=0;
				if (stop_y==1)p->next->member.status &= ~S_FALLING;
			}
		}

		if ((p->next->member.type == T_SHRAPNEL || p->next->member.type == T_BULLET ||
			p->next->member.type == T_BFGCELL || p->next->member.type == T_CHAIN) &&
			(stop_x || stop_y)) { /* bullet and shrapnel die crashing into wall */
			p=p->prev;  /* deleting object makes a great mess in for cycle, so we must cheat the cycle */
			delete_obj(p->next->next->member.id);
			continue;
		}

	}
}


/* returns number of read bytes */
static int process_packet(char *packet,int l)
{
	int a,n=l;

	switch(*packet)
	{
		case P_CHUNK:
		for (a=1;a<l&&a<MAX_PACKET_LENGTH;a+=n)
			n=process_packet(packet+a,l-a);
		break;

		case P_NEW_OBJ:
		if (l < (n=30))break;  /* invalid packet */
		new_obj(
			get_int(packet+1), /* ID */
			packet[27],  /* type */
			get_int16(packet+28), /* time to live */
			get_int16(packet+5),  /* sprite */
			0, /* anim position */
			get_int(packet+23),  /* status */
			get_int(packet+7),  /* x */
			get_int(packet+11),  /* y */
			get_int(packet+15),  /* xspeed */
			get_int(packet+19),  /* yspeed */
			0  /* data */
		);
		break;

		case P_PLAYER_DELETED:
		n=1;
		shut_down(1);
		break;
		
		case P_DELETE_OBJECT:
		if (l<5)break;  /* invalid packet */
		delete_obj(get_int(packet+1));
		n=5;
		break;

		case P_UPDATE_OBJECT:
		if (l < (n=28))break;   /* invalid packet */
		{
			struct object_list *p;
			
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_int(packet+6);
			p->member.y=get_int(packet+10);
			p->member.xspeed=get_int(packet+14);
			p->member.yspeed=get_int(packet+18);
			p->member.status=get_int(packet+22);
			p->member.data=0;
			p->member.ttl=get_int16(packet+26);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
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
			p->member.x=get_int(packet+6);
			p->member.y=get_int(packet+10);
			p->member.xspeed=get_int(packet+14);
			p->member.yspeed=get_int(packet+18);
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
			p->member.xspeed=get_int(packet+6);
			p->member.yspeed=get_int(packet+10);
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
			p->member.x=get_int(packet+6);
			p->member.y=get_int(packet+10);
		}
		break;

		case P_UPDATE_OBJECT_SPEED_STATUS:
		if (l < (n=18))break;   /* invalid packet */
		{
			struct object_list *p;
			
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.xspeed=get_int(packet+6);
			p->member.yspeed=get_int(packet+10);
			p->member.status=get_int(packet+14);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_COORDS_STATUS:
		if (l < (n=18))break;   /* invalid packet */
		{
			struct object_list *p;
			
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_int(packet+6);
			p->member.y=get_int(packet+10);
			p->member.status=get_int(packet+14);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_SPEED_STATUS_TTL:
		if (l < (n=20))break;   /* invalid packet */
		{
			struct object_list *p;
			
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.xspeed=get_int(packet+6);
			p->member.yspeed=get_int(packet+10);
			p->member.status=get_int(packet+14);
			p->member.ttl=get_int16(packet+18);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_OBJECT_COORDS_STATUS_TTL:
		if (l < (n=20))break;   /* invalid packet */
		{
			struct object_list *p;
			
			p=find_in_table(get_int(packet+1));
			if (!p)break;
			if(packet[5]-(p->member.update_counter)>127)break; /* throw out old updates */
			p->member.update_counter=packet[5];
			p->member.x=get_int(packet+6);
			p->member.y=get_int(packet+10);
			p->member.status=get_int(packet+14);
			p->member.ttl=get_int16(packet+18);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_STATUS:
		if (l < (n=9))break;   /* invalid packet */
		{
			struct object_list *p;

			p=find_in_table(get_int(packet+1));
			if (!p)break;  /* ignore objects we don't have */
			p->member.status=get_int(packet+5);
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
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
			p->member.status |= S_HIT;
			p->member.data=(void*)(long)((packet[5]<<16)+(packet[7]<<8)+(packet[6]));
			/* kdyz tasi, tak se nahodi ttl */
			if (p->member.type==T_PLAYER&&(p->member.status & S_HOLDING)&&(p->member.status & S_SHOOTING))
				p->member.ttl=weapon[current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
		}
		break;

		case P_UPDATE_PLAYER:
		if (l<15+2*ARMS)break;  /* invalid packet */
		health=packet[1];
		armor=packet[2];
		for (a=0;a<ARMS;a++)
			ammo[a]=get_int16(packet+3+(a<<1));
		frags=get_int(packet+3+ARMS*2);
		deaths=get_int(packet+7+ARMS*2);
		current_weapon=get_int16(packet+11+2*ARMS);
		weapons=get_int16(packet+13+2*ARMS);
		n=15+2*ARMS;
		break;

		case P_MESSAGE:
			if (l < 3)
				break;   /* invalid packet */
			n = 3 + strlen(packet + 2);
			break;

		case P_END:
		if (l<2)printf("Game terminated.\n");
		else printf("Game terminated by %s.\n",packet+1);
		n=2+strlen(packet+1);
		shut_down(1);

		case P_INFO:
		if (l<=5)break;
		l=6;
		for (a=0;a<packet[5]&&a<TOP_PLAYERS_N;a++)
		{
			int x;
			x=strlen(packet+l+9)+1;
			l+=x+9;
		}
		n=l;
		break;

		case P_EXPLODE_GRENADE:
		case P_EXPLODE_BFG:
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
				int spd=add_int(mul_int(my_and(mul_int(weapon[WEAPON_GRENADE].speed,b+1),15),16),100);
						
				new_obj(
					i,
					T_SHRAPNEL,
					SHRAPNEL_TTL,
					0,
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

		case P_CHANGE_LEVEL:
		{
			char *md5;
			int a;
			char p;

			if (l<38)break;   /* invalid packet */
			a=get_int(packet+1);
			if (level_number==a)goto level_changed;
			level_number=a;
			
			md5=md5_level(level_number);
			if (strcmp((char *)md5,packet+5))   /* MD5s differ */
			{
				mem_free(md5);
				ERROR("Invalid MD5 sum. Can't change level. Exiting...");
				send_quit();
				shut_down(1);
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

	}
	return n;
}


/* read packet from socket */
static void read_data(void)
{
        fd_set rfds;
        struct timeval tv;
        struct sockaddr_in client;
        static char packet[MAX_PACKET_LENGTH];
        int a=sizeof(client);
	int l;

        tv.tv_sec=0;
        tv.tv_usec=0;
        FD_ZERO(&rfds);
        FD_SET(fd,&rfds);
	while(select(fd+1,&rfds,0,0,&tv))
	{
	        if ((l=recv_packet(packet,MAX_PACKET_LENGTH,(struct sockaddr*)(&client),&a,1,my_id,0))<0)
			return;   /* something's strange */
		process_packet(packet,l);

	}
}


/* handle fatal signal (sigabrt, sigsegv, ...) */
static void signal_handler(int signum)
{
	
	if (connected)send_quit();
	shut_down(0);
	clear_memory();
	if (fd) close(fd);
#ifdef HAVE_PSIGNAL
	psignal(signum,"Exiting on signal");
#else
	fprintf(stderr, "Exiting on signal: %d\n", signum);
#endif
	if (!in_signal_handler){in_signal_handler=1;check_memory_leaks();}
	free_packet_buffer();
	EXIT(1);
}


/* print command line help */
static void print_help(void)
{
        printf( 
		"0verkill bot"
		".\n"
                "(c)2000 Brainsoft\n"
                "Usage: bot [-h] -a <server address> [-p <port>] [-n <name>]"
		"\n"
	);
}


static void parse_command_line(int argc,char **argv)
{
        int a;
	char *e;

        while(1)
        {
                a=getopt(argc,argv,"hp:a:n:");
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

			case 'a':
			host=optarg;
			break;

			case 'n':
			static_name=optarg;
			break;

			case 'p':
			port=(unsigned short)strtoul(optarg,&e,10);
			if (*e){ERROR("Error: Decimal number expected.\n");EXIT(1);}
			break;
                }
        }
}


static char * select_name(void)
{
	return static_name? static_name:names[(random())%N_NAMES];
}


static void where2go(void)
{
	if (!direction)
	{
		if (odds(20+const2))direction=2;
		if (odds(20+const2))direction=1;
	}
	if(direction==1)keyboard_status.left=1;
	if(direction==2)keyboard_status.right=1;

	if (odds(40+const1))keyboard_status.jump=1;
	if (odds(100+const4))keyboard_status.down_ladder=1;
	if (odds(3)+const3)keyboard_status.fire=1;

}


static void action(void)
{
	if (hero->status & S_DEAD)
		send_reenter_game();  /* respawn */
	where2go();
}

/*----------------------------------------------------------------------------*/
int main(int argc,char **argv)
{
	int color;
	unsigned long_long last_time;
	char *m;


#ifdef WIN32
	WSADATA wd;

	WSAStartup(0x101, &wd);
	printf("Started WinSock version %X.%02X\n", wd.wVersion/0x100, wd.wVersion&0xFF);
#endif

	chdir_to_data_files();
	ERROR("Initialization...\n");

	last_obj=&objects;
	srandom(get_time());

	const1=random()%50;
	const2=random()%20;
	const3=random()%5;
	const4=random()%20;
	
	host=NULL;
	parse_command_line(argc,argv);
	if (!host){ERROR("No server address specified.\n");EXIT(1);}
	hash_table_init();
	init_sprites();
	init_area();
	ERROR("Loading graphics...\n");
	load_sprites(DATA_PATH GAME_SPRITES_FILE);
	level_sprites_start=n_sprites;

	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGILL,signal_handler);
	signal(SIGABRT,signal_handler);
	signal(SIGFPE,signal_handler);
	signal(SIGSEGV,signal_handler);
#ifndef WIN32
	signal(SIGQUIT,signal_handler);
	signal(SIGBUS,signal_handler);
#endif

	ERROR("Resolving server address ...\n");
	if ((m=find_server(host,port))){ERROR(m);EXIT(1);}
	ERROR("Initializing socket ...\n");
	if ((m=init_socket())){ERROR(m);EXIT(1);}
	ERROR("Contacting server ...\n");
	color=(random()%15)+1;
	if ((m=contact_server(color,select_name()))){ERROR(m);EXIT(1);}

	connected=1;
	ERROR("OK, connected.\nPlaying...\n");

	last_time=get_time();
again:
	last_time+=CLIENT_PERIOD_USEC;
	if (get_time()-last_time>PERIOD_USEC*100)last_time=get_time();
	read_data();
	reset_keyboard();
	priority=0;
	update_game();
	action();
	send_keyboard();
	sleep_until(last_time+CLIENT_PERIOD_USEC);
	goto again;
	if (fd) close(fd);
}
