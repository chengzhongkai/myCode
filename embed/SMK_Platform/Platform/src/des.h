/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _DES_H
#define _DES_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
#define DES_BLOCK_SIZE 8

#define DES1_KEY_SIZE (56 / 8)
#define DES3_KEY_SIZE (168 / 8)

#define DES_CODEC_KEY_SIZE 8 // (56bit + 8bit)

typedef enum {
	DES_1,  // Normal DES
	DES_3   // Triple-DES
} DESType_t;

//=============================================================================
typedef struct {
	uint8_t key[DES_CODEC_KEY_SIZE * 3];
	uint8_t iv[DES_BLOCK_SIZE];
	DESType_t type;
} DES_t;

//=============================================================================
bool DES_Init(DES_t *des, DESType_t type, uint8_t *key, uint8_t *iv);
bool DES_Encrypt(DES_t *des, uint8_t *input, uint8_t *output, uint32_t len);
bool DES_Decrypt(DES_t *des, uint8_t *input, uint8_t *output, uint32_t len);

#endif /* !_DES_H */

/******************************** END-OF-FILE ********************************/
