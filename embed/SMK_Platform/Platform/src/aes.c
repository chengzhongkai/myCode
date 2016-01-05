/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "aes.h"

#define FREESCALE_MMCAU	1
#include "cau_api.h"

#include <assert.h>

//=============================================================================
bool AES_Init(AES_t *aes, AESType_t type, uint8_t *key, uint8_t *iv)
{
	if (aes == NULL || key == NULL || iv == NULL) return (FALSE);

	aes->type = type;
	memset(aes->key_expansion, 0, 60 * 4);
	memcpy(aes->iv, iv, AES_BLOCK_SIZE);
	switch (type) {
	case AES_128:
		memcpy(aes->key, key, AES128_KEY_SIZE);
		cau_aes_set_key(aes->key, 128, aes->key_expansion);
		break;
	case AES_192:
		memcpy(aes->key, key, AES192_KEY_SIZE);
		cau_aes_set_key(aes->key, 192, aes->key_expansion);
		break;
	case AES_256:
		memcpy(aes->key, key, AES256_KEY_SIZE);
		cau_aes_set_key(aes->key, 256, aes->key_expansion);
		break;
	default:
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool AES_Encrypt(AES_t *aes, uint8_t *input, uint8_t *output, uint32_t len)
{
	int rounds;
	uint32_t num_blks;
	uint32_t cnt;
	uint8_t xor_buf[AES_BLOCK_SIZE];
	uint8_t out_buf[AES_BLOCK_SIZE];
	int cnt2;
	uint8_t *iv;

	if (aes == NULL || input == NULL || output == NULL) return (FALSE);
	if (len == 0 || (len % AES_BLOCK_SIZE) != 0) return (FALSE);

	switch (aes->type) {
	case AES_128:
		rounds = 10;
		break;
	case AES_192:
		rounds = 12;
		break;
	case AES_256:
		rounds = 14;
		break;
	default:
		return (FALSE);
	}

	num_blks = len / AES_BLOCK_SIZE;
	for (cnt = 0; cnt < num_blks; cnt ++) {
		// XOR input with IV
		if (cnt == 0) {
			iv = aes->iv;
		} else {
			iv = output - AES_BLOCK_SIZE;
		}
		for (cnt2 = 0; cnt2 < AES_BLOCK_SIZE; cnt2 ++) {
			xor_buf[cnt2] = (*input ++) ^ (*iv ++);
		}

		cau_aes_encrypt(xor_buf, aes->key_expansion, rounds, out_buf);
		memcpy(output, out_buf, AES_BLOCK_SIZE);

		output += AES_BLOCK_SIZE;
	}

	// store last IV
	memcpy(aes->iv, output - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

	return (TRUE);
}

//=============================================================================
bool AES_Decrypt(AES_t *aes, uint8_t *input, uint8_t *output, uint32_t len)
{
	int rounds;
	uint32_t num_blks;
	uint32_t cnt;
	int cnt2;
	uint8_t *iv;
	uint8_t in_buf[2][AES_BLOCK_SIZE];
	uint8_t xor_buf[AES_BLOCK_SIZE];

	if (aes == NULL || input == NULL || output == NULL) return (FALSE);
	if (len == 0 || (len % AES_BLOCK_SIZE) != 0) return (FALSE);

	switch (aes->type) {
	case AES_128:
		rounds = 10;
		break;
	case AES_192:
		rounds = 12;
		break;
	case AES_256:
		rounds = 14;
		break;
	default:
		return (FALSE);
	}

	num_blks = len / AES_BLOCK_SIZE;
	for (cnt = 0; cnt < num_blks; cnt ++) {
		memcpy(in_buf[cnt & 1], input, AES_BLOCK_SIZE);
		cau_aes_decrypt(in_buf[cnt & 1], aes->key_expansion, rounds, xor_buf);

		// XOR output with IV
		if (cnt == 0) {
			iv = aes->iv;
		} else {
			// iv = input - AES_BLOCK_SIZE;
			iv = in_buf[(~cnt) & 1];
		}
		for (cnt2 = 0; cnt2 < AES_BLOCK_SIZE; cnt2 ++) {
			(*output ++) = xor_buf[cnt2] ^ (*iv ++);
		}

		input += AES_BLOCK_SIZE;
	}

	// store last IV
	// memcpy(aes->iv, input - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
	memcpy(aes->iv, in_buf[(~cnt) & 1], AES_BLOCK_SIZE);

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
