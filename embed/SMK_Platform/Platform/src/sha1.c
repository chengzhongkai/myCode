/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "sha1.h"

#define FREESCALE_MMCAU	1
#include "cau_api.h"

//=============================================================================
bool SHA1_Hash(char *str, SHA1_t sha1_array)
{
	uint32_t len;
	uint32_t total;
	uint8_t page[64];
	uint32_t add;

	if (str == NULL) {
		return (FALSE);
	}

	len = strlen(str);
	total = len;

	cau_sha1_initialize_output(sha1_array);

	if (len >= 64) {
		cau_sha1_hash_n((uint8_t *)str, len / 64, sha1_array);
		add = (len / 64) * 64;
		str += add;
		len -= add;
	}

	// 0 <= len <= 63
	if (len > 0) {
		memcpy(page, str, len);
	}
	page[len] = 0x80;
	len ++;
	if (len < 64) {
		memset(page + len, 0, 64 - len);
	}
	if (len > (64 - 8)) {
		cau_sha1_hash(page, sha1_array);
		memset(page, 0, 64);
	}

	page[64 - 5] = (total >> 29) & 0xff;
	page[64 - 4] = (total >> 21) & 0xff;
	page[64 - 3] = (total >> 13) & 0xff;
	page[64 - 2] = (total >> 5) & 0xff;
	page[64 - 1] = (total << 3) & 0xff;

	cau_sha1_hash(page, sha1_array);

	return (TRUE);
}

//=============================================================================
bool SHA1_GetDigest(SHA1_t sha1_array, uint8_t digest[SHA1_BYTES_SIZE])
{
	int cnt, idx;

	idx = 0;
	for (cnt = 0; cnt < 5; cnt ++) {
		digest[idx ++] = (uint8_t)((sha1_array[cnt] >> 24) & 0xff);
		digest[idx ++] = (uint8_t)((sha1_array[cnt] >> 16) & 0xff);
		digest[idx ++] = (uint8_t)((sha1_array[cnt] >>  8) & 0xff);
		digest[idx ++] = (uint8_t)((sha1_array[cnt] >>  0) & 0xff);
	}

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
