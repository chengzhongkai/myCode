/*		lgcd.c		*/
#include "longint.h"

/* GCD */

LINT lgcd(LINT a, LINT b)
{
	LINT w, x;

	x.len = x.sign = 0;
	if(a.sign == -1 || b.sign == -1)
	{
		fprintf(stderr, "Error : Illegal parameter  in lgcd()\n");
		return x;
	}
	do
	{
		ldivide(a, b, &x);
		a = b;
		b = x;
	} while(b.len > 0);
	return a;
}
