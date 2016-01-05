/*		longint.h		*/
#ifndef		_LONGINT
#define		_LONGINT

#ifdef __FREESCALE_MQX__
#include <mqx.h>
#include <bsp.h>
#include <stdlib.h>
#else
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<sys/types.h>
#include	<math.h>
#include	<stdlib.h>
#endif


#define		BASE		100
#define		BASE1		99			/* BASE - 1 */
#define		BASE2		10000		/* BASE ^ 2 */
#define		MAXLEN		500			/* ç≈ëÂåÖêî */
#define		MAXLEN2		1000		/* ÇPÇOêiï\é¶éûÇÃç≈ëÂåÖêî */
#define		NBASE		2			/* BASE = 10 ^ NBASE */

typedef		struct
			{
				int len;
				int sign;			/* + : 0, - : -1 */
				int num[MAXLEN + 1];
			}							 LINT;

LINT lset(long n);
LINT lread(char *s);
LINT ladd(LINT a, LINT b);
LINT aux1(LINT a, LINT b);
LINT alsub(LINT a, LINT b);
LINT lsub(LINT a, LINT b);
LINT sadd(LINT a, int n);
LINT ssub(LINT a, int n);
LINT smul(LINT a, int n);
LINT lmul(LINT a, LINT b);
LINT sdiv(LINT a, int n, int *r);
LINT ldivide(LINT a, LINT b, LINT *r);
LINT lsqrt(LINT a);
LINT lsqr(LINT a);
LINT lmod(LINT a, LINT b);
LINT lpwr(LINT a, int n);
LINT lpwrmod(LINT a, LINT b, LINT n);
LINT lgcd(LINT a, LINT b);
LINT linv(LINT s, LINT n);
LINT lkaijo(int n);
LINT lrnd(int len);
LINT lrnd1(LINT n);
LINT llcm(LINT a, LINT b);
LINT llgcd(LINT *a, int n);
LINT lllcm(LINT *a, int n);
LINT det(LINT *aa, int n);
LINT lmulp(LINT a, LINT b);
int lcmp(LINT a, LINT b);
int iset(LINT a);
int smod(LINT a, int n);
int lprime_chk(LINT x);
void lwrite(char *s, LINT a);
void linc(LINT *a, int n);
void ldec(LINT *a, int n);
void alinc(LINT *a, int n);
void aldec(LINT *a, int n);
void smul1(LINT *a, int n);
void sdiv1(LINT *a, int n, int *r);
void lup(LINT *a, int n);
void ldown(LINT *a, int n);
void lswap(LINT *a, LINT *b);
void lmul1(LINT *a, LINT b);
void lfactor(LINT x);
void larrayse(LINT *a, int *num, int n);
void larrayread(LINT *a, char *string[], int n);
void ldisp(char *s, LINT a);
extern double urnd1(void);

#endif
