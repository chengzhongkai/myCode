/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "hash.h"

#define FREESCALE_MMCAU	1
#include "cau_api.h"

#include <assert.h>

//=============================================================================
static void Hash_MD5(const uint8_t *buf, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_md5_hash(buf, state);
}

//=============================================================================
static void Hash_SHA1(const uint8_t *buf, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_sha1_hash(buf, (uint32_t *)state);
}

//=============================================================================
static void Hash_SHA256(const uint8_t *buf, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_sha256_hash(buf, (uint32_t *)state);
}

//=============================================================================
static void Hash_MD5_N(const uint8_t *buf, int num_blk, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_md5_hash_n(buf, num_blk, state);
}

//=============================================================================
static void Hash_SHA1_N(const uint8_t *buf, int num_blk, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_sha1_hash_n(buf, num_blk, (uint32_t *)state);
}

//=============================================================================
static void Hash_SHA256_N(const uint8_t *buf, int num_blk, uint8_t *state)
{
	// assert((((uint32_t)buf) & 3) == 0);
	cau_sha256_hash_n(buf, num_blk, (uint32_t *)state);
}

//=============================================================================
bool Hash_Init(Hash_t *hash, HashType_t type)
{
	if (hash == NULL) return (FALSE);

	switch (type) {
	case HASH_MD5:
		cau_md5_initialize_output(hash->state);
		hash->hash_size = MD5_BYTES_SIZE;
		hash->hash_func = Hash_MD5;
		hash->hash_n_func = Hash_MD5_N;
		break;
	case HASH_SHA1:
		cau_sha1_initialize_output((uint32_t *)hash->state);
		hash->hash_size = SHA1_BYTES_SIZE;
		hash->hash_func = Hash_SHA1;
		hash->hash_n_func = Hash_SHA1_N;
		break;
	case HASH_SHA256:
		cau_sha256_initialize_output((uint32_t *)hash->state);
		hash->hash_size = SHA256_BYTES_SIZE;
		hash->hash_func = Hash_SHA256;
		hash->hash_n_func = Hash_SHA256_N;
		break;
	default:
		return (FALSE);
	}

	hash->type = type;
	hash->remain = 0;
	hash->total = 0;

	return (TRUE);
}

//=============================================================================
bool Hash_Update(Hash_t *hash, const uint8_t *buf, uint32_t len)
{
	uint32_t add;

	if (hash == NULL) return (FALSE);
	if (buf == NULL || len == 0) return (FALSE);

	hash->total += len;

	if (hash->remain > 0) {
		if ((hash->remain + len) < HASH_BLOCK_SIZE) {
			memcpy(&hash->block[hash->remain], buf, len);
			hash->remain += len;
			return (TRUE);
		}
		uint32_t fill_len;
		fill_len = HASH_BLOCK_SIZE - hash->remain;
		memcpy(&hash->block[hash->remain], buf, fill_len);
		(*hash->hash_func)(hash->block, hash->state);
		buf += fill_len;
		len -= fill_len;
		hash->remain = 0;
		if (len == 0) return (TRUE);
	}

	assert(hash->remain == 0);

	if (len >= HASH_BLOCK_SIZE) {
		(*hash->hash_n_func)(buf, len / HASH_BLOCK_SIZE, hash->state);
		add = (len / HASH_BLOCK_SIZE) * HASH_BLOCK_SIZE;
		buf += add;
		len -= add;
	}

	if (len > 0) {
		memcpy(hash->block, buf, len);
		hash->remain = len;
	}

	return (TRUE);
}

//=============================================================================
bool Hash_GetDigest(Hash_t *hash, uint8_t *digest)
{
	int cnt, idx;
	uint32_t len;

	if (hash == NULL) return (FALSE);

	len = hash->remain;

	hash->block[len] = 0x80;
	len ++;
	if (len < HASH_BLOCK_SIZE) {
		memset(&hash->block[len], 0, HASH_BLOCK_SIZE - len);
	}
	if (len > (HASH_BLOCK_SIZE - 8)) {
		(*hash->hash_func)(hash->block, hash->state);
		memset(hash->block, 0, HASH_BLOCK_SIZE);
	}

	switch (hash->type) {
	case HASH_MD5:
		hash->block[HASH_BLOCK_SIZE - 4] = (hash->total >> 29) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 5] = (hash->total >> 21) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 6] = (hash->total >> 13) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 7] = (hash->total >> 5) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 8] = (hash->total << 3) & 0xff;
		break;
	case HASH_SHA1:
	case HASH_SHA256:
		hash->block[HASH_BLOCK_SIZE - 5] = (hash->total >> 29) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 4] = (hash->total >> 21) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 3] = (hash->total >> 13) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 2] = (hash->total >> 5) & 0xff;
		hash->block[HASH_BLOCK_SIZE - 1] = (hash->total << 3) & 0xff;
		break;
	}

	(*hash->hash_func)(hash->block, hash->state);

	switch (hash->type) {
	case HASH_MD5:
		memcpy(digest, hash->state, MD5_BYTES_SIZE);
		break;
	case HASH_SHA1:
		idx = 0;
		for (cnt = 0; cnt < (SHA1_BYTES_SIZE / 4); cnt ++) {
			digest[idx ++] = hash->state[cnt * 4 + 3];
			digest[idx ++] = hash->state[cnt * 4 + 2];
			digest[idx ++] = hash->state[cnt * 4 + 1];
			digest[idx ++] = hash->state[cnt * 4];
		}
		break;
	case HASH_SHA256:
		idx = 0;
		for (cnt = 0; cnt < (SHA256_BYTES_SIZE / 4); cnt ++) {
			digest[idx ++] = hash->state[cnt * 4 + 3];
			digest[idx ++] = hash->state[cnt * 4 + 2];
			digest[idx ++] = hash->state[cnt * 4 + 1];
			digest[idx ++] = hash->state[cnt * 4];
		}
		break;
	}

	return (TRUE);
}

//=============================================================================
uint32_t Hash_Size(HashType_t type)
{
	switch (type) {
	case HASH_MD5:
		return (MD5_BYTES_SIZE);
	case HASH_SHA1:
		return (SHA1_BYTES_SIZE);
	case HASH_SHA256:
		return (SHA256_BYTES_SIZE);
	default:
		return (0);
	}
}


#if 0
//=============================================================================
bool SHA1_Init(SHA1_t *sha1)
{
	if (sha1 == NULL) return (FALSE);

	cau_sha1_initialize_output(sha1->hash);

	sha1->remain = 0;
	sha1->total = 0;

	return (TRUE);
}

//=============================================================================
bool SHA1_Update(SHA1_t *sha1, const uint8_t *buf, uint32_t len)
{
	uint32_t add;

	if (sha1 == NULL) return (FALSE);
	if (buf == NULL || len == 0) return (FALSE);

	sha1->total += len;

	if (sha1->remain > 0) {
		if ((sha1->remain + len) < HASH_BLOCK_SIZE) {
			memcpy(&sha1->block[sha1->remain], buf, len);
			return (TRUE);
		}
		uint32_t fill_len;
		fill_len = HASH_BLOCK_SIZE - sha1->remain;
		memcpy(&sha1->block[sha1->remain], buf, fill_len);
		cau_sha1_hash(sha1->block, sha1->hash);
		buf += fill_len;
		len -= fill_len;
		sha1->remain = 0;
		if (len == 0) return (TRUE);
	}

	assert(sha1->remain == 0);

	if (len >= HASH_BLOCK_SIZE) {
		cau_sha1_hash_n(buf, len / HASH_BLOCK_SIZE, sha1->hash);
		add = (len / HASH_BLOCK_SIZE) * HASH_BLOCK_SIZE;
		buf += add;
		len -= add;
	}

	if (len > 0) {
		memcpy(sha1->block, buf, len);
		sha1->remain = len;
	}

	return (TRUE);
}

//=============================================================================
bool SHA1_GetDigest(SHA1_t *sha1, uint8_t digest[SHA1_BYTES_SIZE])
{
	int cnt, idx;
	uint32_t len;

	if (sha1 == NULL) return (FALSE);

	len = sha1->remain;

	sha1->block[len] = 0x80;
	len ++;
	if (len < HASH_BLOCK_SIZE) {
		memset(&sha1->block[len], 0, HASH_BLOCK_SIZE - len);
	}
	if (len > (HASH_BLOCK_SIZE - 8)) {
		cau_sha1_hash(sha1->block, sha1->hash);
		memset(sha1->block, 0, HASH_BLOCK_SIZE);
	}

	sha1->block[HASH_BLOCK_SIZE - 5] = (sha1->total >> 29) & 0xff;
	sha1->block[HASH_BLOCK_SIZE - 4] = (sha1->total >> 21) & 0xff;
	sha1->block[HASH_BLOCK_SIZE - 3] = (sha1->total >> 13) & 0xff;
	sha1->block[HASH_BLOCK_SIZE - 2] = (sha1->total >> 5) & 0xff;
	sha1->block[HASH_BLOCK_SIZE - 1] = (sha1->total << 3) & 0xff;

	cau_sha1_hash(sha1->block, sha1_array);

	idx = 0;
	for (cnt = 0; cnt < 5; cnt ++) {
		digest[idx ++] = (uint8_t)((sha1->hash[cnt] >> 24) & 0xff);
		digest[idx ++] = (uint8_t)((sha1->hash[cnt] >> 16) & 0xff);
		digest[idx ++] = (uint8_t)((sha1->hash[cnt] >>  8) & 0xff);
		digest[idx ++] = (uint8_t)((sha1->hash[cnt] >>  0) & 0xff);
	}

	return (TRUE);
}
#endif

/******************************** END-OF-FILE ********************************/
