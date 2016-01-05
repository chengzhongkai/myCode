/*		llgcd		*/
#include "longint.h"

LINT llgcd(LINT *a, int n)
{
	LINT gcd, w;
	int j;

	gcd.sign = gcd.len = 0;
	for(j = 0; j < n; j++)
	{
		if(a[j].len > 0)
		{
			gcd = a[j];
			gcd.sign = 0;
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
			gcd = lgcd(gcd, w);
		}
	}
	return gcd;
}
