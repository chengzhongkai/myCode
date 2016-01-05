/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _ELL_DEPEND_H
#define _ELL_DEPEND_H

#include <assert.h>
#define ELL_Assert assert

#ifdef __FREESCALE_MQX__
#include <mqx.h>
#include <rtcs.h>
#include <stdlib.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#endif

#define MEM_Set(dest, ch, size) memset(dest, ch, size)
#define MEM_Copy(dest, src, size) memcpy(dest, src, size)
#define MEM_Compare(ptr1, ptr2, size) memcmp(ptr1, ptr2, size)
#define MEM_Alloc(size) malloc(size)
#define MEM_Free(h) do { if ((h) != NULL) free(h); } while (0)

#define ELL_Printf printf

#endif // !_ELL_DEPEND_H

/******************************** END-OF-FILE ********************************/
