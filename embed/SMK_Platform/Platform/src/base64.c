/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "base64.h"

#include <assert.h>

//=============================================================================
static const char base64table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

//=============================================================================
static bool take6bit(uint8_t *buf, uint32_t len, uint32_t *ptr, uint32_t *data)
{
	uint32_t idx;
	uint32_t bit;
	uint32_t work;
	uint32_t rest_bit;

	idx = *ptr / 8;
	bit = *ptr & 7;

	if (idx >= len) {
		return (FALSE);
	}

	if (bit <= 2) {
		work = (buf[idx] >> (2 - bit)) & 0x3f;
	} else {
		work = (buf[idx] << (bit - 2)) & 0x3f;
		idx ++;
		rest_bit = bit - 2;
		if (idx < len) {
			work |= (buf[idx] >> (8 - rest_bit));
		}
	}
	*data = work;
	*ptr += 6;

	return (TRUE);
}

//=============================================================================
bool Base64_Encode(uint8_t *src_buf, uint32_t src_len,
				   char *dest_buf, uint32_t dest_len)
{
	uint32_t src_ptr;
	uint32_t data;
	uint32_t dest_idx;

	if (src_buf == NULL || src_len == 0 || dest_buf == NULL || dest_len == 0) {
		return (FALSE);
	}

	src_ptr = 0;
	dest_idx = 0;
	while (take6bit(src_buf, src_len, &src_ptr, &data)) {
		assert(data < 64);

		dest_buf[dest_idx] = base64table[data];
		dest_idx ++;
		if (dest_idx >= dest_len) {
			return (FALSE);
		}
	}

	while ((dest_idx & 3) != 0) {
		dest_buf[dest_idx] = '=';
		dest_idx ++;
		if (dest_idx >= dest_len) {
			return (FALSE);
		}
	}

	dest_buf[dest_idx] = '\0';

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
