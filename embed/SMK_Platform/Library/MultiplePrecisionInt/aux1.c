/*		aux1.c		*/
#include "longint.h"

/* absolute addition |LINT a| + |LINT b| at |a| > |b| */

LINT aux1(LINT a, LINT b)
{
	int i, t;
	LINT x;

	a.sign = b.sign = 0;
	x.len = x.sign = 0;
	if(lcmp(a, b) < 0)
	{
		fprintf(stderr, "Error : |a| < |b|  in aux1()\n");
		return x;
	}
	t = 0;
	for(i = 1; i <= b.len; i++)
	{
		t += (a.num[i] + b.num[i]);
		if(t < BASE)
		{
			x.num[i] = t;
			t = 0;
		}
		else
		{
			x.num[i] = t - BASE;
			t = 1;
		}
	}
	while((i <= a.len) && (t != 0))
	{
		t += a.num[i];
		if(t < BASE)
		{
			x.num[i] = t;
			t = 0;
		}
		else
		{
			x.num[i] = t - BASE;
			t = 1;
		}
		i++;
	}
	for( ; i <= a.len; i++)	x.num[i] = a.num[i];
	if(t == 0)	x.len = a.len;
	else
	{
		if(a.len <= MAXLEN)
		{
			x.len = a.len + 1;
			x.num[i] = 1;
		}
		else	fprintf(stderr, "Error : Overflow  in aux1()\n");
	}
	return x;
}
