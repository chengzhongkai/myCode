/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _UUID_H
#define _UUID_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
#define UUID_BYTES_SIZE 16

// 00112233-4455-6677-8899-AABBCCDDEEFF + '\0'
#define UUID_STRING_LEN 37

//=============================================================================
typedef union {
	uint8_t bytes[UUID_BYTES_SIZE];
	struct {
		uint8_t time_low[4];
		uint8_t time_mid[2];
		uint8_t time_high_ver[2];
		uint8_t clock_seq[2];
		uint8_t node[6];
	} fields;
} UUID_t;

//=============================================================================
bool UUID_GetUUID4(UUID_t *uuid);
bool UUID_GetString(UUID_t *uuid, char str[UUID_STRING_LEN]);

#endif /* !_UUID_H */

/******************************** END-OF-FILE ********************************/
