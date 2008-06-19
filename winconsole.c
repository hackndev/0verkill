/* CONSOLE INTERFACE FOR WINDOWS */
#include "cfg.h"
#include "console.h"
#include "error.h"

int console_ok=1;

//#define STD_CONSOLE
//#define RAW_MIN_CONSOLE
#define RAW_CONSOLE
#define RAW_CONSOLE_DIRTY
/* allow max 6 character holes when printing blocks of dirty characters (RAW_CONSOLE_DIRTY defined) */
#define MAX_SURPLUS	6

#ifdef RAW_CONSOLE_DIRTY
	#ifndef RAW_CONSOLE
		#error RAW_CONSOLE_DIRTY without RAW_CONSOLE
	#endif
#endif

HANDLE	hMyConsole;

#define		DEF_ATTR	(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
char		curattr=DEF_ATTR,
			color=(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE),
			bkcolor=0,
			hilight=0,
			bkhilight=0;
#ifdef STD_CONSOLE
	#define		generate_attributes()	{	curattr=translate(color)+bkcolor*0x10+hilight*FOREGROUND_INTENSITY+bkhilight*BACKGROUND_INTENSITY; \
										SetConsoleTextAttribute(hMyConsole, curattr); }
#else
	#define		generate_attributes()	curattr=translate(color)+bkcolor*0x10+hilight*FOREGROUND_INTENSITY+bkhilight*BACKGROUND_INTENSITY
#endif
#ifdef	RAW_MIN_CONSOLE
	COORD		curpos;
#endif
#ifdef	RAW_CONSOLE
	char	*screenC=NULL;
	WORD	*screenA=NULL;
	#ifdef RAW_CONSOLE_DIRTY
		unsigned char	*dirtyChars=NULL,
						*dirtyLines=NULL;
		int		*dirtyBlockStart=NULL;
	#endif
	int		curpos,
			width,
			height;
#endif

void c_init(int w, int h) {
	COORD	crd;
	SMALL_RECT	sr;

	hMyConsole=GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleMode(hMyConsole, 0);

#ifdef RAW_MIN_CONSOLE
	curpos.X=0;
	curpos.Y=0;
#endif
#ifdef RAW_CONSOLE
	curpos=0;
#endif

	if ( w==-1 ) {
		CONSOLE_SCREEN_BUFFER_INFO	sbi;
		GetConsoleScreenBufferInfo(hMyConsole, &sbi);
		w=sbi.srWindow.Right-sbi.srWindow.Left+1;
		h=sbi.srWindow.Bottom-sbi.srWindow.Top+1;
	}
#ifdef RAW_CONSOLE
	width=w;
	height=h;
	if ( !(screenC=mem_alloc(w*h)) )
		{exit(1);}
	if ( !(screenA=mem_alloc(sizeof(SHORT)*w*h)) )
		{exit(1);}
	memset(screenC, 0, w*h);
	memset(screenA, 0, w*h*sizeof(SHORT));
	#ifdef RAW_CONSOLE_DIRTY
		if ( !(dirtyChars=mem_alloc(w*h)) )
			{exit(1);}
		if ( !(dirtyLines=mem_alloc(h)) )
			{exit(1);}
		if ( !(dirtyBlockStart=mem_alloc(h*sizeof(int))) )
			{exit(1);}
		memset(dirtyChars, 1, w*h);
		memset(dirtyLines, 1, h);
		memset(dirtyBlockStart, 0, h*sizeof(int));
	#endif
#endif

	crd.X=w;
	crd.Y=h;

	SetConsoleScreenBufferSize(hMyConsole, crd);
	crd=GetLargestConsoleWindowSize(hMyConsole);
	sr.Left=0;
	sr.Top=0;
	sr.Right=min(crd.X-1, w-1);
	sr.Bottom=min(crd.Y-1, h-1);
	SetConsoleWindowInfo(hMyConsole, TRUE, &sr);
	kbd_init();
	c_cls();
}

void c_shutdown(void) {
	kbd_close();
#ifdef RAW_CONSOLE
	if ( screenC ) {
		mem_free(screenC);
		screenC=NULL;
	}
	if ( screenA ) {
		mem_free(screenA);
		screenA=NULL;
	}
	#ifdef RAW_CONSOLE_DIRTY
		if ( dirtyChars ) {
			mem_free(dirtyChars);
			dirtyChars=NULL;
		}
		if ( dirtyLines ) {
			mem_free(dirtyLines);
			dirtyLines=NULL;
		}
		if ( dirtyBlockStart ) {
			mem_free(dirtyBlockStart);
			dirtyBlockStart=NULL;
		}
	#endif
#endif
	c_cursor(1);
}

void c_cls(void) {
	// clears the screen:
#ifndef RAW_CONSOLE
	COORD	coord;
	DWORD	written;
	int		x,
			y;
	c_get_size(&x, &y);
	coord.X=coord.Y=0;
	FillConsoleOutputCharacter(hMyConsole, 0, x*y, coord, &written);
	FillConsoleOutputAttribute(hMyConsole, 0, x*y, coord, &written);
#else
	memset(screenC, 0, width*height);
	memset(screenA, 0, width*height);
	#ifdef RAW_CONSOLE_DIRTY
		memset(dirtyChars, 1, width*height);
		memset(dirtyLines, 1, height);
		memset(dirtyBlockStart, 0xFF, height*sizeof(int));
	#endif
#endif
}

void c_print(char *text) {
#ifdef RAW_CONSOLE
	int	firstNonDirty=-1;
	while( *text ) {
		if ( curpos>width*height )
			return;
		#ifdef RAW_CONSOLE_DIRTY
			if ( dirtyChars[curpos]==0 ) {
				if ( firstNonDirty!=-1 )
					firstNonDirty=curpos;
				else
					dirtyChars[curpos]=-1;
			}
			else if ( firstNonDirty!=-1 ) {
				dirtyChars[firstNonDirty]=curpos-firstNonDirty;
				firstNonDirty=-1;
			}
		#endif
		screenA[curpos]=curattr;//curpos/2;
		screenC[curpos++]=*(text++);//'0'+((( curpos%2 )? curpos/2 : curpos/20)%10);
	}
	#ifdef RAW_CONSOLE_DIRTY
		if ( firstNonDirty!=-1 )
			dirtyChars[firstNonDirty]=curpos-firstNonDirty;
		dirtyBlockStart[curpos/width]=-1;
		dirtyLines[curpos/width]=1;
	#endif
#endif
#ifdef STD_CONSOLE
	DWORD	written;
	WriteConsole(hMyConsole, text, strlen(text), &written, NULL);
#endif
#ifdef RAW_MIN_CONSOLE
	DWORD	wr,
			len=strlen(text);
	WriteConsoleOutputCharacter(hMyConsole, text, len, curpos, &wr);
	FillConsoleOutputAttribute(hMyConsole, curattr, len, curpos, &wr);
	curpos.X+=(SHORT)len;
#endif
}

void c_putc(char c) {
#ifdef RAW_CONSOLE
	if ( curpos>width*height )
		return;
	#ifdef RAW_CONSOLE_DIRTY
		if ( dirtyChars[curpos]==0 ) {
			if ( dirtyBlockStart[curpos/width]==-1 ) {
				dirtyBlockStart[curpos/width]=curpos%width;
				dirtyChars[curpos]=1;
				dirtyLines[curpos/width]=1;
			}
			else if ( dirtyBlockStart[curpos/width]+dirtyChars[dirtyBlockStart[curpos/width]]==curpos%width ) {
				dirtyChars[curpos]=-1;
				if ( ++dirtyChars[dirtyBlockStart[curpos/width]]==254 )
					dirtyBlockStart[curpos/width]=-1;
			}
			else {
				dirtyBlockStart[curpos/width]=curpos%width;
				dirtyChars[curpos]=1;
			}
		};
	#endif
	screenA[curpos]=curattr;//curpos/2;
	screenC[curpos++]=c;//'0'+((( curpos%2 )? curpos/2 : curpos/20)%10;
#endif
#ifdef STD_CONSOLE
	DWORD	written;
	WriteConsole(hMyConsole, &c, sizeof(c), &written, NULL);
#endif
#ifdef RAW_MIN_CONSOLE
	DWORD	wr;
	WriteConsoleOutputCharacter(hMyConsole, &c, 1, curpos, &wr);
	FillConsoleOutputAttribute(hMyConsole, curattr, 1, curpos, &wr);
	++curpos.X;
#endif
}

void c_goto(int x, int y) {
#ifdef STD_CONSOLE
	COORD	coord;
	coord.X=x;
	coord.Y=y;
	SetConsoleCursorPosition(hMyConsole, coord);
#endif
#ifdef RAW_MIN_CONSOLE
	curpos.X=x;
	curpos.Y=y;
#endif
#ifdef RAW_CONSOLE
	curpos=x+y*width;
#endif
}

void c_clear(int x1,int y1,int x2, int y2) {
#ifndef RAW_CONSOLE
	COORD	coord;
	DWORD	written;
	int		y,
			w=x2-x1;
	if ( w<=0 )
		return;
	coord.X=x1;
	for(y=y1; y<=y2; y++) {
		coord.Y=y;
		FillConsoleOutputCharacter(hMyConsole, 0, w, coord, &written);
		FillConsoleOutputAttribute(hMyConsole, 0, w, coord, &written);
	}
#else
	int	y;
	if ( x1>=x2 )
		return;
	for(y=y1; y<=y2; y++) {
		memset(screenC+x1+y*width, 0, x2-x1);
		memset(screenA+x1+y*width, DEF_ATTR, x2-x1);
		memset(dirtyChars+x1+y*width, 1, x2-x1);
	}
	#ifdef RAW_CONSOLE_DIRTY
		memset(dirtyLines+y1, 1, y2-y1);
		memset(dirtyBlockStart+y1, 0xFF, (y2-y1)*sizeof(int));
	#endif
#endif
}

void c_cursor(int c) {
	CONSOLE_CURSOR_INFO	ci;
	GetConsoleCursorInfo(hMyConsole, &ci);
	ci.bVisible=( c==C_NORMAL );
	SetConsoleCursorInfo(hMyConsole, &ci);
}

void c_bell(void) {
	MessageBeep(0xFFFFFFFF);
}

int translate(int a) {
	int	ma=a;
	/*switch(a&0x7) {
	case 0:
		ma=0;
		break;
	case 1:
		ma=FOREGROUND_RED;
		break;
	case 2:
		ma=FOREGROUND_GREEN;
		break;
	case 3:
		ma=FOREGROUND_RED|FOREGROUND_GREEN;
		break;
	case 4:
		ma=FOREGROUND_BLUE;
		break;
	case 5:
		ma=FOREGROUND_BLUE|FOREGROUND_RED;
		break;
	case 6:
		ma=FOREGROUND_BLUE|FOREGROUND_GREEN;
		break;
	case 7:
		ma=FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN;
		break;
	}*/
	ma=(a&0xFA)+0x4*(a&0x1)+(( a&0x4 )? 1 : 0);
	return ma;
}

/* set complete foreground color */
void c_setcolor(unsigned char a) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (a&0xF)+(ci.wAttributes&~0x000F));*/
	//translate(&a);
	color=a&0x7;
	hilight=( a>>3 ) ? 1 : 0;
	generate_attributes();
}

/* set complete foreground color and background color */
void c_setcolor_bg(unsigned char fg,unsigned char bg) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (fg&0xF)+((bg&0xF)<<8)+(ci.wAttributes&~0x00FF));*/
	//translate(&fg);
	//translate(&bg);
	//curattr=(curattr&~0xFF)|(fg&0xF)|((bg&0xF)<<8);
	fg&=0xF;
	if (!(fg>>3)&&!bg) {
		color=fg&0x7;
		hilight=( fg>>3 ) ? 1 : 0;
	}
	else {
		color=fg&0x7;
		bkcolor=bg&0x7;
		hilight=( fg>>3 ) ? 1 : 0;
		//bkhilight=( bg>>3 ) ? 1 : 0;
	}
	generate_attributes();
}

void c_setcolor_3b(unsigned char a) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (a&0x7)+(ci.wAttributes&~0x0007));*/
	//translate(&a);
	color=a&0x7;
	generate_attributes();
}

void c_setcolor_3b_bg(unsigned char fg,unsigned char bg) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (fg&0x7)+((bg&0x7)<<8)+(ci.wAttributes&~0x0077))*/;
	//translate(&fg);
	//translate(&bg);
	color=fg&0x7;
	bkcolor=bg&0x7;
	generate_attributes();
}

/* set highlight color */
void c_sethlt(unsigned char a) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (a ? FOREGROUND_INTENSITY : 0)+(ci.wAttributes&~FOREGROUND_INTENSITY));*/
	hilight=a ? 1 : 0;
	generate_attributes();
}

/* set highlight and background color */
void c_sethlt_bg(unsigned char hlt,unsigned char bg) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, (hlt ? BACKGROUND_INTENSITY : 0)+((bg&0xF)<<8)+(ci.wAttributes&~0x00F0));*/
	//translate(&bg);
	bkcolor=bg&0x7;
	hilight=hlt ? 1 : 0;
	generate_attributes();
}

/* set background color */
void c_setbgcolor(unsigned char a) {
	/*CONSOLE_SCREEN_BUFFER_INFO	ci;
	GetConsoleScreenBufferInfo(hMyConsole, &ci);
	SetConsoleTextAttribute(hMyConsole, ((a&0xF)<<8)+(ci.wAttributes&~0x00F0));*/
	//translate(&a);
	//bkhilight=( a>>3 ) ? 1 : 0;
	bkcolor=a&0x7;
	generate_attributes();
}

void c_refresh(void) {
#ifdef STD_CONSOLE
	FlushFileBuffers(hMyConsole);
#endif
#ifdef RAW_CONSOLE
	#ifndef RAW_CONSOLE_DIRTY
		COORD	coord;
		DWORD	written;
		coord.X=coord.Y=0;
		WriteConsoleOutputCharacter(hMyConsole, screenC, width*height, coord, &written);
		WriteConsoleOutputAttribute(hMyConsole, screenA, width*height, coord, &written);
	#else
		DWORD	written;
		int		chr,
				base,
				cnt=0,
				sur;
		COORD	coord;
		for(coord.Y=0; coord.Y<height; coord.Y++) if ( dirtyLines[coord.Y] ) {
			base=((DWORD)coord.Y)*width;
			sur=0;
			for(chr=0; chr<width; chr++) {
				if ( dirtyChars[base+chr] ) {
					if ( sur ) {
						cnt+=sur;
						sur=0;
					};
					if ( dirtyChars[base+chr]==-1 )
						dirtyChars[base+chr]=1;
					cnt+=dirtyChars[base+chr];
					chr+=dirtyChars[base+chr]-1;	// -1 ... ++
				}
				else if ( cnt ) {
					if ( ++sur>1 ) {
						coord.X=chr-(cnt+sur-1);
						WriteConsoleOutputCharacter(hMyConsole, screenC+base+coord.X, cnt, coord, &written);
						WriteConsoleOutputAttribute(hMyConsole, screenA+base+coord.X, cnt, coord, &written);
						cnt=0;
						sur=0;
					}
				}
			}
			if ( cnt ) {
				coord.X=chr-(cnt+sur);
				WriteConsoleOutputCharacter(hMyConsole, screenC+base+coord.X, cnt, coord, &written);
				WriteConsoleOutputAttribute(hMyConsole, screenA+base+coord.X, cnt, coord, &written);
				cnt=0;
			}
		}
		memset(dirtyChars, 0, width*height);
		memset(dirtyLines, 0, height);
		memset(dirtyBlockStart, 0xFF, height*sizeof(int));
	#endif
#endif
}

void c_get_size(int *x,int *y) {
	CONSOLE_SCREEN_BUFFER_INFO	sbi;
	if ( !hMyConsole )
		hMyConsole=GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hMyConsole, &sbi);
	*x=sbi.srWindow.Right-sbi.srWindow.Left+1;
	*y=sbi.srWindow.Bottom-sbi.srWindow.Top+1;
}

int c_pressed(int k) {
	return kbd_is_pressed(k);
}

int c_was_pressed(int k) {
	return kbd_was_pressed(k);
}

void c_wait_for_key(void) {
	kbd_wait_for_key();
}

void c_update_kbd(void) {
	kbd_update();
}
				
