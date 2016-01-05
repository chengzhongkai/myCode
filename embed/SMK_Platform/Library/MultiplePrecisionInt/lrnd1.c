/*		lrnd1.c		*/
#include "longint.h"

LINT lrnd1(LINT n)
{
	LINT a;
	int i;

	a.sign = n.sign;
	do
	{
		i = a.len = n.len;
		while(--i)	a.num[i] = urnd1() * BASE;
		i = a.len;
		do
		{
			a.num[i] = urnd1() * (n.num[i] + 1);
			if(a.num[i] < n.num[i])	break;
		} while(lcmp(a, n) >= 0);
		while(a.num[a.len] == 0)	a.len--;
	} while(a.len == 0);
	a.num[0] = 0;
	return a;
}
