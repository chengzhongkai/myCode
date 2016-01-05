/*		ssub.c		*/
#include "longint.h"

LINT ssub(LINT a, int n)
{
	ldec(&a, n);
	return a;
}
