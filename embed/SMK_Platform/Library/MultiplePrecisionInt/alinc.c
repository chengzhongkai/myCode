/*		alinc.c		*/
#include "longint.h"

void alinc(LINT *a, int n)
{
	int i, *p, t;

	if(n < 0 || n >= BASE)
	{
		fprintf(stderr, "Error : illegal parameter input in alinc()\n");
		return;
	}
	if(n == 0)	return;
	if(a->len == 0)
	{
		*a = lset(n);
		return;
	}
	p = a->num + 1;
	for(i = 1, t = 0; i <= 3; i++)
	{
		t += (*p + n % BASE);
		*p++ = t % BASE;
		if(t < BASE)	t = 0;
		else			t = 1;
		n /= BASE;
		if(n == 0)	break;
	}
	if(t == 0)
	{
		p--;
		if(a->len < p - a->num)	a->len = p - a->num;
		return;
	}
	for(;;)
	{
		(*p)++;
		if(*p < BASE)	break;
		*p++ = 0;
	}

	a->len = p - a->num;
	if(a->len >= MAXLEN)	fprintf(stderr, "Error : Overflow  in alinc()\n");
	return;
}
