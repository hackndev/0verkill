#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#include "net.h"
#include "data.h"
#include "cfg.h"
#include "getopt.h"
#include "error.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif


int port=DEFAULT_PORT;
struct sockaddr_in server;
char *name=0;
int fd;


/* find server address from name and put it into server structure */
void find_server(void)
{
        struct hostent *h;

        h=gethostbyname(name);
        if (!h){fprintf(stderr,"Error: Can't resolve server address.");exit(1);}

        server.sin_family=AF_INET;
        server.sin_port=htons(port);
        server.sin_addr=*((struct in_addr*)(h->h_addr_list[0]));
}


/* find a socket and and initialize it */
void init_socket(void)
{
        fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(fd<0){fprintf(stderr,"Can't get socket.\n");exit(1);}
}


/* contact server, send information request and wait for response */
void contact_server(void)
{
        static unsigned char packet[256];
        int a;

        fd_set fds;
        struct timeval tv;
        tv.tv_sec=1;
        tv.tv_usec=0;
        FD_ZERO(&fds);
        FD_SET(fd,&fds);

        packet[0]=P_INFO;

        send_packet(packet,1,(struct sockaddr*)(&server),0,0);

        if (!select(fd+1,&fds,NULL,NULL,&tv)){fprintf(stderr,"No reply within 1 second.\n");exit(1);}

        if (recv_packet(packet,256,0,0,1,0,0)<0)
        {
                if (errno==EINTR){fprintf(stderr,"Server hung up.\n");exit(1);}
                else {fprintf(stderr,"Connection refused.\n");exit(1);}
        }
	
	if ((*packet)!=P_INFO){fprintf(stderr,"Server error.\n");exit(1);}
	a=get_int(packet+1);
	printf("%d\n",a);
}



/* write help to stdout */
void print_help(void)
{
	printf(	"0verkill server testing program.\n"
		"(c)2000 Brainsoft\n"
		"Usage: test_server [-h] [-p <port number>] -a <address>\n");
}


void parse_command_line(int argc,char **argv)
{
        int a;
	char *c;

        while(1)
        {
                a=getopt(argc,argv,"hp:a:");
                switch(a)
                {
                        case EOF:
                        return;

                        case '?':
                        case ':':
                        exit(1);

                        case 'h':
                        print_help();
                        exit(0);

                        case 'a':
			name=mem_realloc(name,strlen(optarg)+1);
			memcpy(name,optarg,strlen(optarg)+1);
                        break;

			case 'p':
			port=strtoul(optarg,&c,10);
			if (*c){fprintf(stderr,"Error: Not a number.\n");exit(1);}
			if (errno==ERANGE)
			{
				if (!port){fprintf(stderr,"Error: Number underflow.\n");exit(1);}
				else {fprintf(stderr,"Error: Number overflow.\n");exit(1);}
			}
			break;
                }
        }
}


int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA wd;

	WSAStartup(0x101, &wd);
#endif

	parse_command_line(argc,argv);
	if (!name){print_help();exit(1);}

	init_socket();
	find_server();
	contact_server();
	return 0;
}
