/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _AES_H
#define _AES_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
#define AES_BLOCK_SIZE 16

#define AES128_KEY_SIZE (128 / 8)
#define AES192_KEY_SIZE (192 / 8)
#define AES256_KEY_SIZE (256 / 8)

typedef enum {
	AES_128,
	AES_192,
	AES_256
} AESType_t;

//=============================================================================
typedef struct {
	uint8_t key[AES256_KEY_SIZE];
	uint8_t iv[AES_BLOCK_SIZE];
	uint8_t key_expansion[60 * 4];
	AESType_t type;
} AES_t;

//=============================================================================
bool AES_Init(AES_t *aes, AESType_t type, uint8_t *key, uint8_t *iv);
bool AES_Encrypt(AES_t *aes, uint8_t *input, uint8_t *output, uint32_t len);
bool AES_Decrypt(AES_t *aes, uint8_t *input, uint8_t *output, uint32_t len);

#endif /* !_AES_H */

/******************************** END-OF-FILE ********************************/
