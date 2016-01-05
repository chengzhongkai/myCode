/*		llcm.c		*/
#include "longint.h"

LINT llcm(LINT a, LINT b)
{
	LINT x, w;

	x.len = 0;
	if(a.sign < 0 || b.sign < 0 || a.len == 0 || b.len == 0)
		fprintf(stderr, "Error : illegal parameter  in llcm()\n");
	else	x = ldivide(lmul(a, b), lgcd(a, b), &w);
	return x;
}
