/*		lprime_chk.c		*/
#include "longint.h"

/* prime check   0 : Not Prime , 1 : Prime */

int lprime_chk(LINT x)
{
	LINT i, r, w;

	if(x.len == 0 || x.sign == -1)
	{
		fprintf(stderr, "Error : Illegal parameter  in lprime_chk()\n");
		return 0;
	}
	if(x.len == 1 && x.num[1] < 4)	return 1;
	if(x.num[1] % 2 == 0)	return 0;
	i.len = 1;
	i.sign = 0;
	i.num[1] = 3;
	while(1)
	{
		w = ldivide(x, i, &r);
		if(r.len == 0)	return 0;
		if(lcmp(i, w) >= 0)	return 1;
		alinc(&i, 2);
	}
}
