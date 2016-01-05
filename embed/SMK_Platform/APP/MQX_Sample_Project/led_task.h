/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _LED_TASK_H
#define _LED_TASK_H

#include <mqx.h>
#include <bsp.h>
#include <lwgpio.h>
#include <lwmsgq.h>

#include "mqx_utils.h"

//=============================================================================
#define LED_OFF LWGPIO_VALUE_HIGH
#define LED_ON LWGPIO_VALUE_LOW

enum {
	LED_MODE_NONE = 0,
	LED_MODE_OFF,
	LED_MODE_ON,
	LED_MODE_BLINK_FAST,
	LED_MODE_BLINK_SLOW
};

#define LED_ORANGE (0)
#define LED_GREEN (1)

//=============================================================================
uint32_t LED_StartTask(_task_id *task_id, uint32_t priority);
void LED_SetMode(uint8_t led_number, uint8_t mode);
uint8_t LED_GetMode(uint8_t led_number);

#endif /* !_LED_TASK_H */

/******************************** END-OF-FILE ********************************/
