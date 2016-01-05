/*		lmulp.c		*/
#include "longint.h"

/* multiplication LINT * LINT (except sign) */
LINT lmulp(LINT a, LINT b)
{
	LINT c;
	int *anum1, *cp, *p, *q;
	int alen, blen, clen, i, j, t, u;

	c.len = c.sign = 0;
	if((alen = a.len) == 0 || (blen = b.len) == 0)	return c;
	anum1 = a.num + 1;
	if(alen == 1)	return smul(b, *anum1);
	if(blen == 1)	return smul(a, b.num[1]);
	if((clen = alen + blen) > MAXLEN)
	{
		fprintf(stderr, "Error : Overflow  in lmulp()\n");
		return c;
	}
	cp = c.num + 1;
	i = clen;
	while(i--)	*cp++ = 0;
	for(j = 1; j <= blen; j++)
	{
		if((u = b.num[j]) == 0)	continue;

		t = 0;
		p = anum1;
		cp = c.num + j;
		i = alen;
		while(i--)
		{
			*cp = (t += (*cp + u * *p++)) % BASE;
			t /= BASE;
			cp++;
		}
		*cp += t;
	}
	cp = c.num + clen;
	while(*cp == 0)	cp--;
	c.len = cp - c.num;
	return c;
}
