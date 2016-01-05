/*		alsub.c		*/
#include "longint.h"

/* absolute subtraction |LINT a| - |LINT b| at |a| >= |b| */

LINT alsub(LINT a, LINT b)
{
	int i, t;
	LINT x;

	x.len = x.sign = 0;
	t = lcmp(a, b);
	if(t < 0)
	{
		fprintf(stderr, "Error : a < b  in alsub()\n");
		return x;
	}
	if(t == 0)	return x;
	t = 0;
	for(i = 1; i <= b.len; i++)
	{
		t = a.num[i] - b.num[i] - t;
		if(t >= 0)
		{
			x.num[i] = t;
			t = 0;
		}
		else
		{
			x.num[i] = t + BASE;
			t = 1;
		}
	}
	while((i <= a.len) && (t != 0))
	{
		t = a.num[i] - t;
		if(t >= 0)
		{
			x.num[i] = t;
			t = 0;
		}
		else
		{
			x.num[i] = t + BASE;
			t = 1;
		}
		i++;
	}
	for( ; i <= a.len; i++)	x.num[i] = a.num[i];
	x.len = a.len;
	while (x.num[x.len] == 0)	x.len--;
	return x;
}
