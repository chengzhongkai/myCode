/*****************************************************************************/
/* HA Control Driver on Freescale MQX Platform                               */
/*                                                                           */
/* (c) Copyright 2015, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _HA_CONTROL_H
#define _HA_CONTROL_H

#include <mqx.h>
#include <bsp.h>
#include <message.h>
#include "platform.h"

//=============================================================================
#define HA_CONTROL_NUM_PORT 2

//=============================================================================
typedef enum {
	HA_STATE_OFF = 0,
	HA_STATE_ON,
	HA_STATE_ERROR
} HaState_t;

//=============================================================================
uint32_t HaControl_Init(void);

HaState_t HaControl_GetState(uint8_t ha_number);
HaState_t HaControl_ChangeState(uint8_t ha_number, HaState_t state);

#endif /* !_HA_CONTROL_H */

/******************************** END-OF-FILE ********************************/
