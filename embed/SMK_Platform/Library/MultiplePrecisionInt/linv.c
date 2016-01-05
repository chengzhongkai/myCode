/*		linv.c		*/
#include "longint.h"

/* GCD(s, n) = 1  :  s * x = 1 (mod n) */

LINT linv(LINT s, LINT n)
{
	LINT q, t, u, v, w;

	t = n;
	u = lset(1);
	v.len = 0;
	while(s.len > 0)
	{
		q = ldivide(t, s, &w);
		t = s;
		s = w;
		w = lmul(q, u);
		w = lsub(v, w);
		v = u;
		u = w;
	}
	if(v.sign == -1)	v = ladd(v, n);
	return v;
}
