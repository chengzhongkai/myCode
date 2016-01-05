/*		ldown.c		*/
#include "longint.h"

void ldown(LINT *a, int n)
{
	int i;

	if(a->len == 0 || n == 0)	return;
	if(n < 0)	lup(a, n);
	else if(a->len <= n)	a->len = a->sign = 0;
	else
	{
		a->len -= n;
		for(i = 1; i <= a->len; i++) a->num[i] = a->num[i + n];
	}
	return;
}
