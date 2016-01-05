/*		lsqr.c		*/
#include "longint.h"

/* square (LINT ^ 2) */

LINT lsqr(LINT a)
{
	LINT c;
	int *cp, *q;
	int alen, clen, i, j, k, t, u;

	c.len = c.sign = 0;
	if((alen = a.len) == 0)	return c;
	clen = alen * 2;
	cp = c.num + 1;
	i = clen;
	while(i--)	*cp++ = 0;
	for(i = k = 1; i <= alen; i++, k += 2)
	{
		if((u = a.num[i]) == 0)	continue;

		t = *(cp = c.num + k) + u * u;
		*cp++ = t % BASE;
		t /= BASE;
		u *= 2;
		for(j = i, q = &a.num[i + 1]; j < alen; j++)
		{
			t += (*cp + u * *q++);
			*cp++ = t % BASE;
			t /= BASE;
		}
		*cp = t;
	}
	if(t == 0)	clen--;
	c.len = clen;
	return c;
}
