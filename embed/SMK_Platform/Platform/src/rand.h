/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _RAND_H
#define _RAND_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
void FSL_InitRandom(void);
void FSL_SetRandomSeed(uint32_t seed);
uint32_t FSL_GetRandom(void);

#endif /* !_RAND_H */

/******************************** END-OF-FILE ********************************/
