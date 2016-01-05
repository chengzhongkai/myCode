/*		lwrite.c		*/
#include "longint.h"

/* print out LINT */

void lwrite(char *s, LINT a)
{
	int even, n = 30, *p;

	printf("< %s >\n", s);
	if(a.len == 0)
	{
		printf("0\n");
		return;
	}
	if(a.sign)	printf(" -");
	else		printf(" +");

	p = a.num + a.len;
	a.num[0] = -1;
	if(*p < 10)	printf(" %d", *p--);
	else		printf("%2d", *p--);
	n--;
	even = a.len % 2;
	if(even)	putchar(' ');
	while(*p != -1)
	{
		printf("%02d", *p--);
		if(--n == 0)
		{
			putchar('\n');
			n = 30;
		}
		even = 1 - even;
		if(even)	putchar(' ');
	}
	putchar('\n');
	return;
}
