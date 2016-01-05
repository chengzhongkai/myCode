/*		lmod.c		*/
#include "longint.h"

/* modulus LINT % LINT */

LINT lmod(LINT a, LINT b)
{
	LINT aa, bb, r, w;
	int *anum1, *bnum1, *p, *q, *rp, *wnum1;
	int alen, btop, i, j, d, ql, t, x;

	r.len = r.sign = 0;
	if(b.len == 1)	return lset(smod(a, b.num[1]));
	switch(lcmp(a, b))
	{
	case 0:		return r;
	case -1:	return a;
	}
	if(a.len == MAXLEN)
	{
		fprintf(stderr, "Error : Too large  in lmod1()\n");
		return r;
	}
	r.sign = a.sign;
	d = BASE / (b.num[b.len] + 1);
	aa = a;
	bb = b;
	anum1 = a.num + 1;
	bnum1 = b.num + 1;
	if(d != 1)
	{
		for(i = 1, t = 0, p = anum1; i <= a.len; i++)
		{
			*p++ = (t += *p * d) % BASE;
			t /= BASE;
		}
		while(t)
		{
			a.num[++a.len] = t % BASE;
			t /= BASE;
		}
		for(i = 1, t = 0, p = bnum1; i <= b.len; i++)
		{
			*p++ = (t += *p * d) % BASE;
			t /= BASE;
		}
	}
	btop = b.num[b.len];
	ql = a.len - b.len + 1;
	if((x = a.num[a.len] / btop) == 0)
	{
		ql--;
		x = (a.num[a.len] * BASE + a.num[a.len - 1]) / btop;
	}
	alen = a.len;
	a.sign = b.sign = 0;
	a.num[0] = 1;
	w.len = 0;
	wnum1 = w.num + 1;
	for(;;)
	{
		ql--;
		for(;;)
		{
			p = wnum1;
			i = ql;
			while(i--)	*p++ = 0;
			t = w.sign = 0;
			i = b.len;
			q = bnum1;
			while(i--)
			{
				*p++ = (t += *q++ * x) % BASE;
				t /= BASE;
			}
			while(t)
			{
				*p++ = t % BASE;
				t /= BASE;
			}
			if(alen > (w.len = p - wnum1))	break;
			else if(alen == w.len)
			{
				p = a.num + alen;
				q = w.num + alen;
				while(*q-- == *p--)	;
				if(*(++q) < *(++p))	break;
			}
			x--;
		}

		for(i = w.len + 1, q = w.num + i; i <= alen; i++)	*q++ = 0;
		p = anum1;
		q = wnum1;
		i = alen;
		while(i--)
		{
			if((*p -= *q++) < 0)
			{
				*p += BASE;
				(*q)++;
			}
			p++;
		}
		while(*(--p) == 0)	;
		if((alen = p - a.num) == 0 || ql == 0)	break;
		x = (a.num[alen] * BASE + a.num[alen - 1]) / btop;
	}
	a.len = alen;
	if(d == 1)	r = a;
	else
	{
		r.sign = a.sign;
		i = r.len = a.len;
		t = 0;
		p = a.num + i;
		q = r.num + i;
		while(i--)
		{
			*q-- = (t = (t * BASE + *p--)) / d;
			t %= d;
		}
		while(r.num[r.len] == 0)	r.len--;
	}
	if(r.sign != aa.sign)
	{
		if(r.sign == 0)	r = lsub(r, bb);
		else			r = ladd(r, bb);
	}
	return r;
}
