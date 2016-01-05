/*		lswap.c		*/
#include "longint.h"

void lswap(LINT *a, LINT *b)
{
	LINT t;

	t = *a;
	*a = *b;
	*b = t;
	return;
}
