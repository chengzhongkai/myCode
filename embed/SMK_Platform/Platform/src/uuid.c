/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "uuid.h"
#include "rand.h"

//=============================================================================
#ifdef __FREESCALE_MQX__
#define getRand32() FSL_GetRandom()
#else
#define getRand32() rand()
#endif

//=============================================================================
bool UUID_GetUUID4(UUID_t *uuid)
{
	uint32_t num;

	if (uuid == NULL) {
		return (FALSE);
	}

	num = getRand32();
	*((uint32_t *)&uuid->bytes[0]) = num;
	num = getRand32();
	*((uint32_t *)&uuid->bytes[4]) = num;
	num = getRand32();
	*((uint32_t *)&uuid->bytes[8]) = num;
	num = getRand32();
	*((uint32_t *)&uuid->bytes[12]) = num;

	// clock-seq's MSB 2 bit is '10'
	uuid->fields.clock_seq[0] &= 0x3f;
	uuid->fields.clock_seq[0] |= 0x80;

	// time-high-and-version's MSB 4 bit is '0100' (Version Number is 4)
	uuid->fields.time_high_ver[0] &= 0x0f;
	uuid->fields.time_high_ver[0] |= 0x40;

	return (TRUE);
}

//=============================================================================
#define hexchar(num) (((num) < 10) ? ((num) + '0') : ((num) + 'A' - 10))
static void byte2hex(uint8_t byte, char *hex)
{
	*hex = hexchar(byte >> 4);
	hex ++;
	*hex = hexchar(byte & 0x0f);
}

//=============================================================================
bool UUID_GetString(UUID_t *uuid, char str[UUID_STRING_LEN])
{
	int cnt;
	char *ptr;

	if (uuid == NULL) {
		return (FALSE);
	}

	ptr = str;
	for (cnt = 0; cnt < 16; cnt ++) {
		byte2hex(uuid->bytes[cnt], ptr);
		ptr += 2;
		if (cnt == 3 || cnt == 5 || cnt == 7 || cnt == 9) {
			*ptr = '-';
			ptr ++;
		}
	}
	*ptr = '\0';

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
