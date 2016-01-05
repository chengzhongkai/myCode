/*		linc.c		*/
#include "longint.h"

/* increment LINT */

void linc(LINT *a, int n)
{
	if(n == 0)	return;
	if(n <= -BASE || n >= BASE)
	{
		fprintf(stderr, "Error : illegal parameter input in linc().\n");
		return;
	}
	if(a->len == 0)
	{
		*a = lset(n);
		return;
	}
	if(a->sign == 0)
	{
		if(n > 0)	alinc(a, n);
		else		aldec(a, - n);
	}
	else
	{
		if(n <= 0)	alinc(a, - n);
		else
		{
			a->sign = 0;
			aldec(a, n);
			a->sign = (a->sign == 0) ? -1: 0;
		}
	}
	return;
}
