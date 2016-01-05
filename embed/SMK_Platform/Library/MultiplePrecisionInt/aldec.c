/*		aldec.c		*/
#include "longint.h"

void aldec(LINT *a, int n)
{
	int i, *p, t;

	if(n < 0 || n >= BASE)
	{
		fprintf(stderr, "Error : illegal parameter input in aldec()\n");
		return;
	}
	if(n == 0)	return;
	if(a->len == 0)
	{
		*a = lset(- n);
		return;
	}
	p = a->num + 1;
	if(a->len == 1)
	{
		*p -= n;
		if(*p == 0)	a->len = a->sign = 0;
		else if(*p > 0)	a->sign = 0;
		else
		{
			*p += BASE;
			a->sign = -1;
		}
		return;
	}
	t = *p - n;
	i = 1;
	while((i++ < a->len) && (t < 0))
	{
		*p++ = (t += BASE);
		t = *p - 1;
	}
	if(t != 0)	*p = t;
	else
	{
		a->len--;
		while ((a->num[a->len] == 0) && a->len > 0)	a->len--;
	}
	return;
}
