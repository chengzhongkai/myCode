/*		lpwr.c		*/
/*    original     made by Tomy                              */
/*                 revised by Mr.Eiichi Shimada   2001/12/28 */

#include "longint.h"

/* power LINT ^ int */

LINT lpwr(LINT a, int n)
{
	int r;
	LINT x;

	x.len = x.sign = 0;
	if(n < 0)
	{
		fprintf(stderr, "Error : Illegal parameter (n < 0)  in spwr()\n");
		return x;
	}
	x.len = 1;
	x.num[1] = 1;
	if(n == 0)	return x;
	if(a.len * n > MAXLEN)
	{
		fprintf(stderr, "Error : Too large  in lpwr()\n");
		return x;
	}
	for(int i=0;i<n;i++)
	{
		x=lmul(x,a);
	}
	return x;
}
