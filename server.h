#ifndef __SERVER_H
#define __SERVER_H

#ifndef WIN32
	#include <netinet/in.h>
#else
	#define VERSION_PORT 8
#endif

#include "cfg.h"
#include "data.h"

#define	KBD_RIGHT	(1 << 0)
#define	KBD_LEFT	(1 << 1)
#define	KBD_JUMP	(1 << 2)
#define	KBD_CREEP	(1 << 3)
#define	KBD_SPEED	(1 << 4)
#define	KBD_FIRE	(1 << 5)
#define	KBD_DOWN_LADDER	(1 << 6)
#define KBD_JETPACK	(1 << 7)

/* item of player list */
struct player
{
	struct sockaddr_in address;
	char *name;
	unsigned char color;
	unsigned char health,armor,health_ep;
	unsigned int frags;
	unsigned int deaths;
	unsigned short ammo[ARMS];
	unsigned short current_weapon;
	unsigned short weapons;  /* bitmask of player's weapons */
	unsigned long_long last_update;  /* last time client sent a packet */
	struct  /* keyboard status */
	{
	        unsigned char status, weapon;
	}keyboard_status;
	int id;
	struct it* obj;  /* pointer to player's hero */
	unsigned long invisibility_counter;
	unsigned char packet[MAX_PACKET_LENGTH];
	unsigned short packet_pos;
	signed int current_level;
};

#endif
