/*		smul.c		*/
#include "longint.h"

/* multiplication LINT * int */

LINT smul(LINT a, int n)
{
	int i, t;
	LINT nn, x;

	x.len = x.sign = 0;
	if(a.len == 0 || n == 0)	return x;
	if(abs(n) >= BASE)
	{
		nn = lset(n);
		return lmul(a, nn);
	}
	if(((a.sign ==  0) && (n <  0)) ||
	   ((a.sign == -1) && (n >= 0)))	x.sign = -1;
	if(n < 0)	n = - n;
	t = 0;
	for(i = 1; i <= a.len; i++)
	{
		t += (a.num[i] * n);
		x.num[i] = t % BASE;
		t /= BASE;
	}
	x.len = a.len;
	while(t != 0)
	{
		x.len++;
		if(x.len > MAXLEN)
		{
			fprintf(stderr, "Error : Overflow  in smul()\n");
			x.len--;
			return x;
		}
		x.num[x.len] = t % BASE;
		t /= BASE;
	}
	return x;
}
