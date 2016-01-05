/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _DEBUG_LOG_H
#define _DEBUG_LOG_H

#include <mqx.h>

//=============================================================================
// Set Debug Level (2 = All message, 1 = Error message only, 0 = No message)
#if (DEBUG_LEVEL == 2)
#define err_printf printf_func
#define dbg_printf printf_func
#define msg_printf printf_func
#elif (DEBUG_LEVEL == 1)
#define err_printf printf_func
#define dbg_printf(format, ...) ((void)0)
#define msg_printf printf_func
#else
#define err_printf(format, ...) ((void)0)
#define dbg_printf(format, ...) ((void)0)
#define msg_printf(format, ...) ((void)0)
#endif

#if 0
void dummy_func(const char *format, ...);
#define err_printf 1 ? ((void)0) : dummy_func
#define dbg_printf 1 ? ((void)0) : dummy_func
#endif

//=============================================================================
void printf_func(const char *format, ...);

#endif /* !_DEBUG_LOG_H */

/******************************** END-OF-FILE ********************************/
