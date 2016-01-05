/*		lcmp.c		*/
#include "longint.h"

/* compare |LINT| vs |LINT| */

int lcmp(LINT a, LINT b)
{
	int i;

	if(a.len > b.len)	return 1;
	if(b.len > a.len)	return -1;
	i = a.len;
	a.num[0] = 0;
	b.num[0] = 1;
	while(a.num[i] == b.num[i])	i--;
	if(i == 0)	return 0;
	if(a.num[i] > b.num[i])	return 1;
	return -1;
}
