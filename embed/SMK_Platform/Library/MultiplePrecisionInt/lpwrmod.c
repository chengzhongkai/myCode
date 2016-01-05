/*		lpwrmod.c		*/
#include "longint.h"

LINT lmod1(LINT a, LINT *n, LINT *nn, int d, int nntop, int nnlen);

/* modulus of power  LINT ^ LINT (mod LINT) */

LINT lpwrmod(LINT a, LINT b, LINT n)
{
	LINT nn, x;
	int *bnum1, *btop, *cp, *p, *q;
	int alen, d, nntop, nnlen, i, j, t, u;

	if(a.sign < 0 || b.sign < 0 || n.sign < 0 || n.len == 0)
	{
		fprintf(stderr, "Error : illegal parameter  in lpwrmod1()\n");
		exit(-3);
	}
	if(a.len == 0)	return a;
	if(b.len == 0)	return lset(1);

	x.len = x.sign = 0;
	x.len = 1;
	x.num[1] = 1;
	nn = n;
	nnlen = nn.len;
	d = BASE / (nn.num[nnlen] + 1);
	if(d != 1)
	{
		for(i = 1, t = 0; i <= nnlen; i++)
		{
			nn.num[i] = (t += nn.num[i] * d) % BASE;
			t /= BASE;
		}
	}
	nntop = nn.num[nnlen];
	btop = b.num + b.len;
	bnum1 = b.num + 1;
	do
	{
		if(*bnum1 % 2 == 0)
		{
			p = btop;
			t = 0;
			i = btop - b.num;
			while(i--)
			{
				*p-- = (t += *p) / 2;
				t = (t % 2)? BASE: 0;
			}
			while(*btop == 0)	btop--;
			a = lmod1(lsqr(a), &n, &nn, d, nntop, nnlen);
			if(a.len == 0)
			{
				x.len = 0;
				break;
			}
		}
		else
		{
			(*bnum1)--;
			x = lmod1(lmulp(x, a), &n, &nn, d, nntop, nnlen);
			if(x.len == 0)	break;
		}
	} while(btop > bnum1 || *bnum1);
	return x;
}

LINT lmod1(LINT a, LINT *n, LINT *nn, int d, int nntop, int nnlen)
{
	LINT w;
	int *anum1, *nnnum1, *wnum1, *p, *q;
	int alen, i, ql, t, dx;

	if(n->len == 1)	return lset(smod(a, n->num[1]));
	if(n->len > (alen = a.len))	return a;
	else if(alen == n->len)
	{
		a.num[0] = 0;
		n->num[0] = 1;
		p = a.num + alen;
		q = n->num + alen;
		while(*p-- == *q--)	;
		if(p == a.num)
		{
			a.len = 0;
			return a;
		}
		if(*(++p) < *(++q))	return a;
	}

	if(alen == MAXLEN)
	{
		fprintf(stderr, "Error : Too large  in lmod1()\n");
		a.len = 0;
		return a;
	}
	anum1 = a.num + 1;
	if(d != 1)
	{
		t = 0;
		p = anum1;
		i = alen;
		while(i--)
		{
			*p = (t += *p * d) % BASE;
			t /= BASE;
			p++;
		}
		while(t)
		{
			*p++ = t % BASE;
			t /= BASE;
		}
		a.len = alen = --p - a.num;
	}
	ql = alen - nnlen + 1;
	if((dx = a.num[alen] / nntop) == 0)
	{
		ql--;
		dx = (a.num[alen] * BASE + a.num[alen - 1]) / nntop;
	}
	a.num[0] = 1;
	w.len = w.sign = 0;
	w.num[0] = 0;
	nnnum1 = nn->num + 1;
	wnum1 = w.num + 1;
	for(;;)
	{
		ql--;
		for(;;)
		{
			q = wnum1;
			i = ql;
			while(i--)	*q++ = 0;
			t = 0;
			p = nnnum1;
			i = nnlen;
			while(i--)
			{
				*q++ = (t += *p++ * dx) % BASE;
				t /= BASE;
			}
			while(t)
			{
				*q++ = t % BASE;
				t /= BASE;
			}
			if(alen > (w.len = q - wnum1))	break;
			else if(alen == w.len)
			{
				p = a.num + alen;
				q = w.num + alen;
				while(*q-- == *p--)	;
				if(*(++q) < *(++p))	break;
			}
			dx--;
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
		dx = (a.num[alen] * BASE + a.num[alen - 1]) / nntop;
	}
	if(d != 1)
	{
		p = a.num + (i = alen);
		t = 0;
		while(i--)
		{
			*p = (t += *p) / d;
			t = (t % d) * BASE;
			p--;
		}
		while(a.num[alen] == 0)	alen--;
	}
	a.len = alen;
	return a;
}
