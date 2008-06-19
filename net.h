#ifndef __NET_H
#define __NET_H


/* packet headers commented out are not used - they're obsolette, packets 
   haven't been renumbered only for some unsure backward compatibility */
/* obsolette packets were last used in version 0.6, they're unused since version 0.7 */


#define P_NEW_PLAYER 0
#define P_PLAYER_ACCEPTED 1
#define P_PLAYER_REFUSED 2
#define P_END 3
#define P_NEW_OBJ 4
#define P_UPDATE_STATUS 5
#define P_UPDATE_OBJECT 6
#define P_QUIT_REQUEST 7
#define P_DELETE_OBJECT 8
#define P_PLAYER_DELETED 9
/* #define P_WALK 10    *
 * #define P_JUMP 11    *
 * #define P_SHOOT 12   */
#define P_MESSAGE 13
#define P_UPDATE_PLAYER 14
#define P_HIT 15
/* #define P_CHANGE_WEAPON 16  */
/* number 17 is unused (historical reasons...) */
#define P_KEYBOARD 18
#define P_INFO 19
#define P_REENTER_GAME 20
#define P_CHUNK 21  /* chunk of packets */
#define P_UPDATE_OBJECT_POS 22
#define P_UPDATE_OBJECT_SPEED 23
#define P_UPDATE_OBJECT_COORDS 24
#define P_UPDATE_OBJECT_SPEED_STATUS 25
#define P_UPDATE_OBJECT_COORDS_STATUS 26
#define P_UPDATE_OBJECT_SPEED_STATUS_TTL 27
#define P_UPDATE_OBJECT_COORDS_STATUS_TTL 28
#define P_EXPLODE_GRENADE 29
#define P_BELL 30
#define P_CHANGE_LEVEL 31
#define P_LEVEL_ACCEPTED 32


/* socket filedescriptor */
extern int fd;

/* send packet with CRC and sender's and recipient's ID */
extern void send_packet(unsigned char *packet,int len,const struct sockaddr* addr,int sender,int recipient);
/* receive packet with CRC and sender's and recipient's ID */
extern int recv_packet(unsigned char *packet,int max_len,struct sockaddr* addr,int *addr_len,int sender,int recipient, int *s);

#endif
