/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _HASH_H
#define _HASH_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
#define MD5_BYTES_SIZE 16
#define SHA1_BYTES_SIZE 20
#define SHA256_BYTES_SIZE 32

#define HASH_STATE_SIZE 32 // max
#define HASH_BLOCK_SIZE 64

typedef enum {
	HASH_MD5,
	HASH_SHA1,
	HASH_SHA256
} HashType_t;

typedef void Hash_func(const uint8_t *buf, uint8_t *hash);
typedef void Hash_n_func(const uint8_t *buf, int num_blks, uint8_t *hash);

//=============================================================================
typedef struct {
	uint8_t state[HASH_STATE_SIZE];
	uint8_t block[HASH_BLOCK_SIZE];
	uint32_t remain;
	uint32_t total;

	HashType_t type;
	uint16_t hash_size;
	Hash_func *hash_func;
	Hash_n_func *hash_n_func;
} Hash_t;

//=============================================================================
bool Hash_Init(Hash_t *hash, HashType_t type);
bool Hash_Update(Hash_t *hash, const uint8_t *buf, uint32_t len);
bool Hash_GetDigest(Hash_t *hash, uint8_t *digest);
uint32_t Hash_Size(HashType_t type);

#endif /* !_HASH_H */

/******************************** END-OF-FILE ********************************/
