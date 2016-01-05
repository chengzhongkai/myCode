/*		sdiv.c		*/
#include "longint.h"

/* division LINT / int */

LINT sdiv(LINT a, int n, int *r)
{
	int i, t;
	LINT nn, rr, x;

	x.len = x.sign = *r = 0;
	if(a.len == 0)	return x;
	if(n == 0)
	{
		fprintf(stderr, "Error : Divide by 0  in sdiv()\n");
		return x;
	}
	if(abs(n) >= BASE)
	{
		nn = lset(n);
		x = ldivide(a, nn, &rr);
		*r = iset(rr);
		return x;
	}
	if(((a.sign == 0) && (n < 0)) || ((a.sign == -1) && (n > 0)))	x.sign = -1;
	if(n < 0)	n = - n;
	t = 0;
	for(i = a.len; i >= 1; i--)
	{
		t = t * BASE + a.num[i];
		x.num[i] = t / n;
		t %= n;
	}
	*r = (a.sign == 0) ? t: - t;
	x.len = a.len;
	if (x.num[a.len] == 0)	x.len--;
	return x;
}
