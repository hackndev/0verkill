#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include "config.h"
#endif

#include "data.h"
#include "cfg.h"
#include "hash.h"
#include "time.h"
#include "md5.h"
#include "error.h"


#ifdef TRI_D
int tri_d=0;
int TRI_D_ON=0;
#endif


/* static map of the level */
unsigned char *area;

/* attributes:
	lower 4 bits=color
	higher 4 bits=type
*/
unsigned char *area_a;


struct sprite *sprites=DUMMY;
unsigned char **sprite_names=DUMMY;
int n_sprites;  /* number of sprites */

struct object_list *last_obj;

#define NOBODY 0
#define CLIENT 1
#define BOTH 3
#define BOTH_UPDATE 7

/* object attributes */
struct obj_attr_type obj_attr[N_TYPES]=
{
	/* fall, bounce x, bounce y, slow down x, maintainer, foreground */
	{1,0,0,PLAYER_SLOW_DOWN_X,BOTH_UPDATE,0},  /* player */
	{0,0,0,int2double(1),BOTH,0},  /* bullet */
	{1,0,float2double(.25),PLAYER_SLOW_DOWN_X,BOTH,0},  /* corpse */
	{1,0,0,0,NOBODY,0},  /* medikit */
	{1,0,0,0,NOBODY,0},  /* shotgun */
	{1,0,0,0,NOBODY,0},  /* uzi */
	{1,0,0,0,NOBODY,0},  /* rifle */
	{1,float2double(.8),float2double(.5),float2double(.8),CLIENT,0},  /* shell */
	{1,0,0,0,NOBODY,0},  /* ammo for gun */
	{1,0,0,0,NOBODY,0},  /* ammo for shotgun */
	{1,0,0,0,NOBODY,0},  /* ammo for uzi */
	{1,0,0,0,NOBODY,0},  /* ammo for rifle */
	{0,0,0,0,NOBODY,0},  /* nothing */
	{1,float2double(.4),float2double(.4),float2double(.3),CLIENT,0},   /* mess */
	{1,float2double(.5),float2double(.5),float2double(.9),BOTH/*_UPDATE*/,0},    /* grenade */
	{1,0,0,0,NOBODY,0},  /* grenade ammo */
	{1,0,0,int2double(1),BOTH,0},  /* grenade shrapnel */
	{1,0,0,0,NOBODY,0},  /* armor */
	{1,0,0,0,NOBODY,0},  /* invisibility */
	{1,0,0,0,NOBODY,0},  /* noise */
	{0,0,0,0,NOBODY,1},  /* nothing in foreground */
	{0,0,0,0,NOBODY,1},  /* killing object */
};


/* weapon attributes */
struct weapon_type weapon[ARMS]=
{
	/* name, cadence, ttl, bullet speed, impact, lethalness, armor damage, basic ammo, additional ammo, max ammo, shell xspeed, shell yspeed */
	{"Browning",16,50,float2double(3*36),float2double(.3*36),20,2,13,12,48,float2double((double).3*36),-float2double(1*36)},
	{"Shotgun",25,50,float2double(3*36),float2double(.5*36),10,5,6,12,30,float2double((double).3*36),-float2double((double)1.2*36)},
	{"Uzi",3,50,float2double(4*36),float2double(.25*36),15,4,50,50,150,float2double((double).9*36),-float2double((double)1.5*36)},
	{"Rifle",40,70,float2double(6*36),float2double(.4*36),50,20,1,15,15,0,0},
	{"Grenades",15,60,float2double((double)3.73*36),0,75,40,0,6,24,float2double(3*36),-float2double((double)1.5*36)}  /* shell speed=grenade throwing speed, bullet speed=shrapnel speed */
};


/* initialize playing area */
/* must be run before loading data */
void init_area(void)
{
	area=mem_alloc(AREA_X*AREA_Y);
	if (!area){ERROR("Error: Not enough memory!\n");EXIT(1);}
	area_a=mem_alloc(AREA_X*AREA_Y);
	if (!area_a){ERROR("Error: Not enough memory!\n");EXIT(1);}
	memset(area,' ',AREA_X*AREA_Y);
	memset(area_a,0,AREA_X*AREA_Y);
}

void free_area(void)
{
	mem_free(area);
	mem_free(area_a);
}


/* reinitializes playing area */
/* must be called before loading new level */
void reinit_area(void)
{
	memset(area,' ',AREA_X*AREA_Y);
	memset(area_a,0,AREA_X*AREA_Y);
}

/* skip white space */
/* ancillary function */
void _skip_ws(char **txt)
{
 for (;(**txt)==' '||(**txt)==9||(**txt)==10;(*txt)++);
}


/* find sprite according to its name */
/* returns 1 on error */
/* it's slow but not called in speed critical parts of the program */
int find_sprite(unsigned char *name,int *num)
{
 for ((*num)=0;(*num)<n_sprites;(*num)++)
  if (!strcmp(sprite_names[*num],name))return 0;
 return 1;
}


/* convert type character (from data files) into type */
int _convert_type(unsigned char c)
{
 switch(c)
 {

/* static objects */
  case 'b': return TYPE_BACKGROUND;
  
  case 'w': return TYPE_WALL;
  
  case 'j': return TYPE_JUMP;

  case 'f': return TYPE_FOREGROUND;

  case 'i': return TYPE_JUMP_FOREGROUND;

/* dynamic objects, they use great letter */
  
  case 'M': return T_MEDIKIT;

  case 'A': return T_ARMOR;

  case 'N': return T_NOTHING;
  
  case 'F': return T_NOTHING_FORE;

  case 'K': return T_KILL;

  case 'S': return T_SHOTGUN;

  case 'U': return T_UZI;
  
  case 'R': return T_RIFLE;
  
  case '1': return T_AMMO_GUN;
  
  case '2': return T_AMMO_SHOTGUN;
  
  case '3': return T_AMMO_UZI;
  
  case '4': return T_AMMO_RIFLE;
  
  case '5': return T_AMMO_GRENADE;

  case 'I': return T_INVISIBILITY;
  
  /* birthplace */
  case 'B': return TYPE_BIRTHPLACE;

  default:
  return -1;
 }
}


/* load static data */
void load_data(unsigned char * filename)
{
 FILE * stream;
 static char line[1024];
 char *p,*q,*name;
 int n,x,y;
 int t;

 if (!(stream=fopen(filename,"rb")))
 {
	char msg[256];
 	snprintf(msg,256,"Can't open file \"%s\"!\n",filename);
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
  if ((t=_convert_type(*p))<0)
  {
	char msg[256];
  	snprintf(msg,256,"Unknown object type '%c'.\n",*p);
	ERROR(msg);
	EXIT(1);
  }
  p++;
  _skip_ws(&p);
  x=strtol(p,&q,0);
  _skip_ws(&q);
  y=strtol(q,&p,0);
  if (find_sprite(name,&n))
  {
	char msg[256];
  	snprintf(msg,256,"Unknown bitmap name \"%s\"!\n",name);
	ERROR(msg);
	EXIT(1);
  }
  _put_sprite(AREA_X,AREA_Y,area,area_a,x,y,sprites[n].positions,t,0);
 }
 fclose(stream);
}


/* load sprites */
void load_sprites(unsigned char * filename)
{
 FILE *stream;
 static char line[1024];
 char *p,*q;
 int l;

 stream=fopen(filename,"rb");
 if (!stream)
 {
 	char msg[256];
	snprintf(msg,256,"Can't open file \"%s\"!\n",filename);
	ERROR(msg);
	EXIT(1);
 }
 while(fgets(line,1024,stream))
 {
  p=line;
  _skip_ws(&p);
  for (q=p;(*p)!=' '&&(*p)!=9&&(*p)!=10&&(*p);p++);
  if (!(*p))continue;
  *p=0;p++;
  l=strlen(q);
  n_sprites++;
  sprite_names=(unsigned char **)mem_realloc(sprite_names,n_sprites*sizeof(unsigned char*));
  if (!sprite_names){ERROR("Memory allocation error!\n");EXIT(1);}
  sprites=(struct sprite *)mem_realloc(sprites,n_sprites*sizeof(struct sprite));
  if (!sprites){ERROR("Memory allocation error!\n");EXIT(1);}
  sprite_names[n_sprites-1]=(char *)mem_alloc(l+1);
  if (!sprite_names[n_sprites-1]){ERROR("Memory allocation error!\n");EXIT(1);}
  memcpy(sprite_names[n_sprites-1],q,l+1);
  _skip_ws(&p);
  for (q=p;(*p)!=' '&&(*p)!=9&&(*p)!=10&&(*p);p++);
  *p=0;p++;
  load_sprite(q,sprites+(n_sprites-1));
 }
 fclose(stream);
}


void free_sprites(int start_num)
{
	int a;

	for (a=start_num;a<n_sprites;a++)
	{
		mem_free(sprite_names[a]);
		free_sprite(sprites+a);
	}
	n_sprites=start_num;
  	sprite_names=(unsigned char **)mem_realloc(sprite_names,n_sprites*sizeof(unsigned char*));
	if (!sprite_names){ERROR("Memory allocation error!\n");EXIT(1);}
	sprites=mem_realloc(sprites,n_sprites*sizeof(struct sprite));
	if (!sprites){ERROR("Memory allocation error!\n");EXIT(1);}
}


/* returns allocated string with level name or NULL on error */
/* level_num is a line number in the LEVEL_FILE */
unsigned char *load_level(int level_num)
{
	unsigned char txt[1024];
	unsigned char *retval;
	int a;
	FILE *f;

	f=fopen(DATA_PATH LEVEL_FILE,"r");
	if (!f)return NULL;

	for (a=0;a<=level_num;)
	{
		if (!(fgets(txt,1024,f)))return NULL;
		
		/* remove trailing CR and/or LF */
		if (txt[strlen(txt)-1]==10)txt[strlen(txt)-1]=0;
		if (txt[strlen(txt)-1]==13)txt[strlen(txt)-1]=0;

		if (strlen(txt))a++;
	}

	fclose(f);

	a=strlen(txt);
	retval=mem_alloc(1+a*sizeof(unsigned char));
	if (!retval)return NULL;
	memcpy(retval,txt,a+1); /* including trailing zero */
	if (!strlen(retval)){mem_free(retval);return NULL;}
	return retval;

}

/* create a new object and fill with data and add to hash table */
struct it * new_obj(
	unsigned int id,
	unsigned char type, 
	int ttl, 
	int sprite, 
	unsigned char pos, 
	int status,
	my_double x,
	my_double y, 
	my_double xspeed, 
	my_double yspeed, 
	void * data)
{
        last_obj->next=mem_alloc(sizeof(struct object_list));
        if (!last_obj->next)return 0;
        last_obj->next->prev=last_obj;
        last_obj=last_obj->next;
        last_obj->next=0;
        last_obj->member.x=x;
        last_obj->member.y=y;
        last_obj->member.xspeed=xspeed;
        last_obj->member.yspeed=yspeed;
        last_obj->member.type=type;
        last_obj->member.ttl=ttl;
        last_obj->member.sprite=sprite;
        last_obj->member.anim_pos=pos;
        last_obj->member.data=data;
	last_obj->member.id=id;
	last_obj->member.status=status;
	last_obj->member.update_counter=0;
	last_obj->member.last_updated=get_time();
	add_to_table(last_obj);
        return &(last_obj->member);
}


/* completely delete object from the list */
void delete_obj(unsigned long id)
{
	struct object_list *q;
	if (!(q=remove_from_table(id)))return;  /* packets can come more than once, so we must ignore deleting deleted object */
	q->prev->next=q->next;
	if (!q->next) last_obj=q->prev;  /* q is last object in list */
	else q->next->prev=q->prev;
	mem_free(q);
}


void put_long_long(unsigned char *p,unsigned long_long num)
{
        p[0]=num&255;num>>=8;
        p[1]=num&255;num>>=8;
        p[2]=num&255;num>>=8;
        p[3]=num&255;num>>=8;
        p[4]=num&255;num>>=8;
        p[5]=num&255;num>>=8;
        p[6]=num&255;num>>=8;
        p[7]=num&255;
}


void put_int(unsigned char *p,int num)
{
        p[0]=num&255;num>>=8;
        p[1]=num&255;num>>=8;
        p[2]=num&255;num>>=8;
        p[3]=num&255;
}


int get_int(unsigned char *p)
{
        return 	(int)p[0]+
		((int)(p[1])<<8)+
		((int)(p[2])<<16)+
		((int)(p[3])<<24);
}


void put_int16(unsigned char *p,int num)
{
        p[0]=num&255;num>>=8;
        p[1]=num&255;
}


int get_int16(unsigned char *p)
{
        return (int)p[0]+((int)(p[1])<<8);
}


my_double get_float(unsigned char *p)
{
	int a;
	a=	(int)p[0]+
		((int)(p[1])<<8)+
		((int)(p[2])<<16)+
		((int)(p[3])<<24);
	return a;
}


void put_float(unsigned char *p,my_double num)
{
        p[0]=num&255;num>>=8;
        p[1]=num&255;num>>=8;
        p[2]=num&255;num>>=8;
        p[3]=num&255;
}


unsigned long_long get_long_long(unsigned char *p)
{
#define ULL unsigned long_long
        return 	(ULL)p[0]+
		((ULL)(p[1])<<8)+
		((ULL)(p[2])<<16)+
		((ULL)(p[3])<<24)+
		((ULL)(p[4])<<32)+
		((ULL)(p[5])<<40)+
		((ULL)(p[6])<<48)+
		((ULL)(p[7])<<56);
#undef ULL
}



/* test if vertical line from yh to yl can move from old_x to new_x axis */
/* returns farthest possible x axis */
/* flag is filled with 1 if objects is stopped */
my_double can_go_x(my_double old_x,my_double new_x,int yh, int yl,unsigned char *flag)
{
	int x,y;
	
	if (old_x==new_x+.5)
	{
		if(flag)*flag=0;
		return new_x;
	}
	if(flag)*flag=1;
	if (old_x<new_x)
		for (x=double2int(old_x)+1;x<=round_up(new_x);x++)  /* go to the right */
		{
			if (x>AREA_X-1) return int2double(AREA_X-1);
			for (y=yh;y<=yl;y++)
				if ((area_a[x+y*AREA_X]&240)==TYPE_WALL)
					return int2double(x-1);
		}
	else
		for (x=round_up(old_x)-1;x>=double2int(new_x);x--)  /* go to the left */
		{
			if (x<0) return 0;
			for (y=yh;y<=yl;y++)
				if ((area_a[x+y*AREA_X]&240)==TYPE_WALL) return int2double(x+1);
		}
	if(flag)*flag=0;
	return new_x;
}


/* test if horizontal line from xl to xr can move from old_y to new_y axis */
/* returns farthest possible y axis */
/* flag is filled with 1 if objects is stopped */
/* down ladder: 1=fall through ladders etc., 0=stand on ladders */
my_double can_go_y(my_double old_y, my_double new_y,int xl, int xr,unsigned char *flag,unsigned char down_ladder)
{
	int x,y;
	
	if (old_y==new_y){if(flag)*flag=0;return new_y;}
	if(flag)*flag=1;
	if (old_y<new_y)
		for (y=double2int(old_y)+1;y<=round_up(new_y);y++)  /* go down */
		{
			if (y>AREA_Y-1) return int2double(AREA_Y-1);
			for (x=xl;x<=xr;x++)
				if ((area_a[x+y*AREA_X]&240)==TYPE_WALL||(!down_ladder&&((area_a[x+y*AREA_X]&240)==TYPE_JUMP||(area_a[x+y*AREA_X]&240)==TYPE_JUMP_FOREGROUND))) return int2double(y-1);
		}
	else
	{
		if (flag)*flag=2;
		for (y=round_up(old_y)-1;y>=double2int(new_y);y--)  /* go up */
		{
			if (y<0) return 0;
			for (x=xl;x<=xr;x++)
				if ((area_a[x+y*AREA_X]&240)==TYPE_WALL) return int2double(y+1);
		}
	}
	if(flag)*flag=0;
	return new_y;
}


/* automatically computes dimensions of unknown type, if anim positions is given */
/* object must be rectangular */
#ifdef HAVE_INLINE
	inline void 
#else
	void
#endif
get_dimensions(int type,int status,struct pos *s,int *w,int *h)
{
	switch(type)
	{
		case T_PLAYER:
		if (status&256)
		{
			*w=CREEP_WIDTH;
			*h=CREEP_HEIGHT;
		}
		else
		{
			*w=PLAYER_WIDTH;
			*h=PLAYER_HEIGHT;
		}
		return;

		case T_SHOTGUN:
		*w=22;
		*h=3;
		return;

		case T_UZI:
		*w=12;
		*h=3;
		return;

		default:
		*w=0;
		if (!s){*h=0;return;}
		*h=s->n;
		if (*h)*w=s->lines[0].len;
	}
}



/* updates object's position */
void update_position(struct it* obj,my_double new_x,my_double new_y,int width, int height,unsigned char *fx,unsigned char *fy)
{
	unsigned char down_ladder=0;

	/* player is climbing ladder down */
	if (obj->type==T_PLAYER&&obj->status&2048) down_ladder=1;
	
	if (obj->xspeed>0)
		obj->x=sub_int(can_go_x(add_int(obj->x,width-1),add_int(new_x,width-1),double2int(obj->y),round_up(obj->y)+height-1,fx),width-1);
	else
		obj->x=can_go_x(obj->x,new_x,double2int(obj->y),round_up(obj->y)+height-1,fx);
		
	if (obj->yspeed>0)
		obj->y=sub_int(can_go_y(add_int(obj->y,height-1),add_int(new_y,height-1),double2int(obj->x),round_up(obj->x)+width-1,fy,down_ladder),height-1);
	else
		obj->y=can_go_y(obj->y,new_y,double2int(obj->x),round_up(obj->x)+width-1,fy,down_ladder);
}


unsigned char *__add_md5(unsigned char *filename, int *len, unsigned char**result)
{
	unsigned char *p,*q;
	int a;
	
	q=MD5File(filename,NULL);
	a=strlen(q);
	if (!(*result))*result=DUMMY;
	p=mem_realloc((*result),(*len)+a+1);
	if (!result)return NULL;
	(*result)=p;
	memcpy((*result)+(*len),q,a+1);
	mem_free(q);
	(*len)+=a;
	return (*result);
}

/* computes md5 sum from the level
 * level_num is the line number in level.dat file
 * returns allocated string with the MD5 sum or NULL (on error)
 */
unsigned char* md5_level(int level_num)
{
	unsigned char *result=0;
	char *q;
	int len=0;
	unsigned char p[2048];
	
	q=load_level(level_num);
	if (!q)return NULL;
	
	if (!__add_md5(DATA_PATH LEVEL_FILE,&len,&result)){mem_free(result);return NULL;}

	snprintf(p,2048,"%s%s%s",DATA_PATH,q,LEVEL_SPRITES_SUFFIX);
	if (!__add_md5(p,&len,&result)){mem_free(result);return NULL;}

	snprintf(p,2048,"%s%s%s",DATA_PATH,q,STATIC_DATA_SUFFIX);
	if (!__add_md5(p,&len,&result)){mem_free(result);return NULL;}

	snprintf(p,2048,"%s%s%s",DATA_PATH,q,DYNAMIC_DATA_SUFFIX);
	if (!__add_md5(p,&len,&result)){mem_free(result);return NULL;}

	snprintf(p,2048,"%s%s%s",DATA_PATH,q,LEVEL_SPRITES_SUFFIX);
	mem_free(q);

	{
		FILE *f;

		f=fopen(p,"r");
		if (!f){mem_free(result);return NULL;}
		while (fgets(p,2048,f))
		{
			if (p[strlen(p)-1]==13)p[strlen(p)-1]=0;
			if (p[strlen(p)-1]==10)p[strlen(p)-1]=0;

			q=p;
			_skip_ws(&q);
			for (;(*q)&&(*q)!=' '&&(*q)!=9&&(*q)!=10&&(*q)!=13;q++);
			_skip_ws(&q);
			if (!strlen(q))continue;
			if (!__add_md5(q,&len,&result)){mem_free(result);return NULL;}
		}
		fclose(f);
	}
	
	q=MD5Data(result,len,NULL);
	mem_free(result);
	return q;
}
