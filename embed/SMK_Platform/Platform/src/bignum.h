/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _BIGNUM_H
#define _BIGNUM_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
// #define BIGNUM_DEFAULT_ALLOC 512
#define BIGNUM_DEFAULT_ALLOC (256 + 4)

//=============================================================================
typedef enum {
	BIGNUM_SIGN_PLUS = 0,
	BIGNUM_SIGN_MINUS
} BigNumSign_t;

//=============================================================================
typedef struct {
	uint16_t len;
	uint8_t *num;
	uint16_t alloc;
	BigNumSign_t sign;
} BigNum_t;

//=============================================================================
bool BigNum_InitFromHexString(BigNum_t *bignum, const char *hex_str);
bool BigNum_InitFromBuffer(BigNum_t *bignum, const uint8_t *buf, uint32_t len);
#define BigNum_InitZero(bignum) BigNum_InitFromBuffer((bignum), NULL, 0)
void BigNum_Clear(BigNum_t *bignum);
bool BigNum_Duplicate(BigNum_t *dest, BigNum_t *src);
bool BigNum_Copy(BigNum_t *dest, BigNum_t *src);
void BigNum_Free(BigNum_t *bignum);

bool BigNum_BitTest(BigNum_t *dest, uint32_t bit);
int BigNum_Compare(BigNum_t *num1, BigNum_t *num2);

bool BigNum_ShiftRight(BigNum_t *dest, uint32_t num_shift);
bool BigNum_ShiftLeft(BigNum_t *dest, uint32_t num_shift);

bool BigNum_Add(BigNum_t *dest, BigNum_t *add);
bool BigNum_Sub(BigNum_t *dest, BigNum_t *sub);

bool BigNum_PowMod(BigNum_t *dest, BigNum_t *src, BigNum_t *ex, BigNum_t *mod);

uint32_t BigNum_NumBytes(BigNum_t *bignum);
uint32_t BigNum_GetBytes(BigNum_t *bignum, uint8_t *out, uint32_t out_len);

#endif /* !_BIGNUM_H */

/******************************** END-OF-FILE ********************************/
