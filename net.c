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
unsigned char *p;
int alloc_len = 0;
void send_packet(char *packet,int len,const struct sockaddr* addr,int sender,int recipient)
{
	unsigned long crc=crc32((unsigned char *)packet,len);
	if (!p) p=mem_alloc(len+12);
	else if (len > alloc_len) {
	    p=mem_realloc(p,len+12);
	    alloc_len = len;
	}
	if (!p)return;  /* not enough memory */
	memcpy(p+12,packet,len);
	p[0]=(char)(crc & 0xff);crc>>=8;  /* CRC 32 */
	p[1]=(char)(crc & 0xff);crc>>=8;
	p[2]=(char)(crc & 0xff);crc>>=8;
	p[3]=(char)(crc & 0xff);
	
	p[4]=(char)(sender & 0xff);sender>>=8;  /* sender */
	p[5]=(char)(sender & 0xff);sender>>=8;
	p[6]=(char)(sender & 0xff);sender>>=8;
	p[7]=(char)(sender & 0xff);
	
	p[8]=(char)(recipient & 0xff);recipient>>=8;  /* recipient */
	p[9]=(char)(recipient & 0xff);recipient>>=8;
	p[10]=(char)(recipient & 0xff);recipient>>=8;
	p[11]=(char)(recipient & 0xff);
	
	sendto(fd,p,len+12,0,addr,sizeof(*addr));
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
int recv_packet(char *packet,int max_len,struct sockaddr* addr,int *addr_len,int sender_server,int recipient,int *sender)
/* sender_server:
		1=check if sender is 0  (server)
		0=don't check sender's ID 
   recipient:	recipient's id
server has: sender_server 0, recipient 0
client has: sender_server 1, recipient my_id
*/
{
	int retval;
	unsigned int crc;
	int s,r;
	
	if (!p) p=mem_alloc(max_len+12);
	else if (max_len > alloc_len) {
	    p=mem_realloc(p,max_len+12);
	    alloc_len = max_len;
	}
	if (!p)return -1;  /* not enough memory */
	retval=recvfrom(fd,p,max_len+12,0,addr,(unsigned int *)addr_len);
	if (retval<12) {
		mem_free(p);
		return -1;
	}
	memcpy(packet,p+12,max_len);
	crc=p[0]+(p[1]<<8)+(p[2]<<16)+(p[3]<<24);
	s=p[4]+(p[5]<<8)+(p[6]<<16)+(p[7]<<24);
	if (sender)*sender=s;
	r=p[8]+(p[9]<<8)+(p[10]<<16)+(p[11]<<24);
	if (retval==-1)return -1;
	if (crc!=crc32((unsigned char *)packet,retval-12))return -1;
	if (r!=recipient)return -1;
	if (sender_server&&s)return -1;
	return retval-12;
}

/* free packet buffer, called only when terminating */
void free_packet_buffer() {
	if (p)
	    mem_free(p);
	return;
}
