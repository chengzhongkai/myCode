/*		smul1.c		*/
#include "longint.h"

void smul1(LINT *a, int n)
{
	int i, t;
	LINT nn;

	if(a->len == 0)	return;
	if(n == 0)
	{
		a->len = a->sign = 0;
		return;
	}
	if(abs(n) >= BASE)
	{
		nn = lset(n);
		*a = lmul(*a, nn);
		return;
	}
	if(n < 0)
	{
		a->sign = (a->sign == -1) ? 0: -1;
		n = - n;
	}
	t = 0;
	for(i = 1; i <= a->len; i++)
	{
		t += (a->num[i] * n);
		a->num[i] = t % BASE;
		t /= BASE;
	}
	while(t != 0)
	{
		a->len++;
		if(a->len > MAXLEN)
		{
			fprintf(stderr, "Error : Overflow  in smul()\n");
			a->len--;
			return;
		}
		a->num[a->len] = t % BASE;
		t /= BASE;
	}
	return;
}
