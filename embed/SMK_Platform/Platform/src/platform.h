/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <mqx.h>
#include <bsp.h>
#include <message.h>

#include "debug_log.h"

//=============================================================================
// Common Message Structure
//=============================================================================
typedef struct groupware_message_struct
{
	MESSAGE_HEADER_STRUCT	header;
	uint32_t				data[8];
} GROUPWARE_MESSAGE_STRUCT, * GROUPWARE_MESSAGE_STRUCT_PTR;

//=============================================================================
// Version Information Structure
//=============================================================================
typedef struct version_data
{
	uint8_t 	bMajorVersion;	/* Major version */
	uint8_t		bMinorVersion;	/* Minor version */
	uint16_t	wYear;			/* Year(BCD) */
  	uint8_t		bMonth;			/* Month(BCD) */
	uint8_t		bDate;			/* Date(BCD) */
} VERSION_DATA, * VERSION_DATA_PTR;

#endif /* !_PLATFORM_H */

/******************************** END-OF-FILE ********************************/
