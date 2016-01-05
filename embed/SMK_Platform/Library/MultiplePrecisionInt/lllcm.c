/*		lllcm.c		*/
#include "longint.h"

LINT lllcm(LINT *a, int n)
{
	LINT lcm, w, x;
	int j;

	lcm.sign = lcm.len = 0;
	for(j = 0; j < n; j++)
	{
		if(a[j].len > 0)
		{
			lcm = a[j];
			lcm.sign = 0;
			j++;
			break;
		}
	}
	for(; j < n; j++)
	{
		w = a[j];
		if(w.len > 0)
		{
			w.sign = 0;
			lcm = llcm(lcm, w);
		}
	}
	return lcm;
}
