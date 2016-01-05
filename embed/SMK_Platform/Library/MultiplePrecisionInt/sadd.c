/*		sadd.c		*/
#include "longint.h"

LINT sadd(LINT a, int n)
{
	linc(&a, n);
	return a;
}
