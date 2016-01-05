/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _SHA1_H
#define _SHA1_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
#define SHA1_BYTES_SIZE 20

//=============================================================================
typedef uint32_t SHA1_t[5];

//=============================================================================
bool SHA1_Hash(char *str, SHA1_t sha1_array);
bool SHA1_GetDigest(SHA1_t sha1_array, uint8_t digest[SHA1_BYTES_SIZE]);

#endif /* !_SHA1_H */

/******************************** END-OF-FILE ********************************/