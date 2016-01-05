/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MQX_UTILS_H
#define _MQX_UTILS_H

#include <mqx.h>
#include <bsp.h>

#include <lwgpio.h>

//=============================================================================
typedef struct {
	LWGPIO_PIN_ID pin_id;
	uint32_t pin_mux;
} GPIOAssign_t;


//=============================================================================
uint32_t MQX_CreateTask(_task_id *task_id,
						TASK_FPTR func, _mem_size stack_size,
						uint32_t priority, const char *name, uint32_t param);

void MQX_SWReset(void);

#endif /* !_MQX_UTILS_H */

/******************************** END-OF-FILE ********************************/
