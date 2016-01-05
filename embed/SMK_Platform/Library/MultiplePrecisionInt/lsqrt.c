/*		lsqrt.c		*/
#include "longint.h"

/* square root */

LINT lsqrt(LINT a)
{
	int c, i, xl;
	LINT aa, dm, w, x, z, z1;

	x.len = x.sign = 0;
	if(a.sign == -1)
	{
		fprintf(stderr, "Error : a < 0  in lsqrt()\n");
		return x;
	}
	if(a.len == 0)	return x;
	if(a.len == 1)
	{
		x.len = 1;
		x.num[1] = (int)sqrt((double)a.num[1]);
		return x;
	}
	if(a.len == 2)
	{
		x.len = 1;
		x.num[1] = (int)sqrt((double)(a.num[2] * BASE + a.num[1]));
		return x;
	}
	x.len = (a.len + 1) / 2;
	xl = z.len = x.len;
	for(i = 1; i <= xl; i++)	x.num[i] = z.num[i] = 0;
	i = (xl - 1) * 2 + 1;
	if (a.len % 2 == 1)	c = a.num[i];
	else				c = a.num[i] + a.num[i + 1] * BASE;
	z.num[xl] = (x.num[xl] = (int)sqrt((double)c)) * 2;
	aa = lsub(a, lsqr(x));
	xl--;
	while(1)
	{
		z1 = ldivide(aa, z, &dm);
		if(xl > 1)	for(i = 1; i < xl; i++)	z1.num[i] = 0;
		if(z1.len > xl)
		{
			z1.num[xl] = BASE1;
			z1.len = xl;
		}
		while(1)
		{
			if(z.len >= z1.len)	dm = aux1(z, z1);
			else				dm = aux1(z1, z);
			w = lmul(dm, z1);
			if(lcmp(w, aa) <= 0)	break;
			z1.num[xl]--;
		}
		x.num[xl] = z1.num[xl];
		if(xl == 1)	break;
		if(dm.len >= z1.len)	z = aux1(dm, z1);
		else					z = aux1(z1, dm);
		aa = alsub(aa, w);
		xl--;
	}
	return x;
}
