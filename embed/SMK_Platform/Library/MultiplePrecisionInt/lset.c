/*		lset.c		*/
#include "longint.h"

/* setting LINT by int */

LINT lset(long n)
{
	LINT a;
	int i;

	a.len = a.sign = 0;
	if(n == 0)	return a;
	if(n < 0)
	{
		a.sign = -1;
		n = - n;
	}
	i = 1;
	do
	{
		a.num[i++] = n % BASE;
		n /= BASE;
	} while (n > 0);
	a.len = i - 1;
	return a;
}
