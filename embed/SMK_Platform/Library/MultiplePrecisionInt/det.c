/*		det.c		çsóÒéÆÇÃílåvéZ		*/
#include "longint.h"

LINT det(LINT *aa, int n)
{
	LINT *a, d, dd, g, min, w, x, k1, k2;
	int i, j, k, p, ip, jp;

	if(n < 1)
	{
		fprintf(stderr, "Error : illegal size in  det()\n");
		exit(-1);
	}
	if(n == 1)	return *a;

	a = (LINT *)malloc(n * n * sizeof(LINT));
	if(a == NULL)
	{
		fprintf(stderr, "Error : out of memory  in det().\n");
		exit(-2);
	}
	for(i = 0; i < n * n; i++)	a[i] = aa[i];

	d.sign = dd.sign = 0;
	d.len = dd.len = 1;
	d.num[1] = dd.num[1] = 1;
	for(p = n; p > 2; p--)
	{
		if(a[0].len == 0)
		{
			for(i = 1; i < p; i++)	if(a[i * p].len > 0)	break;
			if(i == p)
			{
				d.len = 0;
				free(a);
				return d;
			}
			d.sign = (d.sign == -1)? 0: -1;
			i *= p;
			for(j = 0; j < p;)
			{
				w = a[j];
				a[j++] = a[i];
				a[i++] = w;
			}
		}
		g = lllcm(a, p);
		k1 = ldivide(g, a[0], &w);
		lmul1(&dd, k1);
		lmul1(&d, g);
		for(i = 1; i < p; i++)	lmul1(&a[i * p], k1);
		for(j = 1; j < p; j++)
		{
			if(a[j].len > 0)
			{
				k2 = ldivide(g, a[j], &w);
				lmul1(&dd, k2);
				for(i = 1; i < p; i++)
				{
					k = i * p;
					a[k + j] = lsub(lmul(a[k + j], k2), a[k]);
				}
			}
		}
		for(i = 1, k = 0; i < p; i++)
			for(j = 1; j < p; j++)	a[k++] = a[i * p + j];
	}
	lmul1(&d, lsub(lmul(a[0], a[3]), lmul(a[1], a[2])));
	d = ldivide(d, dd, &w);
	free(a);
	return d;
}
