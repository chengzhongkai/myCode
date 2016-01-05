/*		lfactor.c		*/
#include "longint.h"

/* listing prime factor */

void lfactor(LINT x)
{
	LINT d, r, w;
	static int table[4] = {1, 2, 2, 4};
	int c, *t;
	char s[20];

	if(x.len == 0 || x.sign == -1)
	{
		fprintf(stderr, "Error : Illegal parameter  in lfactor()\n");
		return;
	}
	lwrite("input number : ", x);
	d = lset(2);
	t = table;
	c = 0;
	while(lcmp(x, d) > 0)
	{
		w = ldivide(x, d, &r);
		if(r.len == 0)
		{
			x = w;
			sprintf(s, " factor %4d : ", ++c);
			lwrite(s, d);
		}
		else
		{
			alinc(&d, *t);
			if(*t != 4)	t++;
			else		t--;
		}
	}
	sprintf(s, " factor %4d : ", ++c);
	lwrite(s, x);
}
