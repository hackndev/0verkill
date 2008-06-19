#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
	#include <sys/socket.h>
#else
	#include <winsock.h>
#endif

#include "net.h"
#include "crc32.h"
#include "error.h"

/*
PACKET:
	4 bytes    CRC32 (made only of data)
	4 bytes    sender
	4 bytes    recipient
	n bytes    data
*/


/* packet = data to transmit
 * len = length of data
 * addr = recipient's address
 * sender = sender's (my) ID 
 * recipient = recipient's ID
 */
void send_packet(unsigned char *packet,int len,const struct sockaddr* addr,int sender,int recipient)
{
	unsigned char *p;
	unsigned long crc=crc32(packet,len);
	p=mem_alloc(len+12);
	if (!p)return;  /* not enough memory */
	memcpy(p+12,packet,len);
	p[0]=crc&255;crc>>=8;  /* CRC 32 */
	p[1]=crc&255;crc>>=8;
	p[2]=crc&255;crc>>=8;
	p[3]=crc&255;
	
	p[4]=sender&255;sender>>=8;  /* sender */
	p[5]=sender&255;sender>>=8;
	p[6]=sender&255;sender>>=8;
	p[7]=sender&255;
	
	p[8]=recipient&255;recipient>>=8;  /* recipient */
	p[9]=recipient&255;recipient>>=8;
	p[10]=recipient&255;recipient>>=8;
	p[11]=recipient&255;
	
	sendto(fd,p,len+12,0,addr,sizeof(*addr));
	mem_free(p);
}


/* packet = buffer for received data
 * max_len = size of the buffer
 * addr = sender's address is filled in
 * addr_len = size of addr
 * sender_server = check
 * recipient = recipient's (my) ID
 * sender = if !NULL sender's ID
 */

/* returns:	-1 on error
		length of received data (data! not CRC nor sender's/recipient's ID 
 */
int recv_packet(unsigned char *packet,int max_len,struct sockaddr* addr,int *addr_len,int sender_server,int recipient,int *sender)
/* sender_server:
		1=check if sender is 0  (server)
		0=don't check sender's ID 
   recipient:	recipient's id
server has: sender_server 0, recipient 0
client has: sender_server 1, recipient my_id
*/
{
	unsigned char *p;
	int retval;
	unsigned long crc;
	int s,r;
	
	p=mem_alloc(max_len+12);
	if (!p)return -1;  /* not enough memory */
	retval=recvfrom(fd,p,max_len+12,0,addr,addr_len);
	memcpy(packet,p+12,max_len);
	crc=p[0]+(p[1]<<8)+(p[2]<<16)+(p[3]<<24);
	s=p[4]+(p[5]<<8)+(p[6]<<16)+(p[7]<<24);
	if (sender)*sender=s;
	r=p[8]+(p[9]<<8)+(p[10]<<16)+(p[11]<<24);
	mem_free(p);
	if (retval==-1)return -1;
	if (crc!=crc32(packet,retval-12))return -1;
	if (r!=recipient)return -1;
	if (sender_server&&s)return -1;
	return retval-12;
}

