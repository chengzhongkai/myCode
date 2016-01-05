/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "bignum.h"
#include <assert.h>

#if 1 // DEBUG
//=============================================================================
void BigNum_Print(BigNum_t *bignum)
{
	int cnt;

	assert(bignum != NULL);

	if (bignum->len == 0) {
		printf("0x0");
		return;
	}

	printf("0x");
	for (cnt = (bignum->len - 1); cnt >= 0; cnt --) {
		printf("%02x", bignum->num[cnt]);
	}
}
#endif

//=============================================================================
bool BigNum_InitFromHexString(BigNum_t *bignum, const char *hex_str)
{
	return (TRUE);
}

//=============================================================================
bool BigNum_InitFromBuffer(BigNum_t *bignum, const uint8_t *buf, uint32_t len)
{
	uint32_t cnt;

	if (bignum == NULL) return (FALSE);

	if (len < BIGNUM_DEFAULT_ALLOC) {
		bignum->alloc = BIGNUM_DEFAULT_ALLOC;
	} else {
		bignum->alloc = len;
	}

	bignum->num = _mem_alloc(bignum->alloc);
	if (bignum->num == NULL) {
		return (FALSE);
	}

	memset(bignum->num, 0, bignum->alloc);
	bignum->len = 0;
	bignum->sign = BIGNUM_SIGN_PLUS;

	// if buf is NULL or len is 0, set as '0'
	if (buf == NULL || len == 0) {
		return (TRUE);
	}

	// skip zero
	for (cnt = 0; cnt < len; cnt ++) {
		if (buf[cnt] != 0) break;
	}
	buf += cnt;
	len -= cnt;
	if (len == 0) {
		return (TRUE);
	}

	// set MSB to LSB
	for (cnt = 0; cnt < len; cnt ++) {
		bignum->num[cnt] = buf[len - cnt - 1];
	}
	bignum->len = len;

	return (TRUE);
}

//=============================================================================
void BigNum_Clear(BigNum_t *bignum)
{
	if (bignum == NULL) return;

	memset(bignum->num, 0, bignum->alloc);
	bignum->len = 0;
	bignum->sign = BIGNUM_SIGN_PLUS;
}

//=============================================================================
bool BigNum_Duplicate(BigNum_t *dest, BigNum_t *src)
{
	if (dest == NULL || src == NULL) return (FALSE);

	dest->num = _mem_alloc(src->alloc);
	if (dest->num == NULL) {
		return (FALSE);
	}

	dest->alloc = src->alloc;
	memcpy(dest->num, src->num, dest->alloc);
	dest->len = src->len;
	dest->sign = src->sign;

	return (TRUE);
}

//=============================================================================
bool BigNum_Copy(BigNum_t *dest, BigNum_t *src)
{
	if (dest == NULL || src == NULL) return (FALSE);

	if (dest->alloc < src->len) {
		return (FALSE);
	}

	memcpy(dest->num, src->num, src->len);
	dest->len = src->len;
	dest->sign = src->sign;
	memset(&dest->num[dest->len], 0, dest->alloc - dest->len);

	return (TRUE);
}

//=============================================================================
void BigNum_Free(BigNum_t *bignum)
{
	if (bignum == NULL) return;

	if (bignum->num != NULL) {
		_mem_free(bignum->num);
		bignum->num = NULL;
	}
	bignum->len = 0;
	bignum->alloc = 0;
}

//=============================================================================
bool BigNum_BitTest(BigNum_t *dest, uint32_t bit)
{
	uint32_t idx;
	uint8_t test;

	if (dest == NULL) return (FALSE);

	idx = bit / 8;
	if (idx >= dest->len) return (FALSE);

	bit %= 8;
	test = 1 << bit;

	return ((dest->num[idx] & test) != 0);
}

//=============================================================================
int BigNum_Compare(BigNum_t *num1, BigNum_t *num2)
{
	int cnt;

	if (num1 == NULL || num2 == NULL) return (0);

	if (num1->sign != num2->sign) {
		if (num1->sign == BIGNUM_SIGN_PLUS) {
			return (1);
		} else {
			return (-1);
		}
	}

	if (num1->len > num2->len) {
		if (num1->sign == BIGNUM_SIGN_PLUS) {
			return (1);
		} else {
			return (-1);
		}
	}
	else if (num1->len < num2->len) {
		if (num1->sign == BIGNUM_SIGN_PLUS) {
			return (-1);
		} else {
			return (1);
		}
	}

	for (cnt = (num1->len - 1); cnt >= 0; cnt --) {
		if (num1->num[cnt] != num2->num[cnt]) break;
	}
	if (cnt >= 0) {
		if (num1->num[cnt] < num2->num[cnt]) {
			if (num1->sign == BIGNUM_SIGN_PLUS) {
				return (-1);
			} else {
				return (1);
			}
		}
		else if (num1->num[cnt] < num2->num[cnt]) {
			if (num1->sign == BIGNUM_SIGN_PLUS) {
				return (-1);
			} else {
				return (1);
			}
		}
	}

	return (0);
}

//=============================================================================
// Shift Right
// dest >>= num_shift
//=============================================================================
bool BigNum_ShiftRight(BigNum_t *dest, uint32_t num_shift)
{
	uint32_t shift_bytes;
	uint8_t prev, next;
	int cnt;

	if (dest == NULL) return (FALSE);
	if (num_shift == 0) return (TRUE);

	shift_bytes = num_shift / 8;
	if (shift_bytes >= dest->len) {
		dest->len = 0;
		dest->num[0] = 0;
		return (TRUE);
	}
	if (shift_bytes > 0) {
		memcpy(&dest->num[0],
			   &dest->num[shift_bytes], dest->len - shift_bytes);
		dest->len -= shift_bytes;
		memset(&dest->num[dest->len], 0, shift_bytes);
	}

	num_shift %= 8;

	if (num_shift > 0) {
		prev = 0;
		for (cnt = (dest->len - 1); cnt >= 0; cnt --) {
			next = dest->num[cnt] << (8 - num_shift);
			dest->num[cnt] = prev | dest->num[cnt] >> num_shift;
			prev = next;
		}
		// check if MSB byte is zero
		for (cnt = (dest->len - 1); cnt >= 0; cnt --) {
			if (dest->num[cnt] != 0) break;
		}
		dest->len = cnt + 1;
	}

	return (TRUE);
}

//=============================================================================
// Shift Left
// dest <<= num_shift
//=============================================================================
bool BigNum_ShiftLeft(BigNum_t *dest, uint32_t num_shift)
{
	uint32_t shift_bytes;
	uint8_t prev, next;
	int cnt;

	if (dest == NULL) return (FALSE);
	if (num_shift == 0) return (TRUE);

	shift_bytes = num_shift / 8;
	if (shift_bytes > (dest->alloc - dest->len)) {
		// Overflow...
		return (FALSE);
	}
	if (shift_bytes > 0) {
		memcpy(&dest->num[shift_bytes], &dest->num[0], dest->len);
		memset(&dest->num[0], 0, shift_bytes);
		dest->len += shift_bytes;
	}

	num_shift %= 8;

	if (num_shift > 0) {
		prev = 0;
		for (cnt = 0; cnt < dest->len; cnt ++) {
			next = dest->num[cnt] >> (8 - num_shift);
			dest->num[cnt] = prev | dest->num[cnt] << num_shift;
			prev = next;
		}
		if (prev != 0) {
			if (dest->len < dest->alloc) {
				dest->num[dest->len] = prev;
				dest->len ++;
			} else {
				// Overflow...
			}
		}
	}

	return (TRUE);
}

//=============================================================================
// dest += add
//=============================================================================
bool BigNum_Add(BigNum_t *dest, BigNum_t *add)
{
	int len;
	int cnt;
	uint16_t tmp, next;

	if (dest == NULL || add == NULL) return (FALSE);

	len = (dest->len > add->len) ? dest->len : add->len;

	next = 0;
	for (cnt = 0; cnt < len; cnt ++) {
		tmp = (uint32_t)dest->num[cnt] + (uint32_t)add->num[cnt] + next;
		dest->num[cnt] = (uint8_t)(tmp & 0xff);
		next = tmp >> 8;
	}
	if (next > 0) {
		if (len <dest->alloc) {
			dest->num[len] = next;
			len ++;
		} else {
			// Overflow...
		}
	}
	dest->len = len;

	return (TRUE);
}

//=============================================================================
// dest -= sub
//=============================================================================
bool BigNum_Sub(BigNum_t *dest, BigNum_t *sub)
{
	uint8_t borrow;
	uint16_t sub_val;
	int cnt;

	if (dest == NULL || sub == NULL) return (FALSE);

	if (dest->sign != sub->sign) {
		BigNum_Add(dest, sub);
		return (TRUE);
	}

	if (BigNum_Compare(dest, sub) >= 0) {
		borrow = 0;
		for (cnt = 0; cnt < dest->len; cnt ++) {
			sub_val = (uint16_t)sub->num[cnt];
			sub_val += borrow;
			if (((uint16_t)dest->num[cnt]) >= sub_val) {
				borrow = 0;
			} else {
				borrow = 1;
			}
			dest->num[cnt] = (uint8_t)((uint16_t)dest->num[cnt] - sub_val);
		}
	} else {
		borrow = 0;
		for (cnt = 0; cnt < sub->len; cnt ++) {
			sub_val = (uint16_t)dest->num[cnt];
			sub_val += borrow;
			if (((uint16_t)sub->num[cnt]) >= sub_val) {
				borrow = 0;
			} else {
				borrow = 1;
			}
			dest->num[cnt] = (uint8_t)((uint16_t)sub->num[cnt] - sub_val);
		}
		if (dest->sign == BIGNUM_SIGN_PLUS) {
			dest->sign = BIGNUM_SIGN_MINUS;
		} else {
			dest->sign = BIGNUM_SIGN_PLUS;
		}
	}

	// check if MSB byte is zero
	for (cnt = (dest->len - 1); cnt >= 0; cnt --) {
		if (dest->num[cnt] != 0) break;
	}
	dest->len = cnt + 1;

	return (TRUE);
}

//=============================================================================
bool BigNum_Mod(BigNum_t *dest, BigNum_t *mod)
{
	if (dest == NULL || mod == NULL) return (FALSE);

	while (BigNum_Compare(dest, mod) >= 0) {
		BigNum_Sub(dest, mod);
	}

	return (TRUE);
}

//=============================================================================
// dest = (a * b) % mod
// a and b will be destroyed
//=============================================================================
bool BigNum_MulMod(BigNum_t *dest,
				   BigNum_t *num_a, BigNum_t *num_b, BigNum_t *mod)
{
	if (dest == NULL || num_a == NULL || num_b == NULL || mod == NULL) {
		return (FALSE);
	}

	memset(dest->num, 0, dest->alloc);
	dest->len = 0;
	dest->sign = BIGNUM_SIGN_PLUS;

	while (num_b->len > 0) {
		if (BigNum_BitTest(num_b, 0)) {
			BigNum_Add(dest, num_a);
			BigNum_Mod(dest, mod);
		}
		BigNum_ShiftLeft(num_a, 1);
		BigNum_Mod(num_a, mod);
		BigNum_ShiftRight(num_b, 1);
	}

	return (TRUE);
}

//=============================================================================
// dest = (src ** ex) % mod
//=============================================================================
bool BigNum_PowMod(BigNum_t *dest, BigNum_t *src, BigNum_t *ex, BigNum_t *mod)
{
	uint8_t init_val[1];
	BigNum_t work;
	BigNum_t work2;

	if (dest == NULL || src == NULL || ex == NULL || mod == NULL) {
		return (FALSE);
	}

	if (!BigNum_InitZero(&work)) {
		return (FALSE);
	}
	if (!BigNum_InitZero(&work2)) {
		return (FALSE);
	}

	init_val[0] = 1;
	if (!BigNum_InitFromBuffer(dest, init_val, 1)) {
		return (FALSE);
	}

	while (ex->len > 0) {
		if (BigNum_BitTest(ex, 0)) {
			BigNum_Copy(&work2, src);
			BigNum_MulMod(&work, dest, src, mod);
			BigNum_Copy(dest, &work);
			BigNum_Copy(src, &work2);
		}
		BigNum_ShiftRight(ex, 1);
		BigNum_Copy(&work2, src);
		BigNum_MulMod(&work, src, &work2, mod);
		BigNum_Copy(src, &work);
	}

	BigNum_Free(&work);
	BigNum_Free(&work2);

	return (TRUE);
}

//=============================================================================
uint32_t BigNum_NumBytes(BigNum_t *bignum)
{
	if (bignum == NULL) return (0);

	return (bignum->len);
}

//=============================================================================
uint32_t BigNum_GetBytes(BigNum_t *bignum, uint8_t *out, uint32_t out_len)
{
	int32_t idx;

	if (bignum == NULL || out == NULL || out_len == 0) return (0);

	if (out_len > bignum->alloc) {
		out_len = bignum->alloc;
	}

	for (idx = (out_len - 1); idx >= 0; idx --) {
		*out = bignum->num[idx];
		out ++;
	}

	return (bignum->len);
}

/******************************** END-OF-FILE ********************************/
