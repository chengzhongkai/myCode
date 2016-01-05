/*		larrayset.c		*/
#include "longint.h"

/* setting LINT array by int array*/

void larrayse(LINT *a, int *num, int n)
{
	int i;

	for(i = 0; i < n; i++)	a[i] = lset(num[i]);
}
