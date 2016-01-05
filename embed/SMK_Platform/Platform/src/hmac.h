/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _HMAC_H
#define _HMAC_H

#include <mqx.h>
#include <bsp.h>

#include "hash.h"

//=============================================================================
bool HMAC_hash(HashType_t type,
			   const uint8_t *secret, uint32_t secret_len,
			   const uint8_t *msg, uint32_t msg_len,
			   const uint8_t *msg2, uint32_t msg2_len,
			   uint8_t *out);

bool PRF_calc(const uint8_t *secret, uint32_t secret_len,
			  const uint8_t *label_seed, uint32_t ls_len,
			  uint8_t *out, uint32_t out_len);

#endif /* !_HMAC_H */

/******************************** END-OF-FILE ********************************/
