#ifndef __SERVER_H
#define __SERVER_H

#ifndef WIN32
	#include <netinet/in.h>
#else
	#define VERSION_PORT 8
#endif

#include "cfg.h"
#include "data.h"


/* item of player list */
struct player
{
	struct sockaddr_in address;
	unsigned char *name;
	unsigned char color;
	unsigned char health,armor;
	unsigned int frags;
	unsigned int deaths;
	unsigned short ammo[ARMS];
	unsigned char current_weapon;
	unsigned char weapons;  /* bitmask of player's weapons */
	unsigned long_long last_update;  /* last time client sent a packet */
	struct  /* keyboard status */
	{
	        unsigned char right,left,jump,creep,speed,fire,weapon,down_ladder;
	}keyboard_status;
	int id;
	struct it* obj;  /* pointer to player's hero */
	unsigned long invisibility_counter;
	unsigned char packet[MAX_PACKET_LENGTH];
	unsigned short packet_pos;
	signed int current_level;
};

#endif
