/*		ldiv.c		*/
#include "longint.h"

/* division LINT / LINT */

LINT ldivide(LINT a, LINT b, LINT *r)
{
	int i, d, *p, ql, *rp, x;
	LINT aa, bb, q, w;

	q.len = q.sign = 0;
	r->len = r->sign = 0;
	if(a.len == 0)	goto ret;
	if(b.len == 0)
	{
		fprintf(stderr, "Error : Divide by 0  in ldivide()\n");
		goto ret;
	}
	if(b.len == 1)
	{
		if(b.sign == 0)	q = sdiv(a, b.num[1], &d);
		else			q = sdiv(a, - b.num[1], &d);
		*r = lset(d);
		goto ret;
	}
	switch(lcmp(a, b))
	{
	case 0:
		q.sign = (a.sign == b.sign) ? 0: -1;
		q.len = 1;
		q.num[1] = 1;
		goto ret;
	case -1:
		*r = a;
		goto ret;
	}
	if(a.len == MAXLEN)
	{
		fprintf(stderr, "Error : Too large  in ldivide()\n");
		goto ret;
	}
	q.sign = (a.sign == b.sign) ? 0: -1;
	r->sign = a.sign;
	d = BASE / (b.num[b.len] + 1);
	aa = a;
	bb = b;
	if(d != 1)
	{
		smul1(&aa, d);
		smul1(&bb, d);
	}
	q.len = aa.len - bb.len + 1;
	if((x = aa.num[aa.len] / bb.num[bb.len]) == 0)
	{
		q.len--;
		x = (aa.num[aa.len] * BASE + aa.num[aa.len - 1]) / bb.num[bb.len];
	}
	aa.sign = bb.sign;
	w.len = 0;
	for(i = 1, p = q.num + 1; i <= q.len; i++)	*p++ = 0;
	ql = q.len;
	for(;;)
	{
		for(;;)
		{
			w = smul(bb, x);
			lup(&w, ql - 1);
			if(lcmp(w, aa) <= 0)	break;
			x--;
		}
		q.num[ql--] = x;
		aa = lsub(aa, w);
		if(aa.len == 0 || ql == 0)	break;
		x = (aa.num[aa.len] * BASE + aa.num[aa.len - 1]) / bb.num[bb.len];
	}
	if(q.num[q.len] == 0)	q.len--;
	if(aa.len > 0)
	{
		*r = sdiv(aa, d, &x);
		if(r->sign != a.sign)
		{
			if (a.sign == 0)	aldec(&q, 1);
			else				alinc(&q, 1);
			if (r->sign == 0)	*r = lsub(*r, b);
			else				*r = ladd(*r, b);
		}
	}
ret:
	return q;
}
