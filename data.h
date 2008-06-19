#ifndef __DATA_H
#define __DATA_H

#ifndef WIN32
#include "config.h"
#endif
#include "cfg.h"
#include "sprite.h"
#include "math.h"

#define TYPE_BACKGROUND 0
#define TYPE_WALL 16
#define TYPE_JUMP 32  /* proskakovatko */
#define TYPE_FOREGROUND 48  /* pred maniakem */
#define TYPE_JUMP_FOREGROUND 64  /* proskakovatko pred maniakem */

#define TYPE_BIRTHPLACE 666

/* object types */
#define N_TYPES 22

#define T_PLAYER 0
#define T_BULLET 1
#define T_CORPSE 2
#define T_MEDIKIT 3
#define T_SHOTGUN 4
#define T_UZI 5
#define T_RIFLE 6
#define T_SHELL 7
#define T_AMMO_GUN 8
#define T_AMMO_SHOTGUN 9
#define T_AMMO_UZI 10
#define T_AMMO_RIFLE 11
#define T_NOTHING 12
#define T_MESS 13
#define T_GRENADE 14
#define T_AMMO_GRENADE 15
#define T_SHRAPNEL 16
#define T_ARMOR 17
#define T_INVISIBILITY 18
#define T_NOISE 19
#define T_NOTHING_FORE 20
#define T_KILL 21


#define E_INCOMPATIBLE_VERSION 0
#define E_PLAYER_REFUSED 1


#define WEAPON_GRENADE 4
#define WEAPON_RIFLE 3
#define WEAPON_UZI 2
#define WEAPON_SHOTGUN 1
#define WEAPON_GUN 0

#define WEAPON_MASK_GRENADE 16
#define WEAPON_MASK_RIFLE 8
#define WEAPON_MASK_UZI 4
#define WEAPON_MASK_SHOTGUN 2
#define WEAPON_MASK_GUN 1


#define ARMS 5

unsigned char *weapon_name[ARMS];

/* STATUS
0: walk
1,2: left(01), right(10), center (00)
3: fall
4: shoot
5: holding gun
6: hidden
7: hit
8: creep
9: throwing grenade
10: dead
*/


/* object attribute table */
struct obj_attr_type
{
	unsigned char fall;   /* 1=can fall, 0=can't */
	my_double bounce_x,bounce_y;  /* slow down during horizontal/vertical bouncing */
	my_double slow_down_x;   /* slow down when not falling, speed is multiplied with this constant */
	unsigned char maintainer;   
	unsigned char foreground;   /* is this object in foreground? */
	       /* who computes the object: 
		      bit 0=clients update
		      bit 1=server updates
		      bit 2=server sends updates to clients
		      */
}obj_attr[N_TYPES];


/* weapon attribut table */
struct weapon_type
{
	unsigned char *name;
	unsigned char cadence;
	int ttl:16;
	my_double speed,impact;
	unsigned char lethalness;
	unsigned char armor_damage;
	unsigned char basic_ammo;
	unsigned char add_ammo;
	unsigned char max_ammo;
	my_double shell_xspeed,shell_yspeed;
}weapon[ARMS];


/* object in the game */
struct it
{
	my_double x,y,xspeed,yspeed;  /* position and velocity */
	unsigned char type;  /* one of T_...... constants */
	unsigned int id:24;   /* unique ID */
	int ttl:16;   /* time to live */
	unsigned int sprite:16;  /* sprite number */
	unsigned int anim_pos:16;  /* animating position */
	unsigned int status:16;  /* status - especially for hero objects */
	void * data;  /* object's own data, this is individual filled */
	unsigned char update_counter;  /* increased with each update of the object */
	unsigned long_long last_updated;   /* last time object was updated */
};


/* object list */
struct object_list
{
        struct object_list *next,*prev;
        struct it member;
};


extern struct object_list *last_obj;

extern struct sprite *sprites;
extern unsigned char **sprite_names;
extern int n_sprites;

/* static map */
extern unsigned char *area;
extern unsigned char *area_a;

extern void load_sprites(unsigned char *);
extern void load_data(unsigned char *);
extern int find_sprite(unsigned char *,int *);
extern void init_area(void);
extern void reinit_area(void);
extern void free_area(void);
extern unsigned char *md5_level(int);
extern struct it* new_obj(
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
	void * data);
void delete_obj(unsigned long id);
extern void put_int(unsigned char *p,int num);
extern int get_int(unsigned char *p);
extern void put_int16(unsigned char *p,int num);
extern int get_int16(unsigned char *p);
extern void put_int(unsigned char *p,int num);
extern int get_int(unsigned char *p);
extern void put_float(unsigned char *p,my_double num);
extern my_double get_float(unsigned char *p);
extern void put_long_long(unsigned char *p,unsigned long_long num);
extern unsigned long_long get_long_long(unsigned char *p);
extern void free_sprites(int);

#ifdef HAVE_INLINE
	extern inline void 
#else
	extern void
#endif
get_dimensions(int type,int status,struct pos *s,int *w,int *h);

void update_position(
	struct it* obj,
	my_double new_x,
	my_double new_y,
	int width,
	int height,
	unsigned char *fx,
	unsigned char *fy
	);

extern void _skip_ws(char**txt);
extern int _convert_type(unsigned char c);
extern unsigned char * load_level(int);
#endif
