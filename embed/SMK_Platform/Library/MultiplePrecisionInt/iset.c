/*		iset.c		*/
#include "longint.h"

/* setting LINT by int */

int iset(LINT a)
{
	int n;

	if(a.len == 0)	return 0;
	if(a.len == 1)
	{
		if(a.sign == 0)	return a.num[1];
		else			return - a.num[1];
	}
	if(a.len == 2)
	{
		n = a.num[2] * BASE + a.num[1];
		if(a.sign == 0)	return n;
		else			return -n;
	}
	return 0;
}
