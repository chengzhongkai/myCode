/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "hmac.h"

//=============================================================================
bool HMAC_hash(HashType_t type,
			   const uint8_t *secret, uint32_t secret_len,
			   const uint8_t *msg, uint32_t msg_len,
			   const uint8_t *msg2, uint32_t msg2_len, uint8_t *out)
{
	Hash_t hash;
	int hash_size;
	uint8_t ipad[HASH_BLOCK_SIZE];
	int cnt;

	if (secret == NULL || secret_len == 0 || msg == NULL || msg_len == 0
		|| out == NULL) return (FALSE);

	// if secret is larger than hash block size, what do i do?
	if (secret_len > HASH_BLOCK_SIZE) return (FALSE);

	// get hash size
	hash_size = Hash_Size(type);

	// make ipad
	for (cnt = 0; cnt < HASH_BLOCK_SIZE; cnt ++) {
		if (cnt < secret_len) {
			ipad[cnt] = secret[cnt] ^ 0x36;
		} else {
			ipad[cnt] = 0x00 ^ 0x36;
		}
	}

	// hash(ipad + msg)
	if (!Hash_Init(&hash, type)) return (FALSE);

	Hash_Update(&hash, ipad, HASH_BLOCK_SIZE);
	Hash_Update(&hash, msg, msg_len);
	if (msg2 != NULL && msg2_len > 0) {
		Hash_Update(&hash, msg2, msg2_len);
	}

	Hash_GetDigest(&hash, out); // use out space temporally

	// make opad (reuse ipad space)
	for (cnt = 0; cnt < HASH_BLOCK_SIZE; cnt ++) {
		if (cnt < secret_len) {
			ipad[cnt] = secret[cnt] ^ 0x5c;
		} else {
			ipad[cnt] = 0x00 ^ 0x5c;
		}
	}

	// hash(opad + hash(ipad + msg))
	if (!Hash_Init(&hash, type)) return (FALSE);

	Hash_Update(&hash, ipad, HASH_BLOCK_SIZE);
	Hash_Update(&hash, out, hash_size);

	Hash_GetDigest(&hash, out);

	return (TRUE);
}

//=============================================================================
bool P_hash(HashType_t type, const uint8_t *secret, uint32_t secret_len,
			const uint8_t *msg, uint32_t msg_len,
			uint8_t *out, uint32_t out_len)
{
	uint8_t *array;
	uint32_t array_len;
	int hash_size;
	uint8_t *mid_result;

	if (secret == NULL || secret_len == 0 || msg == NULL || msg_len == 0
		|| out == NULL || out_len == 0) return (FALSE);

	if (secret_len > HASH_BLOCK_SIZE) return (FALSE);

	hash_size = Hash_Size(type);

	// alloc A (array)
	array_len = msg_len + hash_size;
	array = _mem_alloc(array_len);
	if (array == NULL) return (FALSE);

	mid_result = _mem_alloc(hash_size);
	if (mid_result == NULL) {
		_mem_free(array);
		return (FALSE);
	}

	// calc A(1)
	HMAC_hash(type, secret, secret_len, msg, msg_len, NULL, 0, array);

	while (out_len > 0) {
		memcpy(array + hash_size, msg, msg_len);
		HMAC_hash(type, secret, secret_len,
				  array, array_len, NULL, 0, mid_result);
		if (out_len < hash_size) {
			memcpy(out, mid_result, out_len);
			out += out_len;
			out_len = 0;
		} else {
			memcpy(out, mid_result, hash_size);
			out += hash_size;
			out_len -= hash_size;
		}
		if (out_len > 0) {
			HMAC_hash(type, secret, secret_len,
					  array, hash_size, NULL, 0, mid_result);
			memcpy(array, mid_result, hash_size);
		}
	}

	_mem_free(mid_result);
	_mem_free(array);

	return (TRUE);
}

//=============================================================================
bool PRF_calc(const uint8_t *secret, uint32_t secret_len,
			  const uint8_t *label_seed, uint32_t ls_len,
			  uint8_t *out, uint32_t out_len)
{
	uint8_t *p_md5;
	int32_t cnt;

	if (secret == NULL || secret_len == 0
		|| label_seed == NULL || ls_len == 0 || out == NULL || out_len == 0) {
		return (FALSE);
	}

	p_md5 = _mem_alloc(out_len);
	if (p_md5 == NULL) return (FALSE);

	// P_MD5
	if (!P_hash(HASH_MD5, secret, (secret_len + 1) / 2, label_seed, ls_len,
				p_md5, out_len)) {
		_mem_free(p_md5);
		return (FALSE);
	}

	// P_SHA1
	if (!P_hash(HASH_SHA1, secret + (secret_len / 2), (secret_len + 1) / 2,
				label_seed, ls_len, out, out_len)) {
		_mem_free(p_md5);
		return (FALSE);
	}

	for (cnt = 0; cnt < out_len; cnt ++) {
		out[cnt] ^= p_md5[cnt];
	}

	_mem_free(p_md5);
	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
