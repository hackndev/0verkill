#ifndef WIN32
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#if (!defined(WIN32))
	#ifdef TIME_WITH_SYS_TIME
		#include <sys/time.h>
		#include <time.h>
	#else
		#ifdef TM_IN_SYS_TIME
			#include <sys/time.h>
		#else
			#include <time.h>
		#endif
	#endif
	#include <sys/socket.h>
#else
	#include <time.h>
	#include <winsock.h>
#endif

#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifndef __USE_GNU
	#define __USE_GNU
#endif
#include <string.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef __EMX__
#define INCL_DOS
#include <os2.h>
#endif

#include "data.h"
#include "server.h"
#include "net.h"
#include "cfg.h"
#include "hash.h"
#include "time.h"
#include "getopt.h"
#include "math.h"
#include "error.h"


unsigned short port=DEFAULT_PORT;  /* game port (not a gameport ;-) )*/
int level_number=0;   /* line number in the LEVEL_FILE */
int fd;   /* socket */
int n_players;   /* highest ID of player */
int active_players=0;  /* # of players in the game */
unsigned int id=0;  /* my ID */
unsigned long_long game_start; /* time of game start */
/* important sprites */
int grenade_sprite,bullet_sprite,slug_sprite,shell_sprite,shotgun_shell_sprite,mess1_sprite,mess2_sprite,mess3_sprite,mess4_sprite,noise_sprite;
int shrapnel_sprite[N_SHRAPNELS];
int nonquitable=0;  /* 1=clients can't abort game pressing F12 (request is ignored) */
unsigned long_long last_tick;
unsigned long_long last_player_left=0,last_packet_came=0;

unsigned char *level_checksum=0;   /* MD5 sum of the level */

unsigned char weapons_order[ARMS]={4,0,3,1,2};

struct birthplace_type
{
	int x,y;
}*birthplace=DUMMY;

int n_birthplaces=0;

/* list of players */
struct player_list
{
        struct player_list *next,*prev;
        struct player member;
}players;


/* time queue of respawning objects */
struct queue_list
{
	struct queue_list* next;
	unsigned long_long time;
	struct it member;
}time_queue;


/* list of objects */
struct object_list objects;

struct player_list *last_player;
struct object_list *last_obj;


#ifdef WIN32
	#define random rand
	#define srandom srand

	#define	SERVICE_NAME			"0verkill_015"
	#define	SERVICE_DISP_NAME		"0verkill Server 0.15"
	int		consoleApp=0;	/* 0 kdyz to bezi jako server na pozadi */

/***************************************************/
/* WINDOWS NT SERVICE INSTALLATION/RUNNING/REMOVAL */

#include <winsvc.h>

int server(void);

SERVICE_STATUS			ssStatus;
SERVICE_STATUS_HANDLE	sshStatusHandle;
DWORD					globErr=ERROR_SUCCESS;
TCHAR					szErr[2048];
int						hServerExitEvent=0;	/* close server flag */

LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize) {
	DWORD	dwRet;
	LPTSTR	lpszTemp=NULL;
	globErr=GetLastError();
	dwRet=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL,
		globErr, LANG_NEUTRAL, (LPTSTR)&lpszTemp, 0, NULL);
	/* supplied buffer is not long enough */
	if (( !dwRet )||( (long)dwSize < (long)dwRet+14 ))
		sprintf(lpszBuf, ": %u", globErr);
	else {
		lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');
		CharToOem(lpszTemp, lpszTemp);
		sprintf(lpszBuf, ": %u: %s", globErr, lpszTemp);
	}
	if ( lpszTemp )
		LocalFree((HLOCAL)lpszTemp);
	return lpszBuf;
}

void CmdInstallService(LPCTSTR pszUserName, LPCTSTR pszUserPassword, LPCTSTR pszDependencies) {
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	TCHAR		szPath[2048];
	printf("Installing %s as ", SERVICE_NAME);
	if ( pszUserName==NULL )
		printf("LOCAL_SYSTEM");
	else {
		printf("\"%s\"", pszUserName);
		if ( pszUserPassword )
			printf("/\"%s\"", pszUserPassword);
	};
	printf("...\r\n");
	if ( GetModuleFileName(NULL, szPath, 2046)==0 ) {
		printf("Unable to install %s%s\r\n", SERVICE_NAME, GetLastErrorText(szErr, 2046));
		return;
	};
	schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if ( schSCManager ) {
		schService=CreateService(
			schSCManager,               /* SCManager database */
			SERVICE_NAME,               /* name of service */
			SERVICE_DISP_NAME,          /* name to display */
			SERVICE_ALL_ACCESS,         /* desired access */
			SERVICE_WIN32_OWN_PROCESS,  /* service type */
			SERVICE_DEMAND_START,       /* start type */
			SERVICE_ERROR_NORMAL,       /* error control type */
			szPath,                     /* service's binary */
			NULL,                       /* no load ordering group */
			NULL,                       /* no tag identifier */
			pszDependencies,            /* dependencies */
			pszUserName,                /* account */
			pszUserPassword);           /* password */
		if ( schService ) {
			printf("%s was installed successfully.\r\n", SERVICE_NAME);
			globErr=ERROR_SUCCESS;
			CloseServiceHandle(schService);
		}
		else
			printf("CreateService failed%s\r\n", GetLastErrorText(szErr, 2046));
		CloseServiceHandle(schSCManager);
	}
	else
		printf("OpenSCManager failed%s\r\n", GetLastErrorText(szErr, 2046));
}

void CmdRemoveService() {
	SC_HANDLE	schService,
				schSCManager;
	schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if ( schSCManager ) {
		schService=OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
		if ( schService ) {
			if ( ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus) ) {
				printf("Stopping %s...\r\n", SERVICE_NAME);
				Sleep(500);
				while( QueryServiceStatus(schService, &ssStatus) ) {
					if ( ssStatus.dwCurrentState==SERVICE_STOP_PENDING ) {
						printf(".");
						Sleep(500);
					}
					else
						break;
				};
				if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
					printf("\r\n%s stopped successfully.\r\n", SERVICE_NAME);
				else
					printf("\r\n%s failed to stop.\r\n", SERVICE_NAME);
			}
			else
				printf("ControlService failed%s\r\n", GetLastErrorText(szErr, 2046));
			if ( DeleteService(schService) ) {
				globErr=ERROR_SUCCESS;
				printf("%s removed.\r\n", SERVICE_NAME);
			}
			else
				printf("DeleteService failed%s\r\n", GetLastErrorText(szErr, 2046));
			CloseServiceHandle(schService);
		}
		else
			printf("OpenService failed%s\r\n", GetLastErrorText(szErr, 2046));
		CloseServiceHandle(schSCManager);
	}
	else
		printf("OpenSCManager failed%s\r\n", GetLastErrorText(szErr, 2046));
}

void AddToMessageLog(LPTSTR lpszMsg, int infoOnly) {
	TCHAR	szMsg[2048];
	HANDLE	hEventSource;
	LPTSTR	lpszStrings[2];

	hEventSource=RegisterEventSource(NULL, SERVICE_NAME);

	sprintf(szMsg, infoOnly ? "%s notification:" : "%s error: %u", SERVICE_NAME, globErr);
	lpszStrings[0]=szMsg;
	lpszStrings[1]=lpszMsg;

	if ( hEventSource!=INVALID_HANDLE_VALUE ) {
		ReportEvent(hEventSource, infoOnly ? EVENTLOG_INFORMATION_TYPE : EVENTLOG_ERROR_TYPE, 0, 0, NULL, 2, 0, (LPCTSTR*)lpszStrings, NULL);
		DeregisterEventSource(hEventSource);
	};
};

BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
	static DWORD	dwCheckPoint=1;
    BOOL			fResult=TRUE;

	if ( dwCurrentState==SERVICE_START_PENDING )
		ssStatus.dwControlsAccepted=0;
	else
		ssStatus.dwControlsAccepted=SERVICE_ACCEPT_STOP;

	ssStatus.dwCurrentState=dwCurrentState;
	ssStatus.dwWin32ExitCode=dwWin32ExitCode;
	ssStatus.dwWaitHint=dwWaitHint;

	if (( dwCurrentState==SERVICE_RUNNING )||
		( dwCurrentState==SERVICE_STOPPED ))
		ssStatus.dwCheckPoint=0;
	else
		ssStatus.dwCheckPoint=dwCheckPoint++;
	if ( !(fResult=SetServiceStatus(sshStatusHandle, &ssStatus)) ) {
		globErr=GetLastError();
		AddToMessageLog("SetServiceStatus failed", 0);
	}
	return fResult;
}

void ServiceStop() {
	hServerExitEvent=1;
	raise(SIGINT);
}

void WINAPI ServiceCtrl(DWORD dwCtrlCode) {
	switch( dwCtrlCode ) {
	case SERVICE_CONTROL_STOP:
		ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 500);
		ServiceStop();
		return;
	};
	if ( hServerExitEvent )
		ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
	else
		ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0);		
}


void ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv) {
	if ( dwArgc>1 )
		SetCurrentDirectory(lpszArgv[1]);
	else {
		char	path[1024],
				*p2;
		GetModuleFileName(NULL, path, sizeof(path));
		p2=path+strlen(path);
		while(( p2>=path )&&( *p2!='\\' )&&( *p2!='/' ))
			p2--;
		*p2=0;
		SetCurrentDirectory(path);
	}
	server();
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) {
	if ( !(sshStatusHandle=RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrl)) )
		return;

	ssStatus.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
	ssStatus.dwServiceSpecificExitCode=0;
	if ( !ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
		goto Cleanup;

	nonquitable=1;
  	ServiceStart(dwArgc, lpszArgv);

Cleanup:
	ReportStatusToSCMgr(SERVICE_STOPPED, globErr, 0);
	return;
};
/* WINDOWS NT SERVICE INSTALLATION/REMOVAL */
/*******************************************/
#endif


/* load dynamic data */
void load_dynamic(unsigned char * filename)
{
	FILE * stream;
	static unsigned char line[1024];
	char *p,*q,*name;
	int n,x,y,t;

	if (!(stream=fopen(filename,"rb"))){unsigned char msg[256];snprintf(msg,256,"Can't open file \"%s\"!\n",filename);ERROR(msg);EXIT(1);}
	while(fgets(line,1024,stream))
	{
		p=line;
		_skip_ws(&p);
		for (name=p;(*p)!=' '&&(*p)!=9&&(*p)!=10&&(*p);p++);
		if (!(*p))continue;
		*p=0;p++;
		_skip_ws(&p);
		if ((t=_convert_type(*p))<0){unsigned char msg[256];snprintf(msg,256,"Unknown object type '%c'.\n",*p);ERROR(msg);EXIT(1);}
		p++;
		_skip_ws(&p);
		x=strtol(p,&q,0);
		_skip_ws(&q);
		y=strtol(q,&p,0);
		if (t==TYPE_BIRTHPLACE)   /* birthplace */
		{
			n_birthplaces++;
			birthplace=mem_realloc(birthplace,sizeof(struct birthplace_type)*n_birthplaces);
			if (!birthplace){ERROR("Error: Not enough memory.\n");EXIT(1);}
			birthplace[n_birthplaces-1].x=x;
			birthplace[n_birthplaces-1].y=y;
			continue;
		}
		if (find_sprite(name,&n)){unsigned char msg[256];snprintf(msg,256,"Unknown bitmap name \"%s\"!\n",name);ERROR(msg);EXIT(1);}
		new_obj(id,t,0,n,0,0,int2double(x),int2double(y),0,0,0);
		id++;
	}
	fclose(stream);
	if (!n_birthplaces){ERROR("Error: No birthplaces.\n");EXIT(1);}
}


void print_ip(unsigned char * txt,struct in_addr ip)
{
	unsigned int a;

	sprintf(txt,"%d",*((unsigned char *)&ip));
	for (a=1;a<sizeof(ip);a++)
		sprintf(txt+strlen(txt),".%d",((unsigned char *)&ip)[a]);
}


/* write a message to stdout or stderr */
void message(unsigned char *msg,int output)
{
	time_t t;
	struct tm tm;
	static unsigned char timestamp[64];

#ifdef WIN32
	if ( !consoleApp )
		return;
#endif
	
	t=time(0);
	tm=*(localtime(&t));
	
	/* time stamp is in format day.month.year hour:minute:second */
	snprintf(timestamp,64,"%2d.%2d.%d %02d:%02d:%02d  ",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
	switch (output)
	{
		case 1:
		printf("%s%s",timestamp,msg);
		fflush(stdout);
		break;

		case 2:
		fprintf(stderr,"%s%s",timestamp,msg);
		fflush(stderr);
		break;
	}
}


/* switch weapon to the player's best one */
int select_best_weapon(struct player* p)
{
	unsigned char t[ARMS]={4,0,1,3,2}; 
	int a;

	for (a=ARMS-1;a>=0;a--)
		if ((p->weapons&(1<<t[a]))&&p->ammo[t[a]])return t[a];
		
	return p->current_weapon;
}


/* Bakta modify - better weapon selection routines */
/* test if weapon is better than the one he wields */
int is_weapon_better(struct player* p, int weapon)
{
	int i;
	int cur=0, test=0;

	for(i=ARMS-1;i>=0;i--)
	{
		if(weapons_order[i]==p->current_weapon)
			cur = i;
		if(weapons_order[i]==weapon)
			test = i;
	}
	return cur<test;
}


/* test if player p has weapon and can use it (has ammo for it) */
int is_weapon_usable(struct player* p, int weapon)
{
	return !!((p->weapons&(1<<weapon))&&p->ammo[weapon]);
}


/* test if player p has weapon and has no ammo for it */
int is_weapon_empty(struct player* p, int weapon)
{
	return !!((p->weapons&(1<<weapon))&&!p->ammo[weapon]);
}


/* initialize socket */
void init_socket(void)
{
	struct sockaddr_in server;
	fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (fd<0)
	{
		ERROR("Error: Can't create socket!\n");
		EXIT(1);
	}

	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	server.sin_addr.s_addr=INADDR_ANY;

	if (bind(fd,(struct sockaddr*)&server,sizeof(server)))
	{
		unsigned char msg[256];
		snprintf(msg,256,"Error: Can't bind socket to port %d!\n",port);
		ERROR(msg);
		EXIT(1);
	}
}


/* selects random birthplace and returns */
void find_birthplace(int *x,int *y)
{
	int a=random()%n_birthplaces;
	
	*x=birthplace[a].x;
	*y=birthplace[a].y;
}


/* return index of hero with color num in sprites field */
/* if there's not such color, returns -1 */
int select_hero(int num)
{
	static unsigned char txt[32];
	int a;

	sprintf(txt,"hero%d",num);
	if (find_sprite(txt,&a))return -1;
	return a;
}


/* initialize player */
void init_player(struct player* p,my_double x,my_double y)
{
	int a;
	
	p->health=100;
	p->armor=0;
	p->current_weapon=0;
	p->weapons=17; /* he has only gun and grenades */
	p->invisibility_counter=0;
	for (a=0;a<ARMS;a++)
		p->ammo[a]=0;
	p->ammo[0]=weapon[0].basic_ammo;
	p->obj->xspeed=0;
	p->obj->yspeed=0;
	p->obj->status=0;
	p->obj->ttl=0;
	p->obj->anim_pos=0;
	p->obj->x=x;
	p->obj->y=y;
}


/* completely add player to server's database */
/* player starts on x,y coordinates */
int add_player(unsigned char color, unsigned char *name,struct sockaddr_in *address,int x, int y)
{
	int h;
#define cp last_player->next

	cp=last_player->next;

	/* alloc memory for new player */
	cp=mem_alloc(sizeof(struct player_list));
	if (!cp)
		{message("Not enough memory.\n",2);return 1;}
	cp->member.name=mem_alloc(strlen(name)+1);
	if (!cp->member.name)
		{mem_free(cp);message("Not enough memory.\n",2);return 1;}
	
	/* fill in the structure */
	cp->member.color=color;
	cp->member.frags=0;
	cp->member.deaths=0;
	cp->member.keyboard_status.left=0;
	cp->member.keyboard_status.right=0;
	cp->member.keyboard_status.speed=0;
	cp->member.keyboard_status.fire=0;
	cp->member.keyboard_status.creep=0;
	cp->member.keyboard_status.down_ladder=0;
	cp->member.keyboard_status.weapon=0;
	cp->member.id=n_players+1;  /* player numbering starts from 1, 0 is server's ID */
	cp->member.last_update=get_time();
	memcpy(cp->member.name,name,strlen(name)+1);
	cp->member.address=*address;
	cp->member.packet_pos=0;
	cp->member.current_level=-1;
	/* create new object "player" */
	h=select_hero(cp->member.color);
	if (h<0)
		{mem_free(cp->member.name);mem_free(cp);message ("No such color.\n",1);return 1;}
	cp->member.obj=new_obj(id,T_PLAYER,0,h,0,0,int2double(x),int2double(y),0,0,(&(cp->member)));
	if (!cp->member.obj)
		{mem_free(cp->member.name);mem_free(cp);message("Can't create object.\n",1);return 1;}
	id++;
	init_player(&(cp->member),int2double(x),int2double(y));

	/* that's all */
	cp->prev=last_player;
	cp->next=0;
	last_player=cp;
#undef cp 
	return 0;
}


void send_chunk_to_player(struct player *player)
{
	static unsigned char p[MAX_PACKET_LENGTH];

	if (!(player->packet_pos))return;
	if (player->packet_pos>MAX_PACKET_LENGTH-1)return;
	p[0]=P_CHUNK;
	memcpy(p+1,player->packet,player->packet_pos);
	send_packet(p,(player->packet_pos)+1,(struct sockaddr*)(&(player->address)),0,player->id);
	player->packet_pos=0;
}


void send_chunk_packet_to_player(unsigned char *packet, int len, struct player *player)
{
	if (len>MAX_PACKET_LENGTH-1)return;
	if ((player->packet_pos)+len>MAX_PACKET_LENGTH-1)send_chunk_to_player(player);
	memcpy((player->packet)+(player->packet_pos),packet,len);
	player->packet_pos+=len;
}


void send_chunks(void)
{
	struct player_list* p;

	for (p=&players;p->next;p=p->next)
 		send_chunk_to_player(&(p->next->member));
}


/* send a packet to all players except one */
/* if not_this_player is null, sends to all players */
void sendall(unsigned char *packet,int len, struct player * not_this_player)
{
	struct player_list* p;

	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
 			send_packet(packet,len,(struct sockaddr*)(&(p->next->member.address)),0,p->next->member.id);
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_packet(packet,len,(struct sockaddr*)(&(p->next->member.address)),0,p->next->member.id);
}


/* send a packet to all players except one */
/* if not_this_player is null, sends to all players */
void sendall_chunked(unsigned char *packet,int len, struct player * not_this_player)
{
	struct player_list* p;

	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
 			send_chunk_packet_to_player(packet,len,&(p->next->member));
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_chunk_packet_to_player(packet,len,&(p->next->member));
}


int which_update(struct it *obj,my_double old_x,my_double old_y,my_double old_x_speed,my_double old_y_speed,unsigned int old_status,int old_ttl)
{
	int a=0;
	
	if (old_ttl==obj->ttl)
	{
		if (old_x_speed==obj->xspeed&&old_y_speed==obj->yspeed)a=5;  /* update pos and status */
		if (old_x==obj->x&&old_y==obj->y)a=4; /* update speed and status */
	}

	if (old_status==obj->status&&old_ttl==obj->ttl)
	{
		a=1;  /* update speed+coords */
		
		if (old_x_speed==obj->xspeed&&old_y_speed==obj->yspeed)a=3; /* update pos */
		if (old_x==obj->x&&old_y==obj->y)
		{
			if(a==3)return -1;  /* nothing to update */
			a=2;  /* update speed */
		}
	}
	return a;
}

/* send a packet to all players except one */
/* if not_this_player is null, sends to all players */
/* type is: 
		0=full update
		1=update speed+coordinates
		2=update speed
		3=update coordinates 
		4=update speed + status
		5=update coordinates + status
		6=update speed + status + ttl
		7=update coordinates + status + ttl
*/
void sendall_update_object(struct it* obj, struct player * not_this_player,int type)
{
	struct player_list* p;
	int l;
	static unsigned char packet[32];

	switch (type)
	{
		case 0: /* all */
		packet[0]=P_UPDATE_OBJECT;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->x);
		put_float(packet+10,obj->y);
		put_float(packet+14,obj->xspeed);
		put_float(packet+18,obj->yspeed);
		put_int16(packet+22,obj->status);
		put_int16(packet+24,obj->ttl);
		l=26;
		break;

		case 1: /* coordinates and speed */
		packet[0]=P_UPDATE_OBJECT_POS;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->x);
		put_float(packet+10,obj->y);
		put_float(packet+14,obj->xspeed);
		put_float(packet+18,obj->yspeed);
		l=22;
		break;
		
		case 2: /* speed */
		packet[0]=P_UPDATE_OBJECT_SPEED;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->xspeed);
		put_float(packet+10,obj->yspeed);
		l=14;
		break;
		
		case 3: /* coordinates */
		packet[0]=P_UPDATE_OBJECT_COORDS;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->x);
		put_float(packet+10,obj->y);
		l=14;
		break;
		
		case 4: /* speed and status */
		packet[0]=P_UPDATE_OBJECT_SPEED_STATUS;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->xspeed);
		put_float(packet+10,obj->yspeed);
		put_int16(packet+14,obj->status);
		l=16;
		break;
		
		case 5: /* coordinates and status */
		packet[0]=P_UPDATE_OBJECT_COORDS_STATUS;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->x);
		put_float(packet+10,obj->y);
		put_int16(packet+14,obj->status);
		l=16;
		break;
		
		case 6: /* speed and status and ttl */
		packet[0]=P_UPDATE_OBJECT_SPEED_STATUS_TTL;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->xspeed);
		put_float(packet+10,obj->yspeed);
		put_int16(packet+14,obj->status);
		put_int16(packet+16,obj->ttl);
		l=18;
		break;
		
		case 7: /* coordinates and status and ttl */
		packet[0]=P_UPDATE_OBJECT_COORDS_STATUS_TTL;
		put_int(packet+1,obj->id);
		packet[5]=obj->update_counter;
		put_float(packet+6,obj->x);
		put_float(packet+10,obj->y);
		put_int16(packet+14,obj->status);
		put_int16(packet+16,obj->ttl);
		l=18;
		break;

		default: /* don't update */
		return;
		
	}

	obj->update_counter++;
	
	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
			send_chunk_packet_to_player(packet,l,&(p->next->member));
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_chunk_packet_to_player(packet,l,&(p->next->member));
}


/* send update status packet to all players (except not_this_player if this is !NULL)*/
void sendall_update_status(struct it* obj, struct player * not_this_player)
{
	struct player_list* p;
	static unsigned char packet[8];

	packet[0]=P_UPDATE_STATUS;
	put_int(packet+1,obj->id);
	put_int16(packet+5,obj->status);
	
	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
 			send_chunk_packet_to_player(packet,7,&(p->next->member));
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_chunk_packet_to_player(packet,7,&(p->next->member));
}


/* send hit packet to all players except not_this player (if !NULL)*/
void sendall_hit(unsigned long id,unsigned char direction,unsigned char xoffs,unsigned char yoffs,struct player* not_this_player)
{
	struct player_list* p;
	static unsigned char packet[8];

	packet[0]=P_HIT;
	put_int(packet+1,id);
	packet[5]=direction;
	packet[6]=xoffs;
	packet[7]=yoffs;
	
	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
 			send_chunk_packet_to_player(packet,8,&(p->next->member));
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_chunk_packet_to_player(packet,8,&(p->next->member));
}


/* sort players according to frags */
int _qsort_cmp(const void *a,const void *b)
{
	int fa,fb,da,db;
	fa=(*((struct player**)a))->frags;
	fb=(*((struct player**)b))->frags;
	da=(*((struct player**)a))->deaths;
	db=(*((struct player**)b))->deaths;

	if (fa>fb)return -1;
	if (fa==fb)
	{
		if (da<db)return -1;
		if (da>db)return 1;
		return 0;
	}
	return 1;
}


/* send info how many players are in the game and top score */
/* id=recipient's id */
/* if addr is NULL info is sent to all */
void send_info(struct sockaddr *addr,int id)
{
	unsigned char *packet;
	int p=active_players>TOP_PLAYERS_N?TOP_PLAYERS_N:active_players;
	int a,n,l;
	struct player **t;  /* table for qsort */
	struct player_list *q;

	/* alloc memory */
	t=mem_alloc(active_players*sizeof(struct player*));
	if (!t)return;
	packet=mem_alloc(1+4+1+p*(MAX_NAME_LEN+4+4));
	if (!packet)return;
	
	/* fill table for quicksort */
	for (a=0,q=(&players);q->next;a++,q=q->next)
		t[a]=&(q->next->member);
	
	/* sort players */
	qsort(t,active_players,sizeof(struct player*),_qsort_cmp);
	
	/* create packet header */
	packet[0]=P_INFO;
	put_int(packet+1,active_players);
	packet[5]=p;
	n=6;

	/* put players into packet */
	for (a=0;a<p;a++)
	{
		put_int(packet+n,(t[a])->frags);
		put_int(packet+n+4,(t[a])->deaths);
		packet[n+8]=t[a]->color;
		l=strlen((t[a])->name)+1;
		memcpy(packet+n+9,(t[a])->name,l);
		n+=l+9;
	}
	if (!addr)sendall(packet,n,0);
	else send_packet(packet,n,addr,0,id);

	mem_free(packet);
	mem_free(t);
}

/* send message to a player */
/* name is name of player who sent the message (chat), NULL means it's from server */
void send_message(struct player* player,unsigned char *name,unsigned char *msg)
{
	static unsigned char packet[256];
	int len;

	packet[0]=P_MESSAGE;
	if (!name){snprintf(packet+1,256,"%s",msg);len=strlen(msg)+1+1;}
	else {snprintf(packet+1,256,"%s> %s",name,msg);len=strlen(name)+strlen(msg)+1+3;}
	send_chunk_packet_to_player(packet,len,player);
}


/* similar to send_message but sends to all except one or two players */
void sendall_message(unsigned char *name,unsigned char *msg,struct player *not1,struct player* not2)
{
	static unsigned char packet[256];
	int len;
	struct player_list* p;

	packet[0]=P_MESSAGE;
	if (!name){snprintf(packet+1,255,"%s",msg);len=strlen(msg)+1+1;}
	else {snprintf(packet+1,255,"%s> %s",name,msg);len=strlen(name)+strlen(msg)+1+3;}
	for (p=&players;p->next;p=p->next)
		if ((!not1||(&(p->next->member))!=not1)&&(!not2||(&(p->next->member))!=not2))
 			send_chunk_packet_to_player(packet,len,&(p->next->member));
}


void sendall_bell(void)
{
	static unsigned char packet;
	struct player_list* p;

	packet=P_BELL;
	for (p=&players;p->next;p=p->next)
 		send_chunk_packet_to_player(&packet,1,&(p->next->member));

}


/* send new object information to given address */
void send_new_obj(struct sockaddr* address, struct it* obj,int id)
{
	static unsigned char packet[32];

	packet[0]=P_NEW_OBJ;
	put_int(packet+1,obj->id);
	put_int16(packet+5,obj->sprite);
	put_float(packet+7,obj->x);
	put_float(packet+11,obj->y);
	put_float(packet+15,obj->xspeed);
	put_float(packet+19,obj->yspeed);
	put_int16(packet+23,obj->status);
	packet[25]=obj->type;
	put_int16(packet+26,obj->ttl);
	send_packet(packet,28,address,0,id);
}


/* send new object information to given address */
void send_new_obj_chunked(struct player* player, struct it* obj)
{
	static unsigned char packet[32];

	packet[0]=P_NEW_OBJ;
	put_int(packet+1,obj->id);
	put_int16(packet+5,obj->sprite);
	put_float(packet+7,obj->x);
	put_float(packet+11,obj->y);
	put_float(packet+15,obj->xspeed);
	put_float(packet+19,obj->yspeed);
	put_int16(packet+23,obj->status);
	packet[25]=obj->type;
	put_int16(packet+26,obj->ttl);
	send_chunk_packet_to_player(packet,28,player);
}


/* send player update to given player */
void send_update_player(struct player* p)
{
	static unsigned char packet[32];
	int a;

	packet[0]=P_UPDATE_PLAYER;
	packet[1]=p->health;
	packet[2]=p->armor;
	for (a=0;a<ARMS;a++)
		put_int16(packet+3+(a<<1),p->ammo[a]);
	put_int(packet+3+2*ARMS,p->frags);
	put_int(packet+7+2*ARMS,p->deaths);
	packet[11+2*ARMS]=p->current_weapon;
	packet[12+2*ARMS]=p->weapons;
	send_chunk_packet_to_player(packet,13+2*ARMS,p);
}

/* send a packet to all players except one */
/* if not_this_player is null, sends to all players */
void sendall_new_object(struct it* obj, struct player * not_this_player)
{
	struct player_list* p;

	if (!not_this_player)
		for (p=&players;p->next;p=p->next)
 			send_new_obj_chunked(&(p->next->member),obj);
	else
		for (p=&players;p->next;p=p->next)
			if ((&(p->next->member))!=not_this_player)
 				send_new_obj_chunked(&(p->next->member),obj);
}



/* send all objects except one to specified player */
/* if not_this_object is null, sends all objects */
void send_objects(struct player* player,struct it* not_this_object)
{
	struct object_list *p;

	if (!not_this_object)
		for(p=&objects;p->next;p=p->next)
			send_new_obj_chunked(player,&(p->next->member));
	else
		for(p=&objects;p->next;p=p->next)
			if((&p->next->member)!=not_this_object)
				send_new_obj_chunked(player,&(p->next->member));
}


/* send all objects except one to specified player */
/* if not_this_object is null, sends all objects */
void send_change_level(struct player* player)
{
	unsigned char packet[64];

	packet[0]=P_CHANGE_LEVEL;
	put_int(packet+1,level_number);
	memcpy(packet+5,level_checksum,33);
	send_packet(packet,38,(struct sockaddr*)(&(player->address)),0,player->id);
}


/* send all objects in time queue to given player */
void send_timeq_objects(struct player *player)
{
	struct queue_list *p;
	
	for (p=&time_queue;p->next;p=p->next)
		send_new_obj_chunked(player,&(p->next->member));
}


/* remove player from server's list */
/* delete player's hero and send message */
void delete_player(struct player_list *q)
{
	unsigned char packet[5];
	
	/* delete object player's hero */
	packet[0]=P_DELETE_OBJECT;
	put_int(packet+1,q->member.obj->id);
	sendall_chunked(packet,5,&(q->member));
	delete_obj(q->member.obj->id);

	mem_free (q->member.name);
	q->prev->next=q->next;
	if (!q->next)last_player=q->prev;   /* we're deleting last player of the list */
	else q->next->prev=q->prev;
	mem_free(q);
	active_players--;
	if (!active_players)last_player_left=get_time();
}


/* create a new object and fill with data and adds to hash table */
struct it * add_to_timeq(
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
        void * data,unsigned long_long t)
{
	struct queue_list *p;
	struct queue_list *q;

	t+=get_time();
	for (p=&time_queue;p->next&&p->next->member.last_updated<t;p=p->next);
	
	q=p->next;

        p->next=mem_alloc(sizeof(struct queue_list));
        if (!p->next){p->next=q;return 0;}
        p=p->next;
        p->next=q;
        p->member.x=x;
        p->member.y=y;
        p->member.xspeed=xspeed;
        p->member.yspeed=yspeed;
        p->member.type=type;
        p->member.ttl=ttl;
	p->member.id=id;
        p->member.sprite=sprite;
        p->member.anim_pos=pos;
        p->member.data=data;
        p->member.status=status|64;
	p->member.last_updated=t;
        return &(p->member);
}


/* update time queue (respawning objects) */
/* add objects timed out to the game */
void update_timeq(void)
{
	unsigned long_long t=get_time();
	struct queue_list *p,*q;
	struct it *o;
	
#define N p->next->member
	for (p=&time_queue;p->next&&p->next->member.last_updated<=t;)
	{
		N.status&=~64;
		o=new_obj(N.id,N.type,N.ttl,N.sprite,N.anim_pos,N.status,N.x,N.y,N.xspeed,N.yspeed,N.data);
		sendall_update_status(o,0);
		q=p->next->next;
		mem_free(p->next);
		p->next=q;
	}
#undef N
}


/* find player with given address */
/* on unsuccess return 0 */
struct player_list* find_player(struct sockaddr_in *address,int id)
{
	struct player_list *p;

	if (!address)
	{
		for(p=&players;p->next;p=p->next)
		if (p->next->member.id==id)
			return p->next;
			return 0;
	}
	for(p=&players;p->next;p=p->next)
		if (((p->next->member.address.sin_addr.s_addr)==(address->sin_addr.s_addr)&&(p->next->member.address.sin_port)==(address->sin_port))&&
		     (p->next->member.id==id))
			return p->next;
	return 0;
}



/* create noise on given position */
void create_noise(int x,int y,struct player *p)
{
	struct it *o;

	p->obj->status|=4096;
	o=new_obj(id,T_NOISE,NOISE_TTL,noise_sprite,0,0,int2double(x),int2double(y),0,0,(void *)(p->id));
	if (!o)return;
	id++;
	sendall_new_object(o,0);
}



/* read packet from socket */
void read_data(void)
{
	unsigned char txt[256];
	unsigned char txt1[256];
	fd_set rfds;
	struct timeval tv;
	struct sockaddr_in client;
	int a=sizeof(client);
	static unsigned char packet[280];
	struct player *p;
	struct player_list *q;
	unsigned char min,maj;
	int s,x,y,l;

	packet[279]=0;

	tv.tv_sec=0;
	tv.tv_usec=0;
	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);
	while (select(fd+1,&rfds,0,0,&tv))
	{
		if ((l=recv_packet(packet,256,(struct sockaddr*)(&client),&a,0,0,&s))==-1)
			return;

 		last_packet_came=get_time();
 
		switch(*packet)
		{
			case P_NEW_PLAYER:
			if (l<6)break;   /* invalid packet */
			{
				unsigned long_long t;
				if (packet[1]) /* this byte must be always 0 - version authentification, old versions transmitted color byte here */
				{
					message("Incompatible client version.\n",2);
					packet[0]=P_PLAYER_REFUSED;
					packet[1]=E_PLAYER_REFUSED;
					send_packet(packet,2,(struct sockaddr*)(&client),0,last_player->member.id);
					break;
				}
				maj=packet[2];
				min=packet[3];
				print_ip(txt1,client.sin_addr);
				snprintf(txt,256,"Request for player #%d (client version %d.%d) from %s.\n",n_players,maj,min,txt1);
				message(txt,2);
				if (maj!=VERSION_MAJOR||min<MIN_CLIENT_VERSION_MINOR)
				{
					message("Incompatible client version. Player refused.\n",2);
					packet[0]=P_PLAYER_REFUSED;
					packet[1]=E_INCOMPATIBLE_VERSION;
					send_packet(packet,2,(struct sockaddr*)(&client),0,last_player->member.id);
					break;
				}
				find_birthplace(&x,&y);
				if (add_player(packet[4],packet+5,&client,x,y)) /* failed to add player */
				{
					message("Player refused.\n",2);
					packet[0]=P_PLAYER_REFUSED;
					packet[1]=E_PLAYER_REFUSED;
					send_packet(packet,2,(struct sockaddr*)(&client),0,last_player->member.id);
					break;
				}
				snprintf(txt,256,"Player #%d accepted, name \"%s\", address %s.\n",n_players,packet+5,txt1);
				message(txt,2);
				snprintf(txt,256,"%s entered the game.",packet+5);
				active_players++;
				n_players++;
				packet[0]=P_PLAYER_ACCEPTED;
				last_player->member.obj->status|=1024;
				put_int(packet+1,last_player->member.obj->id);
				put_int16(packet+5,last_player->member.obj->sprite);
				put_float(packet+7,last_player->member.obj->x);
				put_float(packet+11,last_player->member.obj->y);
				put_float(packet+15,last_player->member.obj->xspeed);
				put_float(packet+19,last_player->member.obj->yspeed);
				put_int16(packet+23,last_player->member.obj->status);
				t=get_time();
				put_long_long(packet+25,t-game_start);
				put_int(packet+33,last_player->member.id);
				packet[37]=VERSION_MAJOR;
				packet[38]=VERSION_MINOR;
				send_packet(packet,39,(struct sockaddr*)(&client),0,0);
				send_change_level(&(last_player->member));
				sendall_bell();
				sendall_message(0,txt,0,0);
				snprintf(txt,256,"There'%s %d %s in the game.",active_players==1?"s":"re",active_players,active_players==1?"player":"players");
				sendall_message(0,txt,0,0);
				send_info(0,0);
			}
			break;

			case P_LEVEL_ACCEPTED:
			q=find_player(&client,s);
			if (!q)break;
			if (q->member.current_level>=0)break;
			
			q->member.current_level=level_number;
			sendall_new_object(q->member.obj,&(q->member));
			create_noise(double2int(q->member.obj->x),double2int(q->member.obj->y),&(q->member));
			/* send all objects in the game */
			send_objects(&(q->member),q->member.obj);
			/* send all objects waiting for respawn */
			send_timeq_objects(&(q->member));
			break;

			case P_INFO:
			q=find_player(&client,s);
			if (!q) send_info((struct sockaddr*)(&client),0);
			else send_info((struct sockaddr*)(&client),q->member.id);
			break;

			case P_REENTER_GAME:
			q=find_player(&client,s);
			if (!q)break;
			if (!(q->member.obj->status&1024)||(q->member.obj->status&4096))break;
			find_birthplace(&x,&y);
			create_noise(x,y,&(q->member));
			q->member.obj->x=int2double(x);
			q->member.obj->y=int2double(y);
			sendall_update_object(q->member.obj,0,3);  /* update coordinates */
			break;
			
			case P_END:
			if (nonquitable)break;
			q=find_player(&client,s);
			if (!q)break;
			p=&(q->member);
			packet[0]=P_END;
			memcpy(packet+1,p->name,strlen(p->name)+1);
			sendall(packet,2+strlen(p->name),0);
			snprintf(txt,256,"Game terminated by player \"%s\".\n",p->name);
			message(txt,2);
			exit(0);

			case P_QUIT_REQUEST:
			q=find_player(&client,s);
			if (!q)  /* this player has been deleted, but due to network inconsistency he doesn't know it */
			{
				packet[0]=P_PLAYER_DELETED;
				send_packet(packet,1,(struct sockaddr*)(&client),0,last_player->member.id);
				break;
			}
			snprintf(txt,256,"%s left the game.\n",q->member.name);
			message(txt,2);
			packet[0]=P_PLAYER_DELETED;
			send_packet(packet,1,(struct sockaddr*)(&client),0,last_player->member.id);
			snprintf(txt,256,"%s left the game.",q->member.name);
			sendall_message(0,txt,0,0);
			delete_player(q);
			send_info(0,0);
			break;
		
			case P_KEYBOARD:
			if (l<3)break;   /* invalid packet */
			q=find_player(&client,s);
			if (!q)break;
			q->member.last_update=get_time();
			q->member.keyboard_status.right=(packet[1])&1;
			q->member.keyboard_status.left=((packet[1])>>1)&1;
			q->member.keyboard_status.jump=((packet[1])>>2)&1;
			q->member.keyboard_status.creep=((packet[1])>>3)&1;
			q->member.keyboard_status.speed=((packet[1])>>4)&1;
			q->member.keyboard_status.fire=((packet[1])>>5)&1;
			q->member.keyboard_status.down_ladder=((packet[1])>>6)&1;
			q->member.keyboard_status.weapon=packet[2];
			break;
			
			case P_MESSAGE:
			if (l<2)break;   /* invalid packet */
			q=find_player(&client,s);
			if (!q)break;
			sendall_message(q->member.name,packet+1,0,0);
			snprintf(txt,256,"%s> %s\n",q->member.name,packet+1);
			message(txt,1);
			break;

			default:
			snprintf(txt,256,"Unknown packet: head=%d\n",*packet);
			message(txt,2);
			break;
		}
	}
}


/* compute collision of two objects */
/* return value: 0=nothing */
/*               1=collision */
int collision(int x1,int y1,int t1,int s1,struct pos* p1,int x2,int y2,int t2,int s2,struct pos *p2)
{
	int w1,w2,h1,h2;
	unsigned char h=0,v=0;

	get_dimensions(t1,s1,p1,&w1,&h1);
	get_dimensions(t2,s2,p2,&w2,&h2);

	if ((x2>=x1&&x2<=x1+w1-1)||(x2+w2-1>=x1&&x2+w2-1<=x1+w1-1))h=1;
	if ((x1>=x2&&x1<=x2+w2-1)||(x1+w1-1>=x2&&x1+w1-1<=x2+w2-1))h=1;
	if (!h)return 0;
	
	if ((y2>=y1&&y2<=y1+h1-1)||(y2+h2-1>=y1&&y2+h2-1<=y1+h1-1))v=1;
	if ((y1>=y2&&y1<=y2+h2-1)||(y1+h1-1>=y2&&y1+h1-1<=y2+h2-1))v=1;
	if (!v)return 0;
	return 1;
}


/* create corpse on given position and with color num */
/* num is number of dead player */
void create_corpse(int x,int y,int num)
{
	struct it *o;
	static unsigned char txt[32];
	int a;
	int xoffs=num>15?-15:-5;
	int yoffs=num>15?-1:0;

	sprintf(txt,"corpse%d",num);
	if (find_sprite(txt,&a))return;
	
	o=new_obj(id,T_CORPSE,CORPSE_TTL,a,0,0,int2double(x+xoffs),int2double(y+yoffs),0,0,0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);
}


/* create mess on given position */
/* y1=y coordinate of the mess */
/* y2=y coordinate of the player (place where blood gushes and guts fly from */
void create_mess(int x,int y1,int y2)
{
	struct it *o;

	o=new_obj(id,T_CORPSE,CORPSE_TTL,mess1_sprite,0,0,int2double(x),int2double(y1),0,0,0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess2_sprite,0,0,int2double(x),int2double(y2),-float2double(2*36),-float2double((double)1.4*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess2_sprite,0,0,int2double(x),int2double(y2),float2double((double)2.8*36),-float2double(1*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess2_sprite,0,0,int2double(x),int2double(y2),float2double(3*36),float2double((double).5*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess3_sprite,0,0,int2double(x),int2double(y2),-float2double((double)2.5*36),float2double((double).3*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess4_sprite,0,0,int2double(x),int2double(y2),-float2double((double)3.3*36),-float2double((double)2.1*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess4_sprite,0,0,int2double(x),int2double(y2),float2double((double)2.9*36),-float2double((double)1.7*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess4_sprite,0,0,int2double(x),int2double(y2),-float2double((double)1.3*36),float2double((double).8*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);

	o=new_obj(id,T_MESS,MESS_TTL,mess3_sprite,0,0,int2double(x),int2double(y2),float2double((double).7*36),-float2double((double)1.2*36),0);
	if (!o)return;
	id++;
	sendall_new_object(o,0);
}


/* compute collision of given object with the others */
/* return value: 1=delete this object */
/*               0=don't delete */
/* 		 2= delete this object but don't send delete packet */
int dynamic_collision(struct it *obj)
{
	struct it *p;
	struct player_list *pl;
	unsigned char txt[256];
	struct it *o;
	my_double b;
	int a,c,s;
	int px,py,h;

#define OX (double2int(obj->x))
#define OY (double2int(obj->y))
#define PX (double2int(p->x))
#define PY (double2int(p->y))
#define A ((struct player*)(p->data))->armor
#define H ((struct player*)(p->data))->health
#define P ((struct player*)(p->data))
#define MAX_AMMO(x) (weapon[x].max_ammo)

	
	for (pl=&players;pl->next;pl=pl->next)
	{
		p=pl->next->member.obj;
		if (p->type==T_PLAYER&&(p->status&1024))continue; /* dead player */
		if (collision(OX,OY,obj->type,obj->status,sprites[obj->sprite].positions,PX,PY,p->type,p->status,sprites[p->sprite].positions))
			switch(obj->type)
			{
				case T_AMMO_GRENADE:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->ammo[WEAPON_GRENADE]==MAX_AMMO(WEAPON_GRENADE))break;
					((struct player*)(p->data))->ammo[WEAPON_GRENADE]+=weapon[WEAPON_GRENADE].add_ammo;
					if (((struct player*)(p->data))->ammo[WEAPON_GRENADE]>MAX_AMMO(WEAPON_GRENADE))
						((struct player*)(p->data))->ammo[WEAPON_GRENADE]=MAX_AMMO(WEAPON_GRENADE);
/*					P->current_weapon=select_best_weapon(P);  */
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got grenades");
					snprintf(txt,256,"%s got grenades.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_AMMO_GRENADE,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,AMMO_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_AMMO_GUN:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->ammo[0]==MAX_AMMO(0))break;

 					if(is_weapon_better((struct player*)(p->data), 0) 
 						&& is_weapon_empty((struct player*)(p->data), 0))
 						P->current_weapon = 0;

					((struct player*)(p->data))->ammo[0]+=weapon[0].add_ammo;
					if (((struct player*)(p->data))->ammo[0]>MAX_AMMO(0))
						((struct player*)(p->data))->ammo[0]=MAX_AMMO(0);
/*					P->current_weapon=select_best_weapon(P);  */
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got a magazine");
					snprintf(txt,256,"%s got a magazine.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_AMMO_GUN,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,AMMO_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_AMMO_SHOTGUN:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->ammo[1]==MAX_AMMO(1))break;

 					if(is_weapon_better((struct player*)(p->data), 1) 
 						&& is_weapon_empty((struct player*)(p->data), 1))
 						P->current_weapon = 1;

					((struct player*)(p->data))->ammo[1]+=weapon[1].add_ammo;
					if (((struct player*)(p->data))->ammo[1]>MAX_AMMO(1))
						((struct player*)(p->data))->ammo[1]=MAX_AMMO(1);
/*					P->current_weapon=select_best_weapon(P);  */
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got shotgun shells");
					snprintf(txt,256,"%s got shotgun shells.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_AMMO_SHOTGUN,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,AMMO_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_AMMO_RIFLE:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->ammo[3]==MAX_AMMO(3))break;

 					if(is_weapon_better((struct player*)(p->data), 3) 
 						&& is_weapon_empty((struct player*)(p->data), 3))
 						P->current_weapon = 3;

					((struct player*)(p->data))->ammo[3]+=weapon[3].add_ammo;
					if (((struct player*)(p->data))->ammo[3]>MAX_AMMO(3))
						((struct player*)(p->data))->ammo[3]=MAX_AMMO(3);
/*					P->current_weapon=select_best_weapon(P);  */
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got cartridges");
					snprintf(txt,256,"%s got cartridges.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_AMMO_RIFLE,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,AMMO_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_AMMO_UZI:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->ammo[2]==MAX_AMMO(2))break;

 					if(is_weapon_better((struct player*)(p->data), 2) 
 						&& is_weapon_empty((struct player*)(p->data), 2))
 						P->current_weapon = 2;

					((struct player*)(p->data))->ammo[2]+=weapon[2].add_ammo;
					if (((struct player*)(p->data))->ammo[2]>MAX_AMMO(2))
						((struct player*)(p->data))->ammo[2]=MAX_AMMO(2);
/*					P->current_weapon=select_best_weapon(P); */
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got ammo for Uzi");
					snprintf(txt,256,"%s got Uzi ammo.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_AMMO_UZI,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,AMMO_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_UZI:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if ((((struct player*)(p->data))->ammo[2]==MAX_AMMO(2))&&((((struct player *)(p->data))->weapons)&4))break;

 					if(is_weapon_better((struct player*)(p->data), 2) 
 						&& !is_weapon_usable((struct player*)(p->data), 2))
 						P->current_weapon = 2;

					((struct player*)(p->data))->weapons|=4;
					((struct player*)(p->data))->ammo[2]+=weapon[2].basic_ammo;
					if (((struct player*)(p->data))->ammo[2]>MAX_AMMO(2))
						((struct player*)(p->data))->ammo[2]=MAX_AMMO(2);
//					P->current_weapon=select_best_weapon(P);
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got Uzi");
					snprintf(txt,256,"%s got Uzi.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_UZI,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,WEAPON_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_RIFLE:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if ((((struct player*)(p->data))->ammo[3]==MAX_AMMO(3))&&((((struct player *)(p->data))->weapons)&8))break;

 					if(is_weapon_better((struct player*)(p->data), 3) 
 						&& !is_weapon_usable((struct player*)(p->data), 3))
 						P->current_weapon = 3;

					((struct player*)(p->data))->weapons|=8;
					((struct player*)(p->data))->ammo[3]+=weapon[3].basic_ammo;
					if (((struct player*)(p->data))->ammo[3]>MAX_AMMO(3))
						((struct player*)(p->data))->ammo[3]=MAX_AMMO(3);
//					P->current_weapon=select_best_weapon(P);
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got sniper rifle");
					snprintf(txt,256,"%s got sniper rifle.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_RIFLE,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,WEAPON_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_SHOTGUN:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if ((((struct player*)(p->data))->ammo[1]==MAX_AMMO(1))&&((((struct player *)(p->data))->weapons)&2))break;

 					if(is_weapon_better((struct player*)(p->data), 1) 
 						&& !is_weapon_usable((struct player*)(p->data), 1))
 						P->current_weapon = 1;

					((struct player*)(p->data))->weapons|=2;
					((struct player*)(p->data))->ammo[1]+=weapon[1].basic_ammo;
					if (((struct player*)(p->data))->ammo[1]>MAX_AMMO(1))
						((struct player*)(p->data))->ammo[1]=MAX_AMMO(1);
//					P->current_weapon=select_best_weapon(P);
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got a shotgun");
					snprintf(txt,256,"%s got a shotgun.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_SHOTGUN,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,WEAPON_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_INVISIBILITY:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;

					((struct player*)(p->data))->invisibility_counter=INVISIBILITY_DURATION;
					p->status|=64;   /* hide player */
					sendall_update_status(p,0);
					send_message((struct player*)(p->data),0,"You got invisibility dope");
					snprintf(txt,256,"%s got invisibility.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_INVISIBILITY,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,INVISIBILITY_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_ARMOR:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->armor>=100)break; /* he has 100% armor */
					((struct player*)(p->data))->armor+=ARMOR_ADD;
					if (((struct player*)(p->data))->armor>100)((struct player*)(p->data))->armor=100;
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You got armor");
					snprintf(txt,256,"%s got armor.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_ARMOR,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,ARMOR_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_MEDIKIT:
				{
					unsigned char txt[256];
					if (p->type!=T_PLAYER)break;
					if (((struct player*)(p->data))->health>=100)break; /* he's healthy */
					((struct player*)(p->data))->health+=MEDIKIT_HEALTH_ADD;
					if (((struct player*)(p->data))->health>100)((struct player*)(p->data))->health=100;
					send_update_player((struct player*)(p->data));
					send_message((struct player*)(p->data),0,"You picked up a medikit");
					snprintf(txt,256,"%s picked up a medikit.\n",((struct player*)(p->data))->name);
					message(txt,1);
					obj->status|=64;
					add_to_timeq(obj->id,T_MEDIKIT,0,obj->sprite,0,0,obj->x,obj->y,0,0,0,MEDIKIT_RESPAWN_TIME);
        				sendall_update_status(obj,0);
				}
				return 2;

				case T_KILL:
				if (p->type!=T_PLAYER)break;
				a=(1-(double)A/100)*KILL_LETHALNESS;
				c=KILL_ARMOR_DAMAGE;
				if (a>=H)  /* player died */
				{
					((struct player*)(p->data))->deaths++;
					send_message((struct player*)(p->data),0,"You killed yourself");
					snprintf(txt,256,"%s suicides",((struct player*)(p->data))->name);
					sendall_message(0,txt,(struct player*)(p->data),0);
					snprintf(txt,256,"%s suicides.\n",((struct player*)(p->data))->name);
					message(txt,2);

					s=p->status;
					px=PX;
					py=PY;
					h=H;
					H=0;
					p->xspeed=0;
					p->yspeed=0;
					p->status|=1024;  /* dead flag */
					sendall_update_object(p,0,0); /* update everything */
					send_update_player((struct player*)(p->data));   /* dead player */
					send_info(0,0);
					if (a-h>=OVERKILL)
						create_mess(px,py+((s&256)?CREEP_HEIGHT:PLAYER_HEIGHT)-MESS_HEIGHT,py);
					else
						create_corpse(px,py+((s&256)?CREEP_HEIGHT:PLAYER_HEIGHT)-CORPSE_HEIGHT,((struct player*)(p->data))->color);
				}
				else 
				{
					H-=a;  /* health */
					A=(c>=A)?0:(A-c);   /* armor */
					send_update_player((struct player*)(p->data));
				}
				return 0;
				
				case T_SHRAPNEL:
				if (p->type==T_CORPSE)
				{
					unsigned char packet[5];
					px=PX;
					py=PY;
					packet[0]=P_DELETE_OBJECT;
					put_int(packet+1,p->id);
					sendall_chunked(packet,5,0);
					delete_obj(p->id);
					create_mess(px,py,py);
					return 1;
				}
				case T_BULLET:
				if (p->type!=T_PLAYER)break;
				b=(obj->type==T_BULLET?FIRE_IMPACT:SHRAPNEL_IMPACT);
				p->status|=128;
				p->xspeed+=obj->xspeed>0?b:-b;
				sendall_update_object(p,0,4);  /* update speed + status */
				p->status&=~128;
				a=(1-(double)A/100)*weapon[obj->status].lethalness*(2-double2int(obj->y-p->y)/(double)PLAYER_HEIGHT)*obj->ttl/weapon[obj->status].ttl;
				c=weapon[obj->status].armor_damage*(2-double2int(obj->y-p->y)/(double)PLAYER_HEIGHT)*obj->ttl/weapon[obj->status].ttl;
				if (a>=H)  /* player was slain */
				{
					o=&((find_in_table((int)(obj->data)))->member);  /* owner of the bullet */
					((struct player*)(p->data))->deaths++;
					if (o->data==p->data) /* suicide */
					{
						((struct player*)(o->data))->frags-=!!(((struct player*)(o->data))->frags);
						send_message((struct player*)(o->data),0,"You killed yourself");
						snprintf(txt,256,"%s suicides",((struct player*)(o->data))->name);
						sendall_message(0,txt,(struct player*)(o->data),0);
						snprintf(txt,256,"%s suicides.\n",((struct player*)(o->data))->name);
						message(txt,2);
					}
					else
					{
						((struct player*)(o->data))->frags++;
						snprintf(txt,256,"%s killed %s.",((struct player*)(o->data))->name,((struct player*)(p->data))->name);
						sendall_message(0,txt,(struct player*)(o->data),(struct player*)(p->data));
						snprintf(txt,256,"%s killed you",((struct player*)(o->data))->name);
						send_message((struct player*)(p->data),0,txt);  /* the dead */
						snprintf(txt,256,"You killed %s",((struct player*)(p->data))->name);
						send_message((struct player*)(o->data),0,txt);  /* the dead */
						snprintf(txt,256,"%s killed %s.\n",((struct player*)(o->data))->name,((struct player*)(p->data))->name);
						message(txt,2);
					}
					s=p->status;
					px=PX;
					py=PY;
					h=H;
					H=0;
					p->xspeed=0;
					p->yspeed=0;
					p->status|=1024;  /* dead flag */
					sendall_update_object(p,0,0); /* update everything */
					send_update_player((struct player*)(p->data));   /* dead player */
					send_update_player((struct player*)(o->data));  /* owner of bullet/shrapnel */
					send_info(0,0);
					if (a-h>=OVERKILL)
						create_mess(px,py+((s&256)?CREEP_HEIGHT:PLAYER_HEIGHT)-MESS_HEIGHT,py);
					else
						create_corpse(px,py+((s&256)?CREEP_HEIGHT:PLAYER_HEIGHT)-CORPSE_HEIGHT,((struct player*)(p->data))->color);
				}
				else 
				{
					H-=a;  /* health */
					A=(c>=A)?0:(A-c);   /* armor */
					sendall_hit(p->id,obj->xspeed<0,OX-PX,OY-PY,0);
					send_update_player((struct player*)(p->data));
				}
				return 1;
			}
	}
	return 0;

#undef MAX_AMMO	
#undef P
#undef H
#undef A
#undef OX
#undef OY
#undef PX
#undef PY
}


/* recompute objects positions */
void update_game(void)
{
	static unsigned char packet[64];
        struct object_list *p;
	struct player_list *q;
        int w,h,b,a;
	unsigned char stop_x,stop_y;
	my_double x,y,xs,ys;
	my_double x1,y1;
	unsigned char sy;
	struct it *s;   /* for grenades throwing */
	int status;
	my_double DELTA_TIME;
	unsigned long_long t;
	unsigned int old_status;
	int old_ttl;
	my_double old_x,old_y,old_x_speed,old_y_speed;

        for(p=&objects;p->next;p=p->next)
        {
		if (p->next->member.type==T_NOTHING)continue;
		if (p->next->member.type==T_PLAYER&&(p->next->member.status&1024))continue;  /* dead player */
		if (p->next->member.type==T_PLAYER&&(((struct player*)(p->next->member.data))->current_level)<0)  /* player hasn't entered the level yet */
		{
			send_change_level((struct player*)p->next->member.data);
			continue;
		}
		
		old_status=p->next->member.status;
		old_ttl=p->next->member.ttl;
		old_x=p->next->member.x;
		old_y=p->next->member.y;
		old_x_speed=p->next->member.xspeed;
		old_y_speed=p->next->member.yspeed;

		x=p->next->member.x;
		y=p->next->member.y;
		xs=p->next->member.xspeed;
		ys=p->next->member.yspeed;
		status=p->next->member.status;
		
		/* decrease invisibility counter of invisible player */
		if ((p->next->member.type==T_PLAYER)&&(((struct player*)(p->next->member.data))->invisibility_counter))
		{
			((struct player*)(p->next->member.data))->invisibility_counter--;
			if (!(((struct player*)(p->next->member.data))->invisibility_counter))
			{
				p->next->member.status&=~64;
				sendall_update_status(&(p->next->member),0);
			}
		}


		/* decrement time to live */
		if (p->next->member.ttl>0)
		{
			p->next->member.ttl--;
			/* create player */
			if (p->next->member.type==T_NOISE&&p->next->member.ttl==(NOISE_TTL>>1))
			{
				/* find player */
				q=find_player(0,(int)(p->next->member.data));
				if (q) /* player is still in the game */
				{
					init_player(&(q->member),q->member.obj->x,q->member.obj->y);
					q->member.obj->status&=~1024;
					q->member.obj->status&=~4096;
					sendall_update_object(q->member.obj,0,0);  /* update everything */
					send_update_player(&(q->member));
				}

				goto cont_cycle;   /* that's all for T_NOISE at this moment */
			}
			/* grenades must be created after locking off */
			/* not when shoot request comes */
			if (p->next->member.type==T_PLAYER&&p->next->member.ttl==GRENADE_DELAY+HOLD_GUN_AFTER_SHOOT&&((struct player*)(p->next->member.data))->current_weapon==WEAPON_GRENADE)
			{
				s=new_obj(
					id,
					T_GRENADE,
					weapon[WEAPON_GRENADE].ttl,
					grenade_sprite,
					0,
					WEAPON_GRENADE,
					add_int(p->next->member.x,p->next->member.status&2?2:PLAYER_WIDTH-2),
					p->next->member.y+GRENADE_FIRE_YOFFSET,
					p->next->member.xspeed+(p->next->member.status&2?-weapon[WEAPON_GRENADE].shell_xspeed:weapon[WEAPON_GRENADE].shell_xspeed),
					p->next->member.yspeed+weapon[WEAPON_GRENADE].shell_yspeed,
					(void *)(p->next->member.id)); 
				id++;
				sendall_new_object(s,0);
			}
			/* if ttl is 0, delete this object */
			if (p->next->member.ttl<=0)
			{
				/* player's ttl means how long he holds the gun */
				switch(p->next->member.type)
				{
					case T_PLAYER:
					p->next->member.status&=~16;
					break;

					case T_GRENADE:
					packet[0]=P_EXPLODE_GRENADE;
					put_int(packet+1,id);
					put_int(packet+5,p->next->member.id);
					sendall_chunked(packet,9,0);
					
					for (b=0;b<N_SHRAPNELS_EXPLODE;b++)
					{
						double angle=(double)b*2*M_PI/N_SHRAPNELS_EXPLODE;
						my_double spd=add_int(mul_int(my_and(mul_int(weapon[WEAPON_GRENADE].speed,b+1),15),16),100);
						
						new_obj(
							id,
							T_SHRAPNEL,
							SHRAPNEL_TTL,
							shrapnel_sprite[random()%N_SHRAPNELS],
							0,
							WEAPON_GRENADE,
							p->next->member.x,
							p->next->member.y,
							p->next->member.xspeed+mul(spd,float2double(cos(angle))),
							p->next->member.yspeed+mul(spd,float2double(sin(angle))),
							p->next->member.data); 
						id++;
					}
					delete_obj(p->next->member.id);
					if (!(p->next))return;
					goto cont_cycle;

					default:
					packet[0]=P_DELETE_OBJECT;
					put_int(packet+1,p->next->member.id);
					sendall_chunked(packet,5,0);
					delete_obj(p->next->member.id);
					if (!(p->next))return;
					goto cont_cycle;
				}
			}
		}
		/* maintain only objects that you are allowed to maintain */
		if (!obj_attr[p->next->member.type].maintainer)goto dc;
		if (!(obj_attr[p->next->member.type].maintainer&2))continue;
		
		
		/* when not falling, slow down x motion */
		if (!(p->next->member.status&8))p->next->member.xspeed=mul(p->next->member.xspeed,obj_attr[p->next->member.type].slow_down_x);
		
		/* fall */
		if (obj_attr[p->next->member.type].fall)
		{
			if (p->next->member.type!=T_SHRAPNEL)p->next->member.status|=8;
			p->next->member.yspeed+=FALL_ACCEL;
			/* but not too fast */
			if (p->next->member.yspeed>MAX_Y_SPEED)p->next->member.yspeed=MAX_Y_SPEED;
		}
			
		t=get_time();
                get_dimensions(p->next->member.type,p->next->member.status,sprites[p->next->member.sprite].positions,&w,&h);
		DELTA_TIME=float2double(((double)((long_long)(t-p->next->member.last_updated)))/MICROSECONDS);
                update_position(
                        &(p->next->member),
                        p->next->member.x+mul(p->next->member.xspeed,DELTA_TIME),
                        p->next->member.y+mul(p->next->member.yspeed,DELTA_TIME),
                        w,h,&stop_x,&stop_y);
		p->next->member.last_updated=t;
		
		/* walk up the stairs */
		if (stop_x&&p->next->member.type==T_PLAYER&&!(p->next->member.status&256))
		{
			x1=p->next->member.x;
			y1=p->next->member.y;
			p->next->member.x=x;
			p->next->member.y=y-int2double(1);
	                update_position(
        	                &(p->next->member),
                	        p->next->member.x+mul(p->next->member.xspeed,DELTA_TIME),
                        	p->next->member.y+mul(p->next->member.yspeed,DELTA_TIME),
	                        w,h,0,&sy);
			if ((p->next->member.xspeed>0&&p->next->member.x<=x1)||(p->next->member.xspeed<0&&p->next->member.x>=x1)) /* restore original values */
			{
				p->next->member.x=x1;
				p->next->member.y=y1;
			}
			else
			{
				stop_y=sy;
				stop_x=0;
			}
		}
		
		if (stop_x)p->next->member.xspeed=-mul(p->next->member.xspeed,obj_attr[p->next->member.type].bounce_x);
		if (my_abs(p->next->member.xspeed)<MIN_X_SPEED)
		{
			p->next->member.xspeed=0;
			p->next->member.status&=~1;
		}


		if (stop_y)
		{
			p->next->member.yspeed=mul(p->next->member.yspeed,obj_attr[p->next->member.type].bounce_y);
			p->next->member.yspeed=-p->next->member.yspeed;
			if (my_abs(p->next->member.yspeed)<MIN_Y_SPEED)
			{
				p->next->member.yspeed=0;
				if (stop_y==1)p->next->member.status&=~8;
			}
		}

		if ((p->next->member.type==T_SHRAPNEL||p->next->member.type==T_BULLET)&&(stop_x||stop_y))  /* bullet and shrapnel die crashing into wall */
		{
			packet[0]=P_DELETE_OBJECT;
			put_int(packet+1,p->next->member.id);
			sendall_chunked(packet,5,0);
			delete_obj(p->next->member.id);
			if (!(p->next))break;
			continue;
		}
dc:
		/* compute collision with other objects */
		a=dynamic_collision(&(p->next->member));
		switch (a)
		{ 
			case 1:  /* object dies */
			packet[0]=P_DELETE_OBJECT;
			put_int(packet+1,p->next->member.id);
			sendall_chunked(packet,5,0);

			case 2:
			delete_obj(p->next->member.id);
			if (!(p->next))return;
			goto cont_cycle;

			case 0:   /* object still lives */
				if (  /* send update but only when something's changed and client doesn't maintain the object */
					(obj_attr[p->next->member.type].maintainer&4)&&  /* server sends update */
					(x!=p->next->member.x||  /* something's changed */
					y!=p->next->member.y||
					xs!=p->next->member.xspeed||
					ys!=p->next->member.yspeed||
					status!=p->next->member.status))
				{
					a=which_update(&(p->next->member),old_x,old_y,old_x_speed,old_y_speed,old_status,old_ttl);
					sendall_update_object(&(p->next->member),0,a);
				}
			break;
		}
cont_cycle:;
        }
}

void free_all_memory(void)
{
	struct queue_list *t;
	struct player_list *p;

	/* delete players */
	for (p=&players;p->next;)
		delete_player(p->next);
	/* delete objects */
	while (last_obj!=(&objects))delete_obj(last_obj->member.id);

	for (t=&time_queue;t->next;)
	{
		struct queue_list *q;

		q=t->next->next;
		mem_free(t->next);
		t->next=q;
	}

	/* delete birthplaces */
	if (n_birthplaces)mem_free(birthplace);
	
	/* delete sprites */
	free_sprites(0);
	
	free_area();
	if (level_checksum)mem_free(level_checksum);
}

/* fatal signal handler (sigsegv, sigabrt, ... ) */
void signal_handler(int sig_num)
{
	unsigned char packet[16];
	unsigned char txt[256];
	
	packet[0]=P_END;
	memcpy(packet+1,"server",7);

	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	sendall(packet,8,0);
	/* 800 % redundancy should be enough ;-) */

	snprintf(txt,256,"Signal %d caught.\n",sig_num);
	message(txt,2);
	free_all_memory();
	check_memory_leaks();
#ifdef WIN32
	hServerExitEvent=1;
#else
	signal(sig_num,SIG_DFL);
	raise(sig_num);
#endif
}


/* walk with given player */
void walk_player(struct player *q,int direction, int speed, int creep)
{
	int a;

	unsigned int old_status=q->obj->status;
	int old_ttl=q->obj->ttl;
	my_double 	old_x=q->obj->x,
			old_y=q->obj->y,
			old_x_speed=q->obj->xspeed,
			old_y_speed=q->obj->yspeed;
	

	if ((q->obj->status&(512+16))==(512+16))return;  /* when throwing grenade can't walk */
	if (creep)  /* creep */
	{
		a=MAX_SPEED_CREEP;
		if (!(q->obj->status&256))q->obj->y+=CREEP_YOFFSET;
		q->obj->status|=256;
		
	}
	else 
	{
		a=speed?MAX_SPEED_WALK_FAST:MAX_X_SPEED;
		if (q->obj->status&256)q->obj->y-=CREEP_YOFFSET;
		q->obj->status&=~256;
	}
	switch (direction)
	{
		case 0:  /* stop */
		q->obj->status&=~1;
		q->obj->xspeed=0;
		break;

		case 1:  /* left */
		q->obj->status|=1;
		q->obj->xspeed-=WALK_ACCEL;
		if (q->obj->xspeed<-a)q->obj->xspeed=-a;
		break;

		case 2:  /* right */
		q->obj->status|=1;
		q->obj->xspeed+=WALK_ACCEL;
		if (q->obj->xspeed>a)q->obj->xspeed=a;
		break;
	}

	a=which_update(q->obj,old_x,old_y,old_x_speed,old_y_speed,old_status,old_ttl);
	
	sendall_update_object(q->obj,0,a);
}


/* jump with given player */
void jump_player(struct player *p)
{
	if (p->obj->status&8||p->obj->status&256)return;
	p->obj->status|=8;
	p->obj->yspeed=-SPEED_JUMP;
	sendall_update_object(p->obj,0,4);   /* update speed + status */
}


/* change weapon of given player (w=new weapon) */
void change_weapon_player(struct player *q,int w)
{
	unsigned char txt[256];

	if (!w)return;
	w--;
	if (q->current_weapon==w)return;
	if (!(q->weapons&(1<<w)))
		{send_message(q,0,"No weapon.");return;}
	if (!(q->ammo[w]))
		{send_message(q,0,"Not enough ammo.");return;}
	q->current_weapon=w;
	snprintf(txt,256,"%s takes %s.\n",q->name,weapon[w].name);
	message(txt,1);
	send_update_player(q);
}


/* shoot with given player */
/* direction: 0=right, 1=left */
void fire_player(struct player *q,int direction)
{
	int a;
	struct it *s;

	if (!(q->obj->status&6)||(q->obj->status&256)||((q->obj->status&16)&&(q->obj->ttl>HOLD_GUN_AFTER_SHOOT)))return;
	if (!q->ammo[q->current_weapon])
	{
		a=select_best_weapon(q);
		if (a==q->current_weapon) return;
		q->current_weapon=a;
	}
	q->ammo[q->current_weapon]--;
	send_update_player(q);
	q->obj->status&=~512;
	if (q->current_weapon==WEAPON_SHOTGUN) /* shotgun */
	{
		s=new_obj(  /* SHELL */
			id,
			T_SHELL,
			SHELL_TTL,
			shotgun_shell_sprite,
			0,
			0,
			add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
			q->obj->y+FIRE_YOFFSET,
			q->obj->xspeed+(direction==1?-weapon[1].shell_xspeed:weapon[1].shell_xspeed),
			weapon[1].shell_yspeed,
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* straight */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?-2:PLAYER_WIDTH+2),
			q->obj->y+FIRE_YOFFSET,
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			0,
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* straight */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
			q->obj->y+FIRE_YOFFSET+int2double(1),
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			0,
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* one up */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?-1:PLAYER_WIDTH+1),
			q->obj->y+FIRE_YOFFSET,
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			float2double((double).1*36),
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* two up */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
			q->obj->y+FIRE_YOFFSET-int2double(1),
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			float2double((double).15*36),
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* one down */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
			q->obj->y+FIRE_YOFFSET+int2double(1),
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			-float2double((double).1*36),
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
		s=new_obj(  /* two down */
			id,
			T_BULLET,
			weapon[q->current_weapon].ttl,
			slug_sprite,
			0,
			q->current_weapon,
			add_int(q->obj->x,direction==1?-1:PLAYER_WIDTH+1),
			q->obj->y+FIRE_YOFFSET,
			q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
			-float2double((double).15*36),
			(void *)(q->obj->id)); 
		id++;
		sendall_new_object(s,0);
	}
	else
	{
		if (q->current_weapon!=WEAPON_GRENADE)  /* not grenades */
		{
			s=new_obj(  /* SHELL */
				id,
				T_SHELL,
				SHELL_TTL,
				shell_sprite,
				0,
				0,
				add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
				q->obj->y+FIRE_YOFFSET,
				q->obj->xspeed+(direction==1?-weapon[q->current_weapon].shell_xspeed:weapon[q->current_weapon].shell_xspeed),
				weapon[q->current_weapon].shell_yspeed,
				(void *)(q->obj->id)); 
			id++;
			sendall_new_object(s,0);
			s=new_obj(
				id,
				T_BULLET,
				weapon[q->current_weapon].ttl,
				bullet_sprite,
				0,
				q->current_weapon,
				add_int(q->obj->x,direction==1?0:PLAYER_WIDTH),
				q->obj->y+FIRE_YOFFSET,
				q->obj->xspeed+(direction==1?-weapon[q->current_weapon].speed:weapon[q->current_weapon].speed),
				0,
				(void *)(q->obj->id)); 
			id++;
			sendall_new_object(s,0);
		}
		else  /* grenades */
		{
			q->obj->status|=512;
			q->obj->status&=~1;
		}
	}
	q->obj->xspeed+=(direction==1)?weapon[q->current_weapon].impact:-weapon[q->current_weapon].impact;
	q->obj->status|=16;
	q->obj->status|=32;
	q->obj->ttl=weapon[q->current_weapon].cadence+HOLD_GUN_AFTER_SHOOT;
	sendall_update_object(q->obj,0,6);  /* update speed and status and ttl */
	q->obj->status&=~32;
}


/* update given player (jump, shoot, creep, change weapon) */
void move_player(struct player *p)
{
	int a;

	if (p->obj->status&1024)return;   /* dead player */

	if (p->keyboard_status.down_ladder)  /* climb down a ladder */
		p->obj->status|=2048;
	else
		p->obj->status&=~2048;

	if (p->keyboard_status.jump)
		jump_player(p);
	if (p->keyboard_status.right)
	{
		if ((p->obj->status&6)==4)   /* walk right */
			walk_player(p,2,p->keyboard_status.speed,p->keyboard_status.creep);
		else
		{
                        if ((p->obj->status)&1)
                                walk_player(p,0,p->keyboard_status.speed,p->keyboard_status.creep);   /* stop */
                        else
                        {
                                a=(p->obj->status&6)>>1;
                                p->obj->status&=~6;
                                p->obj->status|=(a==1)?0:4;
                                sendall_update_status(p->obj,0);
			}
		}
	}

	if (p->keyboard_status.left)
	{
		if ((p->obj->status&6)==2)   /* walk right */
			walk_player(p,1,p->keyboard_status.speed,p->keyboard_status.creep);
		else
		{
                        if ((p->obj->status)&1)
                                walk_player(p,0,p->keyboard_status.speed,p->keyboard_status.creep);   /* stop */
                        else
                        {
                                a=(p->obj->status&6)>>1;
                                p->obj->status&=~6;
                                p->obj->status|=(a==2)?0:2;
                                sendall_update_status(p->obj,0);
			}
		}
	}
	change_weapon_player(p,p->keyboard_status.weapon);
	if (p->keyboard_status.fire)
		fire_player(p,(p->obj->status&6)>>1);
}


/* update players, kick out not responding players */
void update_players(void)
{
	struct player_list *p;
	unsigned char txt[256];
	unsigned char packet;
	unsigned long_long t=get_time();

	for (p=&players;p->next;p=p->next)
	{
		/* this player is dead - delete him */
		if (t-(p->next->member).last_update>=MAX_DUMB_TIME)
		{
			snprintf(txt,256,"%s not responding. Kicked out of the game.\n",p->next->member.name);
			message(txt,2);
			packet=P_PLAYER_DELETED;
			send_packet(&packet,1,(struct sockaddr*)(&(p->next->member.address)),0,last_player->member.id);
			snprintf(txt,256,"%s was kicked out of the game.",p->next->member.name);
			sendall_message(0,txt,0,0);
			delete_player(p->next);
			if (!(p->next))break;
		}
		else
			move_player(&(p->next->member));
	}
}


/* write help message to stdout */
void print_help(void)
{
#ifdef WIN32
	printf(	"0verkill server.\n"
		"(c)2000 Brainsoft\n"
		"Portions (c) 2000 by Filip Konvicka\n"
		"Usage: server [-nh] [-l <level_number>] [-p <port number>] [-i username[/password]] [-I] [-r]\n"
		"-i		Installs as a service\n"
		"-I		Installs with the LOCAL_SYSTEM account.\n"
		"-r		Stops and removes the service\n"
		"-n		Server can't be ended by client\n"
		"You must be an administrator in order to install/remove a service.\n");
#else
	printf(	"0verkill server.\n"
		"(c)2000 Brainsoft\n"
#ifdef __EMX__
		"Portions (c) 2000 by Mikulas Patocka\n"
#endif
		"Usage: server [-nh] [-l <level number>] [-p <port number>]\n"
		"-n		Server can't be ended by client\n"
		);
#endif
}


void parse_command_line(int argc,char **argv)
{
	int a;
	char *c;
	
	while(1)
	{
#ifdef WIN32
		a=getopt(argc,argv,"hl:np:ri:Ic");
#else
		a=getopt(argc,argv,"hl:np:");
#endif
		switch(a)
		{
			case EOF:
			return;

			case '?':
			case ':':
			EXIT(1);

#ifdef WIN32
			case 'c':
				break;	/* run as console app */
			case 'r':	/* remove service */
				CmdRemoveService();
				EXIT(1);
			case 'i': {	/* install service */
				char	username[80],
						password[80],
						*user=NULL,
						*pass=NULL;
				if ( optarg ) {
					username[sizeof(username)-1]=0;
					user=username;
					if ( (pass=strchr(optarg, '/'))==NULL ) {
						strcpy(username, optarg);
						memcpy(username, optarg, sizeof(username)-1);
						password[0]=0;
					}
					else {
						int		size=pass-optarg;
						if ( size>sizeof(username)-1 )
							size=sizeof(username)-1;
						memcpy(username, optarg, size);
						username[size]=0;
						pass++;
						size=strlen(pass);
						if ( size>sizeof(password)-1 )
							size=sizeof(password)-1;
						memcpy(password, pass, size);
					}
				}
				CmdInstallService(user, pass, NULL);
				EXIT(1);
			}
			case 'I': {	/* install service */
				CmdInstallService(NULL, NULL, NULL);
				EXIT(1);
			}
#endif

			case 'h':
			print_help();
			EXIT(0);

			case 'p':
			port=strtoul(optarg,&c,10);
			if (*c){ERROR("Error: Not a number.\n");EXIT(1);}
			if (errno==ERANGE)
			{
				if (!port){ERROR("Error: Number underflow.\n");EXIT(1);}
				else {ERROR("Error: Number overflow.\n");EXIT(1);}
			}
			break;
			
			case 'l':
			level_number=strtoul(optarg,&c,10);
			if (*c){ERROR("Error: Not a number.\n");EXIT(1);}
			if (errno==ERANGE)
			{
				if (!level_number){ERROR("Error: Number underflow.\n");EXIT(1);}
				else {ERROR("Error: Number overflow.\n");EXIT(1);}
			}
			break;
			
			case 'n':
			nonquitable=1;
			break;
		}
	}
}


/*-----------------------------------------------------------------------------------*/
int server(void)
{
	int a;
	unsigned char txt[256];
	unsigned long_long last_time;
	unsigned char *LEVEL;
	
	last_player=&players;
	last_obj=&objects;
	
	snprintf(txt,256,"Running 0verkill server version %d.%d\n",VERSION_MAJOR,VERSION_MINOR);
	message(txt,2);
#ifdef WIN32
	snprintf(txt,256,"This is 0verkill server for Win32, build #%u\n",VERSION_PORT);
	message(txt,2);
#endif
	message("Initialization.\n",2);
#ifdef WIN32
	message("Starting Windows Sockets\n",2);
	{
		WSADATA	wd;
		WSAStartup(0x101, &wd);
		snprintf(txt,256,"Started WinSock version %X.%02X\n", wd.wVersion/0x100, wd.wVersion&0xFF);
		message(txt, 2);
	}
#endif
	init_area();  /* initialize playing area */
	hash_table_init();

#ifdef WIN32
	if ( !consoleApp )
		ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 2000);
#endif

	message("Loading sprites.\n",2);
	load_sprites(DATA_PATH GAME_SPRITES_FILE); /* players, corpses, bullets, ... */
	if (find_sprite("bullet",&bullet_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"bullet\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("slug",&slug_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"slug\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("shell",&shell_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"shell\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("sshell",&shotgun_shell_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"sshell\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("grenade",&grenade_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"grenade\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("mess1",&mess1_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"mess1\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("mess2",&mess2_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"mess2\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("mess3",&mess3_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"mess3\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("mess4",&mess4_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"mess4\".\n");ERROR(msg);EXIT(1);}
	if (find_sprite("noise",&noise_sprite)){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"noise\".\n");ERROR(msg);EXIT(1);}
	for (a=0;a<N_SHRAPNELS;a++)
	{
		sprintf(txt,"shrapnel%d",a+1);
		if (find_sprite(txt,&shrapnel_sprite[a])){unsigned char msg[256];snprintf(msg,256,"Can't find sprite \"%s\".\n",txt);ERROR(msg);EXIT(1);}
	}
	
	LEVEL=load_level(level_number);
	level_checksum=md5_level(level_number);
	if (!LEVEL){char txt[256];snprintf(txt,256,"Can't load level number %d\n",level_number);ERROR(txt);EXIT(1);}
	snprintf(txt,256,"Loading level \"%s\"....\n",LEVEL);
	message(txt,2);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,LEVEL_SPRITES_SUFFIX);
	message("Loading level graphics.\n",2);
	load_sprites(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,STATIC_DATA_SUFFIX);
	message("Loading level map.\n",2);
	load_data(txt);
	snprintf(txt,256,"%s%s%s",DATA_PATH,LEVEL,DYNAMIC_DATA_SUFFIX);
	message("Loading level objects.\n",2);
	mem_free(LEVEL);
	load_dynamic(txt);

	message("Initializing socket.\n",2);
	init_socket();  /* initialize socket */
	
	message("Installing signal handlers.\n",2);
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGFPE,signal_handler);
	signal(SIGILL,signal_handler);
	signal(SIGABRT,signal_handler);
#ifndef WIN32
	signal(SIGBUS,signal_handler);
	signal(SIGQUIT,signal_handler);
#endif
	message("Game started.\n",2);

#ifdef __EMX__
	DosSetPriority(PRTYS_PROCESS, PRTYC_FOREGROUNDSERVER, 1, 0);
#endif
	
	game_start=get_time();
	srandom(game_start);

#ifdef WIN32
	if ( !consoleApp )
		ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0);
#endif

	last_time=get_time();
again:
	last_time+=PERIOD_USEC;
	if (get_time()-last_time>PERIOD_USEC*100)last_time=get_time();
	read_data();
	update_timeq();
	update_game();
	update_players();    /* MUST come after update_game otherwise when player shoots he hit himself */
	send_chunks();
	last_tick=get_time();
	if (!active_players&&(last_tick-last_player_left)>DELAY_BEFORE_SLEEP_USEC&&(last_tick-last_packet_came)>DELAY_BEFORE_SLEEP_USEC)
	{
#ifndef WIN32
		message("Sleep\n",2);
		{
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd,&fds);
			select(fd+1,&fds,0,0,0);
		}
		message("Wakeup\n",2);
#else
		WaitForSingleObject(fd, 500); /* wait max. 0.5 seconds, then we must test hServerExitEvent */
#endif
	}
#ifdef WIN32
	/* we must return so that the service can be properly stopped */
	if ( hServerExitEvent )
		return 0;
#endif
	sleep_until(last_time+PERIOD_USEC);
 	goto again;
 
	return 0;
}


int main(int argc, char **argv)
{
	int a;

#ifdef WIN32
	SERVICE_TABLE_ENTRY	dispatchTable[]={{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain}, {NULL, NULL}};
	if ( argc==1 ) {
		if ( !StartServiceCtrlDispatcher(dispatchTable) ) {
			globErr=GetLastError();
			/*LPTSTR	pszError=CErrorContext::LoadWin32ErrorString(globErr);
			::CharToOem(pszError, pszError);
			_tprintf(_T("StartServiceCtrlDispatcher failed: %u"), globErr);
			if ( pszError!=NULL ) {
				_tprintf(_T(": %s\r\n"), pszError);
				::LocalFree(pszError);
			}
			else
				_tprintf(_T("\r\n"));*/
			AddToMessageLog("StartServiceCtrlDispatcher failed", 0);
		}
	}
	consoleApp=1;
#endif
	parse_command_line(argc,argv);
	
	a=server();
	free_all_memory();
	check_memory_leaks();
	return a;
}
