#ifndef __DATA_H
#define __DATA_H

#ifndef WIN32
#include "config.h"
#else
/* rezim kompatibility s woknama ... */
#include <direct.h>
#define snprintf(t, s, ...) _snprintf_s(t, s, s, __VA_ARGS__)
#define close(...) while(0);
#define chdir(...) _chdir(__VA_ARGS__)
#define random rand
#define srandom(x) srand((unsigned int)x)
#define sleep(x) Sleep(x)
#define strcat(d, s) strcat_s(d, sizeof(s), s)
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
#define N_TYPES 31

#define T_PLAYER	0
#define T_BULLET	1
#define T_CORPSE	2
#define T_MEDIKIT	3
#define T_SHOTGUN	4
#define T_UZI		5
#define T_RIFLE		6
#define T_SHELL		7
#define T_AMMO_GUN	8
#define T_AMMO_SHOTGUN	9
#define T_AMMO_UZI	10
#define T_AMMO_RIFLE	11
#define T_NOTHING	12
#define T_MESS		13
#define T_GRENADE	14
#define T_AMMO_GRENADE	15
#define T_SHRAPNEL	16
#define T_ARMOR		17
#define T_INVISIBILITY	18
#define T_NOISE		19
#define T_NOTHING_FORE	20
#define T_KILL		21
#define T_TELEPORT	22
#define T_BFG		23
#define T_BFGCELL	24
#define T_CHAIN		25
#define T_BIOSKULL	26
#define T_BIOMED	27
#define T_BLOODRAIN	28
#define T_JETPACK	29
#define T_JETFIRE	30

#define E_INCOMPATIBLE_VERSION 0
#define E_PLAYER_REFUSED 1

#define WEAPON_BLOODRAIN 7
#define WEAPON_CHAINSAW 6
#define WEAPON_BFG 5
#define WEAPON_GRENADE 4
#define WEAPON_RIFLE 3
#define WEAPON_UZI 2
#define WEAPON_SHOTGUN 1
#define WEAPON_GUN 0

#define WEAPON_MASK_BLOODRAIN 128
#define WEAPON_MASK_CHAINSAW 64
#define WEAPON_MASK_BFG 32
#define WEAPON_MASK_GRENADE 16
#define WEAPON_MASK_RIFLE 8
#define WEAPON_MASK_UZI 4
#define WEAPON_MASK_SHOTGUN 2
#define WEAPON_MASK_GUN 1

#define	S_WALKING	0x0001
#define S_LOOKLEFT	0x0002
#define S_LOOKRIGHT	0x0004
#define S_FALLING	0x0008
#define S_SHOOTING	0x0010
#define S_HOLDING	0x0020
#define	S_INVISIBLE	0x0040
#define	S_HIT		0x0080
#define S_CREEP		0x0100
#define S_GRENADE	0x0200
#define S_DEAD		0x0400
#define S_CLIMB_DOWN	0x0800
#define S_NOISE		0x1000
#define S_CHAINSAW	0x2000
#define S_ILL		0x4000
#define S_BLOODRAIN	0x8000
#define S_JETPACK	0x10000
#define S_JETPACK_ON	0x20000
#define S_ONFIRE	0x40000
#define S_CHAIN		0x80000

#define	M_DEFAULT	0x00
#define	M_CHAT		0x01
#define	M_INFO		0x02
#define	M_ENTER		0x04
#define	M_LEAVE		0x08
#define	M_AMMO		0x10
#define	M_WEAPON	0x20
#define	M_ITEM		0x40
#define	M_DEATH		0x80

#define	C_BLACK		0x00
#define	C_D_RED		0x01
#define	C_D_GREEN	0x02
#define	C_BROWN		0x03
#define	C_D_BLUE	0x04
#define	C_D_MAGENTA	0x05
#define	C_D_CYAN	0x06
#define	C_GREY		0x07
#define	C_D_GREY	0x08
#define	C_RED		0x09
#define	C_GREEN		0x0a
#define	C_YELLOW	0x0b
#define	C_BLUE		0x0c
#define	C_MAGENTA	0x0d
#define	C_CYAN		0x0e
#define	C_WHITE		0x0f

#define E_NONE		0x00
#define E_DEFAULTS	0x01
#define E_CONN		0x02
#define E_CONN_SUCC	0x04

#define ARMS 8

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
	int bounce_x,bounce_y;  /* slow down during horizontal/vertical bouncing */
	int slow_down_x;   /* slow down when not falling, speed is multiplied with this constant */
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
	char *name;
	unsigned char cadence;
	int ttl:16;
	int speed,impact;
	unsigned char lethalness;
	unsigned char armor_damage;
	unsigned char basic_ammo;
	unsigned char add_ammo;
	unsigned char max_ammo;
	int shell_xspeed,shell_yspeed;
}weapon[ARMS];


/* object in the game */
struct it
{
	int x,y,xspeed,yspeed;  /* position and velocity */
	unsigned char type;  /* one of T_...... constants */
	unsigned int id:24;   /* unique ID */
	int ttl:16;   /* time to live */
	unsigned int sprite:16;  /* sprite number */
	unsigned int anim_pos:16;  /* animating position */
	/* 16b hole in the structure */
	unsigned int status;  /* status - especially for hero objects */
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
extern char **sprite_names;
extern int n_sprites;

/* static map */
extern unsigned char *area;
extern unsigned char *area_a;

extern void load_sprites(char *);
extern void load_data(char *);
extern int find_sprite(char *,int *);
extern void init_area(void);
extern void reinit_area(void);
extern void free_area(void);
extern char *md5_level(int);
extern struct it* new_obj(
	unsigned int id,
	unsigned char type,
	int ttl, 
	int sprite, 
	unsigned char pos, 
	int status, 
	int x,
	int y, 
	int xspeed, 
	int yspeed, 
	void * data);
void delete_obj(unsigned long id);
extern void put_int(char *p,int num, int *offset);
extern int get_int(char *p);
extern void put_int16(char *p,short num, int *offset);
extern int get_int16(char *p);
extern void put_long_long(char *p,unsigned long_long num, int *offset);
extern unsigned long_long get_long_long(char *p);
extern void free_sprites(int);

#ifdef HAVE_INLINE
	extern inline void 
#else
	extern void
#endif
get_dimensions(int type,int status,struct pos *s,int *w,int *h);

void update_position(
	struct it* obj,
	int new_x,
	int new_y,
	int width,
	int height,
	unsigned char *fx,
	unsigned char *fy
	);

extern void _skip_ws(char**txt);
extern int _convert_type(unsigned char c);
extern char * load_level(int);
extern void chdir_to_data_files(void);
#endif
