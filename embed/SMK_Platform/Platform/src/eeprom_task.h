/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _EEPROM_TASK_H
#define _EEPROM_TASK_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
typedef struct {
	uint32_t ofs;
	uint32_t len;
} EEPROM_Table_t;

//=============================================================================
typedef struct {
	const EEPROM_Table_t *table;
	uint32_t table_size;

	_queue_number queue_number;
} EEPROM_TaskParam_t;

//=============================================================================
uint32_t EEPROM_StartTask(EEPROM_TaskParam_t *param, uint32_t priority);

_task_id EEPROM_GetTaskID(void);
_queue_id EEPROM_GetQueueID(void);

bool EEPROM_Write(int id, int size, const void *ptr, _queue_id res_qid);
bool EEPROM_Read(int id, int size, void *ptr, _queue_id res_qid);

#endif /* !_EEPROM_TASK_H */

/******************************** END-OF-FILE ********************************/
