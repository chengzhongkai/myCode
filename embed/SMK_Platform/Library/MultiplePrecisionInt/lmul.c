/*		lmul.c		*/
#include "longint.h"

/* multiplication LINT * LINT */

LINT lmul(LINT a, LINT b)
{
	LINT x;

	x = lmulp(a, b);
	if(x.len == 0 || a.sign == b.sign)	return x;
	x.sign = -1;
	return x;
}
