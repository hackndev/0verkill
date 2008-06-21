#ifndef __MY_MATH_H
#define __MY_MATH_H

#include "cfg.h"

#define PREC 10  /* fixpoint arithmetics precision */


#define PREC2 (1<<PREC)
#define LOW_BITS (PREC2-1)

#define float2double(a) ((int)((a)*PREC2))
#define int2double(a) ((a)<<PREC)
#define double2int(a) ((a)>>PREC)
/* int and (int a,int b) */
#define my_and(a,b) ((a)&((b<<PREC)+LOW_BITS))
#define mul(a,b) ((int)(((long_long)(a)*(b))>>PREC))
/* int add_int (int a,int b) */
#define add_int(a,b) ((a)+((b)<<PREC))
/* int mul_int (int a,int b) */
#define mul_int(a,b) ((int)(((long_long)(a)*((b)<<PREC))>>PREC))
/* int sub_int (int a,int b) */
#define sub_int(a,b) ((a)-((b)<<PREC))
/* int sub_int (int a,int b) */
#define sub_from_int(a,b) (((a)<<PREC)-(b))
#define my_abs(a) ((a)>0?(a):-(a))
#define my_sgn(a) ((a)>0?1:((a)<0?-1:0))
/* int round_up(int a) */
#define round_up(a) (((a)+LOW_BITS)>>PREC)

/* "min" and "max" might already been defined on Win32 */
#ifndef WIN32
	#define max(a,b) ((a)>=(b)?(a):(b))
	#define min(a,b) ((a)<=(b)?(a):(b))
#else
	#define M_PI 3.1415926535897932384626433832795L
#endif

#endif
