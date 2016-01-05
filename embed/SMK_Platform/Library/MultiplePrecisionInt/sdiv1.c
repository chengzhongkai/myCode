/*		sdiv1.c		*/
#include "longint.h"

/* division LINT / int */

void sdiv1(LINT *a, int n, int *r)
{
	int i, t;
	LINT nn, rr;

	*r = 0;
	if(a->len == 0)	return;
	if(n == 0)
	{
		fprintf(stderr, "Error : Divide by 0  in sdiv1()\n");
		return;
	}
	if(abs(n) >= BASE)
	{
		nn = lset(n);
		*a = ldivide(*a, nn, &rr);
		*r = iset(rr);
		return;
	}
	if(n < 0)
	{
		a->sign = (a->sign == -1) ? 0: -1;
		n = - n;
	}
	t = 0;
	for(i = a->len; i >= 1; i--)
	{
		t = t * BASE + a->num[i];
		a->num[i] = t / n;
		t %= n;
	}
	*r = (a->sign == 0) ? t: - t;
	if (a->num[a->len] == 0)	a->len--;
	return;
}
