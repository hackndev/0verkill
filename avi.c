#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "cfg.h"
#include "console.h"
#include "blit.h"
#include "time.h"
#include "help.h"
#include "error.h"

int TRI_D_ON=0;
int tri_d=0;

unsigned char line[16384];
/* For loading lines from ASCII files */
unsigned char *lptr;
/* Used in conjunction with "line" variable */
unsigned char *pos=DUMMY;
/* Pointer to a cube of chars containing all the positions of the avi animation
 * The char and attributre are in a consecutive pair, first comes char, then
 * comes attr.*/
int n_pos=0;
/* Number of valid positions in the animation */
int left=0,right=0,top=0,bottom=0;
/* left: x coord of the leftmost char
 * right: x coord of the rightmost char +1
 * top: y coord of the topmost char
 * bottom: y coord of the bottommost char +1
 */
int dummy;
/* For testing purpose only */
int *sequence=DUMMY;
/* Pointer where the sequence of which positions are played after which is
 * stored */
int sequence_size;
/* Number of pos entries in the sequence. If the positions should be played
 * 1,2,3,4,5,5,5,5,4,4,3,3,2,2,1,1, then sequence contains these numbers and
 * sequence_size is 16. */
FILE *f;
/* File from which the avi is loaded and into which it is written */
int cursor_x, cursor_y, cursor_pos;
int window_size_x, window_size_y;
volatile int resize_needed_flag;
unsigned char char_block[]="  `~1!2@3#4$5%6^7&8*9(0)-_=+\\|qQwWeErRtTyYuUiIoOpP[{]}aAsSdDfFgGhHjJkKlL"
";:'\"zZxXcCvVbBnNmM,<.>/?";
int help=0;
int mode=0; /* 0: command mode, 1: drawing mode */
int hold_mode=0;
int anim=0;
int last_color, last_char;

#define MY_ERROR {ERROR("Syntax error of input file\n");EXIT(-1);}

void load_line(void)
{
	again:
	lptr=line;
	if (!(fgets(line,sizeof(line),f))){
		*line=0;
		return;
	}
	if (*lptr=='#') goto again;
}

int read_number(void)
{
	int retval;
	retval=strtol(lptr,(char **)&lptr,0);
	return retval;
}

void add_pos(int how_much)
{
	n_pos+=how_much;
	pos=mem_realloc(pos,(right-left)*(bottom-top)*2*n_pos);
	memset(pos+(right-left)*(bottom-top)*2*(n_pos-how_much),0,2*(right-left)*(bottom-top)*how_much);
}

void resize(int new_left, int new_right, int new_top, int new_bottom);

/* If do_resize is 1, then out-of-range is solved as a resize. If do_resize is a
 * zero, then a NULL value is returned. */
unsigned char *get_pointer(int x, int y, int p, int do_resize)
{
	int offset;
	int flag=0;
	int new_left=left,new_right=right,new_top=top,new_bottom=bottom;

	if (p>=n_pos){
		add_pos(p-n_pos+1);
	}
	if (x<left){
		new_left=x;
		flag=1;
		}
	if (x>=right){
		new_right=x+1;
		flag=1;
		}
	if (y<top){
		new_top=y;
		flag=1;
		}
	if (y>=bottom){
		new_bottom=y+1;
		flag=1;
		}
	if (flag){
		if (do_resize)
			resize(new_left,new_right,new_top,new_bottom);
		else
			return NULL;
	}
	offset=p;
	offset*=(bottom-top);
	offset+=y-top;
	offset*=(right-left);
	offset+=x-left;
	return offset*2+pos;
}

void resize(int new_left, int new_right, int new_top, int new_bottom)
{
	unsigned char *new_pointer;
	int new_length=(new_right-new_left)*2*(new_bottom-new_top)*n_pos;
	int y,p;
	unsigned char *lp;
	int offset;

	new_pointer=mem_alloc(new_length);
	memset(new_pointer,0,new_length);
	for (p=0;p<n_pos;p++){
		for (y=top;y<bottom;y++){
			lp=get_pointer(left,y,p,1);
			offset=p;
			offset*=(new_bottom-new_top);
			offset+=y-new_top;
			offset*=(new_right-new_left);
			offset+=left-new_left;
			offset*=2;
			memcpy(new_pointer+offset,lp, 2*(right-left));
		}
	}
	left=new_left;
	right=new_right;
	top=new_top;
	bottom=new_bottom;
	mem_free(pos);
	pos=new_pointer;
}

void put_element(int x,int y,int pos, int chr)
{
	unsigned char *ptr;
	
	ptr=get_pointer(x,y,pos,1);
	ptr[0]=chr;
}

void put_attribute(int x,int y,int pos, int chr)
{
	unsigned char *ptr;
	
	ptr=get_pointer(x,y,pos,1);
	if ((chr<'0'||chr>'9')&&(chr<'a'||chr>'z')&&(chr<'A'||chr>'Z'))chr=0;
	else{
		if (chr>='0'&&chr<='9'){
			chr-='0';
		}else if (chr>='a'&&chr<='z'){
			chr=chr-'a'+10;
		}else if (chr>='A'&&chr<='Z'){
			chr=chr-'A'+10;
		}
	}
	ptr[1]=chr;
}

void load_avi(unsigned char *filename)
{
	int xoffset=0, yoffset=-1,pos=-1,x;

	f=fopen(filename,"r");
	if (!f){
		sequence=mem_realloc(sequence,sizeof(*sequence));
		*sequence=0;
		sequence_size=1;
		return;
	}
	new_line:
	load_line();
	switch (*lptr++){
		case 'p':
		pos++;
		xoffset=read_number();
		if ((*lptr++)!=',') MY_ERROR;
		yoffset=read_number();
		yoffset--;
		break;
		
		case 0: goto done;
		
		case 'l':
		yoffset++;
		x=xoffset;
		while((*lptr)&&(*lptr!='\n')){
			put_element(x,yoffset,pos,*lptr);
			lptr++;
			x++;
		}
		break;
		
		case 'a':
		x=xoffset;
		while(*lptr&&*lptr!='\n'){
			put_attribute(x,yoffset,pos,*lptr);
			lptr++;
			x++;
		}
		break;
		
		case 's':
		new_number:
		sequence_size++;
		sequence=mem_realloc(sequence,sequence_size*sizeof(*sequence));
		sequence[sequence_size-1]=read_number();
		if ((*lptr++)==',') goto new_number;
		break;
		
	}
	goto new_line;
	done:
	fclose(f);
}



void normalize_transparency(void)
{
	int length=(right-left)*(bottom-top)*n_pos;
	unsigned char *ptr=pos;
	for (;length;length--,ptr+=2){

		if ((!ptr[1])||(!(*ptr)))*ptr=32;

	}

}

void save_avi(unsigned char *filename)
{
	int pos,y,x,t;
	unsigned char *char_pointer,*attr_pointer;
	int nonempty;
	int state;
	int l;

	normalize_transparency();
	f=fopen(filename,"w");
	if (!f)	return;

	for (pos=0;pos<n_pos;pos++){
		state=0;
		t=top;

		/* skip heading empty lines */
		for (y=top;y<bottom;y++){
			char_pointer=get_pointer(left,y,pos,0);
			nonempty=right-left;
			while((nonempty>0)&&(!char_pointer[2*nonempty-1]))
				nonempty--;
			if (!nonempty){
				state++;
				continue;
			}else{
				for (;state;state--)t++;
				state=0;
				break;
			}
		}

		{
			unsigned char *c;
			int a;

			l=right;
			for (y=t;y<bottom;y++)
			{
				c=get_pointer(left,y,pos,0);
				c++;
				for (a=0;a<right-left;a++)
					if (c[2*a])break;
				if (a+left<l)l=a+left;
			}
			
		}
		fprintf(f,"#%d\np%d,%d\n",pos,l,t);

		for (y=t;y<bottom;y++){
			char_pointer=get_pointer(l,y,pos,0);
			attr_pointer=char_pointer+1;
			nonempty=right-l;
			while((nonempty>0)&&(!char_pointer[2*nonempty-1]))
				nonempty--;
			if (!nonempty){
				state++;
				continue;
			}else{
				for (;state;state--)
					fprintf(f,"l\na\n");
				state=0;
			}
			fprintf(f,"l");
			for (x=nonempty;x;x--){
				unsigned char c=*char_pointer;

				fputc(c,f);

				char_pointer+=2;

			}

			fprintf(f,"\na");

			for (x=nonempty;x;x--){
				unsigned char c=*attr_pointer;

				if (c<10)c+='0';else c=c-10+'a';

				fputc(c=='0'?' ':c,f);

				attr_pointer+=2;

			}

			fprintf(f,"\n");

		}

	}

	fprintf(f,"s");

	for (pos=0;pos<sequence_size;pos++){

		fprintf(f,"%d%s",sequence[pos],pos==sequence_size-1?"":",");

	}

	fprintf(f,"\n");

	fclose(f);

}

void filter_background(void)
{
	int l=SCREEN_X*SCREEN_Y;
	unsigned char *ptr=screen;
	unsigned char *ptr1=screen_a;
	for (;l;l--,ptr++,ptr1++){
		if (!*ptr1){
			*ptr=' ';
			*ptr1=75;
		}
	}
}

int get_colour(void){
	unsigned char *ptr;
	
	ptr=get_pointer(cursor_x,cursor_y,cursor_pos,1);
	return ptr[1];
}

void print_bottom_line(void)
{
	int offs=SCREEN_X*(SCREEN_Y-2);
	unsigned char txt[32];
	int x;

	memset(screen_a+offs,7,SCREEN_X);
	memset(screen+offs,' ',SCREEN_X);
	memset(screen+offs+SCREEN_X,' ',SCREEN_X);
	memset(screen_a+offs+SCREEN_X,0,SCREEN_X);
	x=0;
	print2screen(x,SCREEN_Y-2,11,mode?"DRAW":"COMMAND");x+=9;
	print2screen(x,SCREEN_Y-2,11,anim?"ANIM":"");x+=6;
	print2screen(x,SCREEN_Y-2,11,hold_mode?"HOLD":"");x+=6;
	print2screen(x,SCREEN_Y-2,get_colour(),"COLOR");x+=7;
	print2screen(x,SCREEN_Y-2,7,"POS=");x+=4;
	snprintf(txt,32,"%d",cursor_pos);
	print2screen(x,SCREEN_Y-2,11,txt);x+=strlen(txt)+2;
	print2screen(x,SCREEN_Y-2,7,"[");x+=1;
	snprintf(txt,32,"% 5d",cursor_x);
	print2screen(x,SCREEN_Y-2,11,txt);x+=5;
	print2screen(x,SCREEN_Y-2,7,",");x+=1;
	snprintf(txt,32,"% 5d",cursor_y);
	print2screen(x,SCREEN_Y-2,11,txt);x+=5;
	print2screen(x,SCREEN_Y-2,7,"]");x+=3;
	for (x=0;x<16;x++)
	{
		screen_a[offs+SCREEN_X-16+x]=x;
		screen[offs+SCREEN_X-16+x]=x<10?'0'+x:'A'+x-10;
	}

	x=0;
	print2screen(x,SCREEN_Y-1,9,"F1,O");x+=4;
	print2screen(x,SCREEN_Y-1,7,"-help");x+=6;
	print2screen(x,SCREEN_Y-1,9,"ENTER");x+=5;
	print2screen(x,SCREEN_Y-1,7,"-mode");x+=6;
	print2screen(x,SCREEN_Y-1,9,"TAB");x+=3;
	print2screen(x,SCREEN_Y-1,7,"-hold");x+=6;
	print2screen(x,SCREEN_Y-1,9,"Q");x+=1;
	print2screen(x,SCREEN_Y-1,7,"-save&end");x+=10;
	print2screen(x,SCREEN_Y-1,9,"<,>");x+=3;
	print2screen(x,SCREEN_Y-1,7,"-shift");x+=7;
	print2screen(x,SCREEN_Y-1,9,"[,]");x+=3;
	print2screen(x,SCREEN_Y-1,7,"-copy pos");x+=10;
	print2screen(x,SCREEN_Y-1,9,"-,+");x+=3;
	print2screen(x,SCREEN_Y-1,7,"-prev/next");x+=11;
}


void print_view(void)
{
	int x,y;
	int left,right,top,bottom;
	unsigned char dummy[2]={32,0};
	unsigned char *pointer;

	left=cursor_x-(window_size_x>>1);
	top=cursor_y-(window_size_y>>1);
	right=left+window_size_x;
	bottom=top+window_size_y;

	clear_screen();
	for (y=top;y<bottom;y++){
		for (x=left;x<right;x++){
			pointer=get_pointer(x,y,cursor_pos,0);
			if (!pointer) pointer=dummy;
			screen_a[(x-left)+SCREEN_X*(y-top)]=pointer[1];
			screen[(x-left)+SCREEN_X*(y-top)]=*pointer;
		}
	}
	filter_background();
	/*screen[(cursor_x-left)+SCREEN_X*(cursor_y-top)]='+';
	screen_a[(cursor_x-left)+SCREEN_X*(cursor_y-top)]=15;*/

	print_bottom_line();
	if (help)print_help_window();
	blit_screen(0);
	if (!help)
	{
		c_setcolor(15);
		c_goto((cursor_x-left),(cursor_y-top));
		c_refresh();
	}
}

void animate(void)
{
	int s;
	unsigned long_long animate_speed=1000000/36;
	
	anim=1;
	while(1){
		for (s=0;s<sequence_size;s++){
			c_update_kbd();
			if (c_pressed('h')){
				anim=0;
				return;
			}
			cursor_pos=sequence[s];
			print_view();
			sleep_until(get_time()+animate_speed);
		}
	}
	
}

void draw_colour(int colour){
	unsigned char *ptr;
	
	ptr=get_pointer(cursor_x,cursor_y,cursor_pos,1);
	ptr[1]=colour;
	last_color=colour;
}

void draw_char(int character){
	unsigned char *ptr;
	
	ptr=get_pointer(cursor_x,cursor_y,cursor_pos,1);
	ptr[0]=character;
	if (!ptr[1]) ptr[1]=7;
	last_char=character;
}

void copy_pos(int dest, int src)
{
	if (max(dest,src)>=n_pos)
		add_pos(max(dest,src)+1-n_pos);
	memcpy(pos+(right-left)*(bottom-top)*2*dest,pos+(right-left)*(bottom-top)*2*src,(right-left)*(bottom-top)*2);
}

void shift_right(int p)
{
	int y;
	unsigned char a1,a2;
	unsigned char *ptr=pos+(right-left)*(bottom-top)*2*p;

	if (right==left) return;
	for (y=bottom-top;y;y--){
		a1=ptr[(right-left)*2-1];
		a2=ptr[(right-left)*2-2];
		memmove(ptr+2,ptr,(right-left-1)*2);
		ptr[0]=a2;
		ptr[1]=a1;
		ptr+=(right-left)*2;
	}
}

void shift_left(int p)
{
	int y;
	unsigned char *ptr=pos+(right-left)*(bottom-top)*2*p,a1,a2;

	if (right==left) return;
	for (y=bottom-top;y;y--){
		a1=ptr[1];
		a2=ptr[0];
		memmove(ptr,ptr+2,(right-left-1)*2);
		ptr[(right-left)*2-1]=a1;
		ptr[(right-left)*2-2]=a2;
		ptr+=(right-left)*2;
	}
}

void loop(void)
{
	typedef unsigned long_long ttp;
	int xdelta,ydelta;
	int posdelta;
	ttp last_left=0, last_right=0, last_up=0, last_down=0, repeat,
	repeat_time=100000;
	ttp last_plus=0, last_minus=0;
	int l,r,u,d;
	int oldhelp;
	
	redraw:
	print_view();
	rescan:
	oldhelp=help;
	repeat=get_time()-repeat_time;
	c_update_kbd();
	xdelta=ydelta=posdelta=0;
	l=!!c_pressed(K_LEFT);
	r=!!c_pressed(K_RIGHT);
	u=!!c_pressed(K_UP);
	d=!!c_pressed(K_DOWN);
	if (l^r){
		if (l&&last_left<repeat){
			xdelta--;
			last_left=get_time();
		}
		if (r&&last_right<repeat){
			xdelta++;
			last_right=get_time();
		}
	}
	if (u^d){
		if (u&&last_up<repeat){
			ydelta--;
			last_up=get_time();
		}
		if (d&&last_down<repeat){
			ydelta++;
			last_down=get_time();
		}
	}
	if (c_pressed('=')&&last_plus<repeat){
		posdelta++;
		last_plus=get_time();
	}
	if (c_pressed('-')&&last_minus<repeat){
		posdelta--;
		last_minus=get_time();
	}
	if (c_was_pressed(K_TAB)){
		hold_mode^=1;
		goto redraw;
	}
	if (c_was_pressed(K_ENTER)){
		mode^=1;
		hold_mode=0;
		goto redraw;
	}
	if (c_was_pressed(K_F1))
		help^=1;
	if (mode){  /* draw */
		int a;

		for (a=0;a<((signed)sizeof(char_block))-1;a+=2){
			if (c_pressed(char_block[a])){
				if
					(c_pressed(K_LEFT_SHIFT)||c_pressed(K_RIGHT_SHIFT))
					draw_char(char_block[a+1]);
				else
					draw_char(char_block[a]);
				goto redraw;
			}
		}
	}
	else
	{
		if (c_was_pressed('o'))
			help^=1;
		if (c_was_pressed('q')) return;
		if (c_was_pressed('g')){animate();goto redraw;}
		{
			int a;
			for (a=0;a<10;a++){
				if (c_pressed('0'+a)){
					draw_colour(a);
					goto redraw;
				}
			}
		}
		{
			int a;
			for (a=0;a<6;a++){
				if (c_pressed('a'+a)){
					draw_colour(a+10);
					goto redraw;
				}
			}
		}
		if (c_was_pressed('[')){
			copy_pos(cursor_pos,(cursor_pos+n_pos-1)%n_pos);
			goto redraw;
		}
		if (c_was_pressed(']')){
			copy_pos(cursor_pos,(cursor_pos+1)%n_pos);
			goto redraw;
		}
		if (c_was_pressed(',')){
			shift_left(cursor_pos);
			goto redraw;
		}
		if (c_was_pressed('.')){
			shift_right(cursor_pos);
			goto redraw;
		}
	}
		
	if (!cursor_pos&&posdelta<0) posdelta=0;
	cursor_x+=xdelta;
	cursor_y+=ydelta;
	cursor_pos+=posdelta;
	if (hold_mode)
	{
		if (mode)
			draw_char(last_char);
		else
			draw_colour(last_color);
		goto redraw;
	}
	if (oldhelp^help)goto redraw;
	if (xdelta||ydelta||posdelta) goto redraw;
	goto rescan;
}

/* fatal signal */
void signal_handler(int signum)
{
	
	shutdown_blit();
	c_shutdown();
#ifdef HAVE_PSIGNAL
	psignal(signum,"Exiting on signal");
#else
	fprintf(stderr, "Exiting on signal: %d\n", signum);
#endif
	check_memory_leaks();
	EXIT(1);
}



int main(int argc, char **argv)
{
	if (argc<2){
		fprintf(stderr,"usage: %s foo.avi\n",argv[0]);
		return -1;
	}

	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGILL,signal_handler);
	signal(SIGABRT,signal_handler);
	signal(SIGFPE,signal_handler);
	signal(SIGSEGV,signal_handler);
	signal(SIGQUIT,signal_handler);
	signal(SIGBUS,signal_handler);

	c_get_size(&SCREEN_X,&SCREEN_Y);
	init_blit();
	c_init(SCREEN_X,SCREEN_Y);
	c_get_size(&window_size_x,&window_size_y);
	load_avi(argv[1]);
	loop();
	save_avi(argv[1]);
	c_shutdown();
	mem_free(sequence);
	mem_free(pos);
	shutdown_blit();
	check_memory_leaks();
	return 0;
}
