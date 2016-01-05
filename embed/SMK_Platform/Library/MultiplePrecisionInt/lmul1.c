/*		lmul1.c		*/
#include "longint.h"

/* multiplication LINT * LINT */

void lmul1(LINT *a, LINT b)
{
	int i, imax, j, l, t, u;
	LINT x;

	if(a->len == 0)	return;
	if(b.len == 0)
	{
		a->len = a->sign = 0;
		return;
	}
	if(a->len == 1)
	{
		if(a->sign == 0)	t = a->num[1];
		else				t = - a->num[1];
		*a = smul(b, t);
		return;
	}
	if(b.len == 1)
	{
		if(b.sign == 0)	t = b.num[1];
		else			t = - b.num[1];
		smul1(a, t);
		return;
	}
	l = a->len + b.len;
	if(l > MAXLEN)
	{
		fprintf(stderr, "Error : Overflow  in lmul()\n");
		return;
	}
	x.sign = (a->sign == b.sign) ? 0: -1;
	if(b.len > a->len)	lswap(a, &b);
	for(i = 1; i <= l; i++)	x.num[i] = 0;
	for(j = 1; j <= b.len; j++)
	{
		u = b.num[j];
		if(u != 0)
		{
			t = 0;
			imax = a->len + j - 1;
			for(i = j; i <= imax; i++)
			{
				t += (x.num[i] + u * a->num[j]);
				x.num[i] = t % BASE;
				t /= BASE;
			}
			t += x.num[i];
			while(t >= BASE)
			{
				x.num[i++] = t - BASE;
				t = x.num[i] + 1;
			}
			x.num[i] = t;
		}
	}
	if(x.num[l] == 0)	x.len = l - 1;
	else				x.len = l;
	*a = x;
	return;
}
