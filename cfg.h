#ifndef __CONFIG_H
#define __CONFIG_H

#include "math.h"

/* #define LEAK_DEBUG */
/* #define LEAK_DEBUG_LIST  */


/* if you want to disable 3D support, comment following line out */
#define TRI_D  /* compile with 3D support */

#define VERSION_MAJOR 0
#define VERSION_MINOR 16

/* minimal version number accepted by the server */
#define MIN_CLIENT_VERSION_MINOR 16   /* server accepts client with this number */
#define MIN_SERVER_VERSION_MINOR 16   /* client can connect to server with this number */

/* all TTLs are in microseconds */

#define DEFAULT_FONT_NAME "-misc-fixed-bold-r-normal-*-13-*-*-*-c-*-iso8859-*"


#define DATA_PATH "data/"
#define LEVEL_FILE "level.dat"
#define GAME_SPRITES_FILE "sprites.dat"
#define BANNER_FILE "banner.dat"

#define STATIC_DATA_SUFFIX ".st"
#define DYNAMIC_DATA_SUFFIX ".dn"
#define LEVEL_SPRITES_SUFFIX ".sp"

#define CFG_FILE ".0verkill"

#define AREA_X 1000
#define AREA_Y 500

#define MAX_PLAYER_VIEW_X_RADIUS 40  /* unused */
#define MAX_PLAYER_VIEW_Y_RADIUS 15  /* unused */

#define MAX_DUMB_TIME 30000000   /* delete player after 30 dumb seconds */
#define MICROSECONDS 1000000   /* microseconds in one second */
#define FPS 32
#define CLIENT_FPS 32
#define PERIOD_USEC MICROSECONDS/FPS
#define CLIENT_PERIOD_USEC MICROSECONDS/CLIENT_FPS
#define MENU_PERIOD_USEC MICROSECONDS/20   /* 20Hz FPS */
#define DELAY_BEFORE_SLEEP_USEC 120000000  /* 120 seconds */


#define BULGE_WIDTH 20  /* dimensions of eyes bulging skull */
#define BULGE_HEIGHT 12

#define TITLE_WIDTH 72  /* dimensions of 0verkill title */
#define TITLE_HEIGHT 10

#define PLAYER_WIDTH 15
#define PLAYER_HEIGHT 13

#define CREEP_WIDTH 15  /* this isn't original width of image, because width of player must be still the same */
#define CREEP_HEIGHT 5

#define CREEP_YOFFSET (int2double(PLAYER_HEIGHT-CREEP_HEIGHT))

#define CORPSE_HEIGHT 3
#define CORPSE_WIDTH 34

/* All velocities are in pixels per second (not per tick) */
#define WALK_ACCEL (float2double(1*36))
#define MAX_SPEED_WALK_FAST (float2double(3*36))
#define MAX_SPEED_CREEP (float2double((double).5*36))
#define FALL_ACCEL (float2double((double).25*36*36/FPS))
#define MAX_X_SPEED (float2double(2*36))
#define MAX_Y_SPEED (float2double(5*36))
#define PLAYER_SLOW_DOWN_X (float2double(.9))
#define MIN_X_SPEED (float2double((double).2*36))
#define MIN_Y_SPEED (float2double((double).2*36))
#define SPEED_JUMP (float2double((double)2.5*36))
#define OVERKILL 15  /* if death dammage is greater than OVERKILL player is pulverized, otherwise corpse is created */
/* initial time to live of some objects */

#define N_SHRAPNELS_EXPLODE 40  /* # shrapnels per grenade explosion */

#define SHELL_TTL 160
#define SHRAPNEL_TTL 100
#define CORPSE_TTL 6000
#define MESS_TTL 1200

/* lethalness of killing objects */
#define KILL_LETHALNESS 5
#define KILL_ARMOR_DAMAGE 5

#define NOISE_TTL 30

#define HOLD_GUN_AFTER_SHOOT 8
#define MESS_HEIGHT 2
#define FIRE_YOFFSET (int2double(6))
#define GRENADE_FIRE_YOFFSET (int2double(2))
#define FIRE_IMPACT (float2double((double).3*36))  /* impact of bullet crashing into player */
#define SHRAPNEL_IMPACT (float2double((double)1.78*36))  /* impact of shrapnel crashing into player */
#define GRENADE_DELAY (weapon[WEAPON_GRENADE].cadence>>4)

#define MESSAGE_TTL 10000000  /* in microseconds */
#define MESSAGE_COLOR 11  /* yellow */
#define N_MESSAGES 4 /* maximal number of messages */
#define MAX_MESSAGE_LENGTH 70  /* maximal length of chat message */
#define MAX_PACKET_LENGTH 296-12  /* maximal packet length */

#define N_SHRAPNELS 10  /* number of shrapnel sprites */

#define MAX_NAME_LEN 24  /* maximal length of player's name */
#define MAX_HOST_LEN 64  /* maximal length of hostname */
#define MAX_PORT_LEN 8   /* maximal length of port number */

#define DEFAULT_PORT 6666   /* default game port */

#define MEDIKIT_HEALTH_ADD 30
#define ARMOR_ADD 50
#define INVISIBILITY_DURATION 2000


#define INVISIBILITY_RESPAWN_TIME 200000000   /* in microseconds */
#define ARMOR_RESPAWN_TIME 50000000  /* in microseconds */
#define MEDIKIT_RESPAWN_TIME 30000000  /* in microseconds */
#define WEAPON_RESPAWN_TIME 30000000   /* in microseconds */
#define AMMO_RESPAWN_TIME 25000000   /* in microseconds */

#define TOP_PLAYERS_N 10

/* when TRI_D is defined variable tri_d says plane to draw to */
/* 0 is normal plane, 1 is "extra" plane */
/* this variable works for all console functions and sprite functions */

#ifdef TRI_D
extern int TRI_D_ON;  /* view in 3D */
extern int tri_d;
#endif


#ifdef WIN32
	#include <windows.h>
	#define long_long       __int64
	#define bzero(a,b)      memset((a), 0, (b))
	#undef ERROR
	#ifdef SERVER
		extern int			consoleApp;
		#define ERROR(a)	{if (!console_ok)c_shutdown();if(consoleApp)fprintf(stderr,a);}
		#define EXIT(a)		{if (!consoleApp)ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0);exit(a);}
	#else
		#define ERROR(a)	{if (!console_ok)c_shutdown();fprintf(stderr,a);}
		#define EXIT(a)		{exit(a);}
	#endif
	#define random		rand
#else
	#define long_long	long long
	#ifdef CLIENT
		#define ERROR(a)	{if (!console_ok)c_shutdown();fprintf(stderr,a);}
	#else
		#define ERROR(a)	{fprintf(stderr,a);}
	#endif
	#define EXIT(a)		exit(a);
#endif


#include "console.h"

#endif
