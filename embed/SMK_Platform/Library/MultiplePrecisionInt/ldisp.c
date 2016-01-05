/*		ldisp.c		*/
#include "longint.h"

/* display LINT */

void ldisp(char *s, LINT a)
{
	int i, ls, n, nflag, *p, r;

	fputs(s, stderr);
	if(a.len == 0)
	{
		fprintf(stderr, "0\n");
		return;
	}
	ls = strlen(s) + 2;
	if(ls < 40)	n = 80 - ls;
	else
	{
		fprintf(stderr, "\n");
		ls = 2;
		n = 78;
	}
	if(a.sign)	fprintf(stderr, " -");
	else		fprintf(stderr, " +");

	p = a.num + a.len;
	a.num[0] = -1;
	fprintf(stderr, "%2d ", *p--);
	r = n - 5;
	nflag = 1;
	while(*p != -1)
	{
		fprintf(stderr, "%02d ", *p--);
		nflag = 1;
		r -= 5;
		if(r <= 5)
		{
			fprintf(stderr, "\n");
			nflag = 0;
			for(i = 0; i < ls; i++)	fprintf(stderr, " ");
			r = n;
		}
	}
	if(nflag)	fprintf(stderr, "\n");
	return;
}
