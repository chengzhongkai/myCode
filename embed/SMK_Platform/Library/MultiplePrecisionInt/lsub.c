/*		lsub.c		*/
#include "longint.h"

/* subtraction LINT - LINT */

LINT lsub(LINT a, LINT b)
{
	int s;
	LINT x;

	x.len = x.sign = 0;
	s = lcmp(a, b);
	if(s == 0 && a.sign == b.sign)	return x;
	if(a.len == 0)
	{
		b.sign = (b.sign == 0) ? -1: 0;
		return b;
	}
	if(b.len == 0)	return a;
	s = (s >= 0);
	if(a.sign == b.sign)
	{
		if(s)
		{
			x = alsub(a, b);
			x.sign = a.sign;
		}
		else
		{
			x = alsub(b, a);
			x.sign = (a.sign == -1) ? 0: -1;
		}
	}
	else
	{
		if(s)	x = aux1(a, b);
		else	x = aux1(b, a);
		x.sign = a.sign;
	}
	return x;
}
