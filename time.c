#ifdef __EMX__
	#include <stdlib.h>
#endif
#ifndef WIN32
	#include <sys/time.h>
	#include <sys/types.h>
	#include <unistd.h>
#else
	#include <time.h>
#endif
#include <string.h>

#ifdef HAVE_SYS_SELECT_H
	#include <sys/select.h>
#endif
#include "cfg.h"

/* gets current time in microseconds */
unsigned long_long get_time(void)
{
#ifndef WIN32
	struct timeval tv;
	struct timezone tz;

	memset(&tz,0,sizeof(tz));
	gettimeofday(&tv,&tz);
	return ((unsigned long_long)tv.tv_sec)*1000000+(unsigned long_long)tv.tv_usec;
#else
	return ((unsigned long_long)GetTickCount())*1000UL;
#endif
}


/* waits until time t passes */
void sleep_until(unsigned long_long t)
{
	unsigned long_long u=get_time();


	if (u>=t) return;
	t-=u;
#if defined(WIN32)
	Sleep(t/1000);
#elif defined(__EMX__)
	_sleep2(t/1000);
#else
	{
		struct timeval tv;

		tv.tv_sec=0;
		tv.tv_usec=t;
		select(0,NULL,NULL,NULL,&tv);
	}
#endif
}


/* waits time t microseconds */
void my_sleep(unsigned long_long t)
{
#if defined(WIN32)
	Sleep(t/1000);
#elif defined(__EMX__)
	_sleep2(t/1000);
#else
	{
		struct timeval tv;

		tv.tv_sec=0;
		tv.tv_usec=t;
		select(0,NULL,NULL,NULL,&tv);
	}
#endif
}

