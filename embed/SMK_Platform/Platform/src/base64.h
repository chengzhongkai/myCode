/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _BASE64_H
#define _BASE64_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
bool Base64_Encode(uint8_t *src_buf, uint32_t src_len,
				   char *dest_buf, uint32_t dest_len);

#endif /* !_BASE64_H */

/******************************** END-OF-FILE ********************************/
