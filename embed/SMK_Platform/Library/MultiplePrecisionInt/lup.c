/*		lup.c		*/
#include "longint.h"

void lup(LINT *a, int n)
{
	int i, *p;

	if(a->len == 0 || n == 0)	return;
	if(n < 0)
	{
		ldown(a, n);
		return;
	}
	for(i = a->len, p = a->num + a->len + n; i >= 1; i--, p--) *p = *(p - n);
	for(i = n; i >= 1; i--)	*p-- = 0;
	a->len += n;
	return;
}
