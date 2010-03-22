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


static int port=DEFAULT_PORT;
static struct sockaddr_in server;
static char *name=NULL;
static int ttime=1;
static int verbose=0; /* whether to print a verbose message */
static int toplist=0; /* whether to print the list of top players */
int fd;


/* print info based on the recieved packet */
static void print_info (char * packet)
{
	int n_pl; /* number of players */

	n_pl=get_int(packet+1);
	if (verbose) {
		if (n_pl)
			printf ((n_pl>1)? "There are %d players on the server.\n"
					: "There is %d player on the server.\n", n_pl);
		else
			printf ("This server is not being used.\n");
	} else
		printf("%d\n",n_pl);
	if (toplist && n_pl) {
		unsigned int n_tp=*(packet+5); /* number of players on top list */

		if (!n_tp)
			printf("The top list is empty.\n");
		else {
			unsigned int i; /* iterator */
			/* pointer to the next set of player-specific info */
			char * p = packet+6;

			packet[256-1]='\0';
			printf ("Players on top list (%d):\n", n_tp);
			printf ("rank\tfrags\tdeaths\tcolor\tname\n");
			for (i=0; i<n_tp; i++) {
				printf ("#%d\t%d\t%d\t%d\t%s\n",
						i+1, get_int(p), get_int(p+4),
						*(p+8), p+9);
				p+=10+strlen(p+9);
			}
		}
	}
}


/* find server address from name and put it into server structure */
static void find_server(void)
{
        struct hostent *h;

        h=gethostbyname(name);
        if (!h){fprintf(stderr,"Error: Can't resolve server address.\n");exit(1);}

        server.sin_family=AF_INET;
        server.sin_port=htons(port);
        server.sin_addr=*((struct in_addr*)(h->h_addr_list[0]));
}


/* find a socket and and initialize it */
static void init_socket(void)
{
        fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(fd<0){fprintf(stderr,"Can't get socket.\n");exit(1);}
}


/* contact server, send information request and wait for response */
static int contact_server(void)
{
        static char packet[256];

        fd_set fds;
        struct timeval tv;
        tv.tv_sec=ttime;
        tv.tv_usec=0;
        FD_ZERO(&fds);
        FD_SET(fd,&fds);

        packet[0]=P_INFO;

        send_packet(packet,1,(struct sockaddr*)(&server),0,0);

        if (!select(fd+1,&fds,NULL,NULL,&tv)){fprintf(stderr,"No reply within %i second%s.\n",ttime,ttime==1?"":"s");return 1;}

        if (recv_packet(packet,256,0,0,1,0,0)<0)
        {
                if (errno==EINTR){fprintf(stderr,"Server hung up.\n");return 1;}
                else {fprintf(stderr,"Connection refused.\n");return 1;}
        }
	
	if ((*packet)!=P_INFO){fprintf(stderr,"Server error.\n");return 1;}
	print_info(packet);
	return 0;
}



/* write help to stdout */
static void print_help(void)
{
	printf(	"0verkill server testing program.\n"
		"(c)2000 Brainsoft\n"
		"Usage: test_server [-hvT] [-t <timeout>] [-p <port number>] -a <address>\n"
		"    -v  produce verbose output\n"
		"    -T  print the list of top players (implies -v)\n");
}


static int parse_number (const char * arg)
{
	char *c; /* used by strtoul */
	int n;

	n=strtoul(arg,&c,10);
	if (*c){fprintf(stderr,"Error: Not a number.\n");exit(1);}
	if (errno==ERANGE)
	{
		if (!port){fprintf(stderr,"Error: Number underflow.\n");exit(1);}
		else {fprintf(stderr,"Error: Number overflow.\n");exit(1);}
	}

	return n;
}


static void parse_command_line(int argc,char **argv)
{
        int a;

        while((a=getopt(argc,argv,"hvTp:a:t:")) != -1)
        {
                switch(a)
                {
			case 'T':
			toplist = 1;
			case 'v':
			verbose = 1;
			break;

                        case 'a':
			name=optarg;
                    	break;

			case 'p':
			port=parse_number(optarg);
			break;

			case 't':
			ttime=parse_number(optarg);
			if(ttime<1){fprintf(stderr,"Error: Timeout too low.\n");exit(1);}
			break;

                        case '?':
                        case ':':
                        case 'h':
                        print_help();
                        exit(0);
                }
        }
}


int main(int argc, char **argv)
{
	int ret;
#ifdef WIN32
	WSADATA wd;

	WSAStartup(0x101, &wd);
#endif

	parse_command_line(argc,argv);
	if (!name){print_help();exit(1);}

	init_socket();
	find_server();
	ret=contact_server();
	free_packet_buffer();
	if(fd) close(fd);
	return ret;
}
