/*		lkaijo.c		*/
#include "longint.h"

LINT lkaijo(int n)
{
	long j, t;
	int i;
	LINT x;

	x.sign = 0;
	if(n < 0)
	{
		fprintf(stderr, "Error : n < 0  in lkaijo()\n");
		x.len = 0;
		return x;
	}
	if(n > 449)
	{
		fprintf(stderr, "Error : Too large n  in lkaijo()\n");
		x.len = MAXLEN - 1;
		for(i = 1; i <= x.len; i++)	x.num[i] = BASE1;
		return x;
	}
	x = lset(1);
	for(j = 2; j <= n; j++)
	{
		t = 0;
		for(i = 1; i <= x.len; i++)
		{
			t += j * x.num[i];
			x.num[i] = t % BASE;
			t /= BASE;
		}
		while(t != 0)
		{
			x.num[++x.len] = t % BASE;
			t /= BASE;
		}
	}
	return x;
}
