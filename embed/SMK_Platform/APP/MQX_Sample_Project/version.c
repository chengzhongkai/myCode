/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <bsp.h>
#include "version.h"
#include "Config.h"

//=============================================================================
const VERSION_DATA stSoftwareVersion @".version" =
{
	.bMajorVersion = VERSION_MAJOR,
	.bMinorVersion = VERSION_MINOR,
	.wYear         = VERSION_YEAR,
	.bMonth        = VERSION_MONTH,
	.bDate         = VERSION_DATE
};

/******************************** END-OF-FILE ********************************/
