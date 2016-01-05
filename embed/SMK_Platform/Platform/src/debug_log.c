/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "debug_log.h"

#include <bsp.h>

#include <string.h>
#include <stdarg.h>

//=============================================================================
void printf_func(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

/******************************** END-OF-FILE ********************************/
