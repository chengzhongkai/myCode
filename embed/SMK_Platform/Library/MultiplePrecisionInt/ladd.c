/*		ladd.c		*/
#include "longint.h"

/* addition  LINT + LINT */

LINT ladd(LINT a, LINT b)
{
	LINT x;
	int s;

	if(a.len == 0)	return b;
	if(b.len == 0)	return a;
	s = (lcmp(a, b) >= 0);
	if(a.sign == b.sign)
	{
		if(s)	x = aux1(a, b);
		else	x = aux1(b, a);
		x.sign = a.sign;
	}
	else
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
	return x;
}
