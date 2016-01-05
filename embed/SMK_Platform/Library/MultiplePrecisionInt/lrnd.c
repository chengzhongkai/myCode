/*		lrnd.c		*/
#include "longint.h"

LINT lrnd(int len)
{
	LINT a;

	a.sign = 0;
	a.len = len;
	while(len)	a.num[len--] = urnd1() * BASE;
	while(a.num[a.len] == 0)	a.len--;
	a.num[0] = 0;
	return a;
}
