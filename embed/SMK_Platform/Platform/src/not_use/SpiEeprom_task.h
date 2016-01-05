#ifndef SPIMEM_TASK_H_
#define SPIMEM_TASK_H_

#include "platform.h"

/* Request command (in message) */
#define EEPROM_REQUEST_READ				0			/* READ request */
#define EEPROM_REQUEST_READ_RES			1			/* Response for READ request */
#define EEPROM_REQUEST_READ_ERROR		0x80000001	/* Error Response for READ request */
#define EEPROM_REQUEST_WRITE			2			/* WRITE request */
#define EEPROM_REQUEST_WRITE_RES		3			/* Response for WRITE request */
#define EEPROM_REQUEST_WRITE_ERROR		0x80000003	/* Error Response for WRITE request */
#define EEPROM_REQUEST_ERASE			4			/* ERASE request */
#define EEPROM_REQUEST_ERASE_RES		5			/* Response for ERASE request */
#define EEPROM_REQUEST_ERASE_ERROR		0x80000005	/* Error Response for ERASE request */
#define EEPROM_REQUEST_ISEMPTY			6			/* ISEMPTY request */
#define EEPROM_REQUEST_ISEMPTY_RES		7			/* Response for ISEMPTY request */
#define EEPROM_REQUEST_ISEMPTY_ERROR	0x80000007	/* Error Response for ISEMPTY request */
#define EEPROM_REQUEST_ERROR_RES		0xFFFFFFFF	/* Response for unknown command */

extern _task_id SpiEeprom_GetTaskID(void);
extern _queue_id SpiEeprom_GetQueueID(void);
extern void SpiEeprom_task (uint32_t dummy);

#endif
