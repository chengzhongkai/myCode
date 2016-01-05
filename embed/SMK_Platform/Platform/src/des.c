/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "des.h"

#define FREESCALE_MMCAU	1
#include "cau_api.h"

#include <assert.h>

#include "platform.h"

//=============================================================================
static const uint8_t parity_table[] = {
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

//=============================================================================
static void add_parity(uint8_t *in, uint8_t *out, int len)
{
	int bit_idx;

	for (bit_idx = 0; bit_idx < (len * 8); bit_idx += 7) {
		switch ((bit_idx & 7)) {
		case 0:
			*out = *in & 0xfe;
			break;
		case 1:
			*out = (*in << 1) & 0xfe;
			in ++;
			break;
		case 2:
			*out = ((in[0] << 2) | (in[1] >> 6)) & 0xfe;
			in ++;
			break;
		case 3:
			*out = ((in[0] << 3) | (in[1] >> 5)) & 0xfe;
			in ++;
			break;
		case 4:
			*out = ((in[0] << 4) | (in[1] >> 4)) & 0xfe;
			in ++;
			break;
		case 5:
			*out = ((in[0] << 5) | (in[1] >> 3)) & 0xfe;
			in ++;
			break;
		case 6:
			*out = ((in[0] << 6) | (in[1] >> 2)) & 0xfe;
			in ++;
			break;
		case 7:
			*out = ((in[0] << 7) | (in[1] >> 1)) & 0xfe;
			in ++;
			break;
		}
		*out |= parity_table[*out >> 1];
		out ++;
	}
}

//=============================================================================
bool DES_Init(DES_t *des, DESType_t type, uint8_t *key, uint8_t *iv)
{
	if (des == NULL || key == NULL || iv == NULL) return (FALSE);

	des->type = type;
	memcpy(des->iv, iv, DES_BLOCK_SIZE);
	switch (type) {
	case DES_1:
		add_parity(key, des->key, DES1_KEY_SIZE);
		if (cau_des_chk_parity(des->key) != 0) {
			err_printf("DES parity error\n");
			return (FALSE);
		}
		break;
	case DES_3:
		add_parity(key, des->key, DES3_KEY_SIZE);
		if (cau_des_chk_parity(des->key) != 0
			|| cau_des_chk_parity(des->key + DES_CODEC_KEY_SIZE) != 0
			|| cau_des_chk_parity(des->key + DES_CODEC_KEY_SIZE * 2) != 0) {
			err_printf("3DES parity error\n");
			return (FALSE);
		}
		break;
	default:
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool DES_Encrypt(DES_t *des, uint8_t *input, uint8_t *output, uint32_t len)
{
	uint32_t num_blks;
	uint32_t cnt;
	uint8_t temp_buf[DES_BLOCK_SIZE];
	int cnt2;
	uint8_t *iv;

	if (des == NULL || input == NULL || output == NULL) return (FALSE);
	if (len == 0 || (len % DES_BLOCK_SIZE) != 0) return (FALSE);

	num_blks = len / DES_BLOCK_SIZE;
	for (cnt = 0; cnt < num_blks; cnt ++) {
		// XOR input with IV
		if (cnt == 0) {
			iv = des->iv;
		} else {
			iv = output - DES_BLOCK_SIZE;
		}
		for (cnt2 = 0; cnt2 < DES_BLOCK_SIZE; cnt2 ++) {
			temp_buf[cnt2] = (*input ++) ^ (*iv ++);
		}

		cau_des_encrypt(temp_buf, des->key, output);

		if (des->type == DES_3) {
			cau_des_decrypt(output, des->key + DES_CODEC_KEY_SIZE, output);
			cau_des_encrypt(output, des->key + DES_CODEC_KEY_SIZE * 2, output);
		}

		output += DES_BLOCK_SIZE;
	}

	// store last IV
	memcpy(des->iv, output - DES_BLOCK_SIZE, DES_BLOCK_SIZE);

	return (TRUE);
}

//=============================================================================
bool DES_Decrypt(DES_t *des, uint8_t *input, uint8_t *output, uint32_t len)
{
	uint32_t num_blks;
	uint32_t cnt;
	int cnt2;
	uint8_t *iv;

	if (des == NULL || input == NULL || output == NULL) return (FALSE);
	if (len == 0 || (len % DES_BLOCK_SIZE) != 0) return (FALSE);

	num_blks = len / DES_BLOCK_SIZE;
	for (cnt = 0; cnt < num_blks; cnt ++) {
		switch (des->type) {
		case DES_1:
			cau_des_decrypt(input, des->key, output);
			break;
		case DES_3:
			cau_des_decrypt(input, des->key + DES_CODEC_KEY_SIZE * 2, output);
			cau_des_encrypt(output, des->key + DES_CODEC_KEY_SIZE, output);
			cau_des_decrypt(output, des->key, output);
			break;
		}

		// XOR output with IV
		if (cnt == 0) {
			iv = des->iv;
		} else {
			iv = input - DES_BLOCK_SIZE;
		}
		for (cnt2 = 0; cnt2 < DES_BLOCK_SIZE; cnt2 ++) {
			(*output ++) ^= (*iv ++);
		}

		input += DES_BLOCK_SIZE;
	}

	// store last IV
	memcpy(des->iv, input - DES_BLOCK_SIZE, DES_BLOCK_SIZE);

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
