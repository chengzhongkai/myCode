/*		smod.c		*/
#include "longint.h"

/* modulus LINT % int */

int smod(LINT a, int n)
{
	LINT t, w;
	int i, r;

	if(n == BASE)	r = a.num[1];
	else if(n > BASE)
	{
		t = lset(n);
		ldivide(a, t, &w);
		return iset(w);
	}
	if(a.len == 0)	return 0;
	for(i = a.len, r = 0; i > 0; i--)	r = (r * BASE + a.num[i]) % n;
	return (a.sign == 0) ? r: - r;
}
