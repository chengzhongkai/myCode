/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "rand.h"

//=============================================================================
void FSL_InitRandom(void)
{
	SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK;
	SIM_SCGC6 |= SIM_SCGC6_RNGA_MASK;

	// RNG_CR = RNG_CR_GO_MASK | RNG_CR_HA_MASK | RNG_CR_INTM_MASK;
	RNG_CR = RNG_CR_GO_MASK | RNG_CR_HA_MASK;
}

//=============================================================================
void FSL_SetRandomSeed(uint32_t seed)
{
	RNG_ER = seed;
}

//=============================================================================
uint32_t FSL_GetRandom(void)
{
	while (((RNG_SR & RNG_SR_OREG_LVL_MASK) >> RNG_SR_OREG_LVL_SHIFT) == 0) {
		/* DO NOTHING */
	}
	return (RNG_OR);
}

/******************************** END-OF-FILE ********************************/
