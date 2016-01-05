/*		larrayread.c		*/
#include "longint.h"

/* setting LINT array by string array*/

void larrayread(LINT *a, char *string[], int n)
{
	int i;

	for(i = 0; i < n; i++)	a[i] = lread(string[i]);
}
