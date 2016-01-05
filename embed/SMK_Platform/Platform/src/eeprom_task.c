/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "eeprom_device.h"
#include "eeprom_task.h"

#include "mqx_utils.h"

//=============================================================================
#define EEPROM_TASK_STACK_SIZE 4000
#define EEPROM_TASK_NAME "EEPROM"

/* Request command (in message) */
#define FLASHROM_REQUEST_READ			0			/* READ request */
#define FLASHROM_REQUEST_READ_RES		1			/* Response for READ request */
#define FLASHROM_REQUEST_READ_ERROR		0x80000001	/* Error Response for READ request */
#define FLASHROM_REQUEST_WRITE			2			/* WRITE request */
#define FLASHROM_REQUEST_WRITE_RES		3			/* Response for WRITE request */
#define FLASHROM_REQUEST_WRITE_ERROR	0x80000003	/* Error Response for WRITE request */
#define FLASHROM_REQUEST_ERROR_RES		0xFFFFFFFF	/* Response for unknown command */

//=============================================================================
static const EEPROM_Table_t *gEEPROMTable = NULL;
static uint32_t gEEPROMTableSize = 0;

static _task_id gEEPROMTaskID = 0;
static _queue_id gEEPROMQueueID = 0;

//=============================================================================
static void EEPROM_Task(uint32_t param);

//=============================================================================
uint32_t EEPROM_StartTask(EEPROM_TaskParam_t *param, uint32_t priority)
{
	uint32_t err;

	gEEPROMTable = param->table;
	gEEPROMTableSize = param->table_size;

	err = MQX_CreateTask(&gEEPROMTaskID,
						 EEPROM_Task, EEPROM_TASK_STACK_SIZE,
						 priority, EEPROM_TASK_NAME,
						 (uint32_t)param->queue_number);

	return (err);
}

//=============================================================================
_task_id EEPROM_GetTaskID(void)
{
	return (gEEPROMTaskID);
}

//=============================================================================
_queue_id EEPROM_GetQueueID(void)
{
	return (gEEPROMQueueID);
}

//=============================================================================
bool EEPROM_Write(int id, int size, const void *ptr, _queue_id res_qid)
{
	GROUPWARE_MESSAGE_STRUCT_PTR send_msg = NULL;
	GROUPWARE_MESSAGE_STRUCT_PTR recv_msg = NULL;
	uint8_t *work;
	bool res = FALSE;

	if (size <= 0 || ptr == NULL) return (FALSE);

	/*** check ID and size here, if you want ***/

	// printf("write EEPROM, id=%d, size=%d\n", id, size);

	send_msg = _msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT));
	if (send_msg == NULL) {
		return (FALSE);
	}

	work = _mem_alloc_system(size);
	if (work == NULL) {
		_msg_free(send_msg);
		return (FALSE);
	}

	memcpy(work, ptr, size);

	send_msg->data[0] 		    = FLASHROM_REQUEST_WRITE;
	send_msg->data[1] 		    = id;
	send_msg->data[2]		    = size;
	send_msg->data[3]           = (uint32_t)work;
	send_msg->header.SIZE		= sizeof(GROUPWARE_MESSAGE_STRUCT);
	send_msg->header.TARGET_QID = gEEPROMQueueID;
	send_msg->header.SOURCE_QID = res_qid;

	if (_msgq_send(send_msg) == FALSE) {
		_mem_free(work);
		_msg_free(send_msg);
		return (FALSE);
	}

	recv_msg = _msgq_receive(res_qid, 0);

	if (recv_msg == NULL) {
		return (FALSE);
	}

	if (recv_msg->data[0] == FLASHROM_REQUEST_WRITE_RES) {
		/*** WRITE SUCCESS ***/

		res = TRUE;
	}

	_msg_free(recv_msg);

	return (res);
}

//=============================================================================
bool EEPROM_Read(int id, int size, void *ptr, _queue_id res_qid)
{
	GROUPWARE_MESSAGE_STRUCT_PTR send_msg = NULL;
	GROUPWARE_MESSAGE_STRUCT_PTR recv_msg = NULL;
	bool res = FALSE;

	if (size <= 0 || ptr == NULL) return (FALSE);

	/*** check ID and size here, if you want ***/

	// printf("read EEPROM, id=%d, size=%d\n", id, size);

	send_msg = _msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT));
	if (send_msg == NULL) {
		return (FALSE);
	}

	send_msg->data[0] 			= FLASHROM_REQUEST_READ;
	send_msg->data[1] 			= id;
	send_msg->data[2] 			= 0;
	send_msg->data[3] 			= 0;
	send_msg->header.SIZE			= sizeof(GROUPWARE_MESSAGE_STRUCT);
	send_msg->header.TARGET_QID	= gEEPROMQueueID;
	send_msg->header.SOURCE_QID	= res_qid;

	if (_msgq_send(send_msg) == FALSE) {
		_msg_free(send_msg);
		return (FALSE);
	}

	recv_msg = _msgq_receive(res_qid, 0);

	if (recv_msg == NULL) {
		return (FALSE);
	}

	if (recv_msg->data[0] == FLASHROM_REQUEST_READ_RES) {
		/*** READ SUCCESS ***/
		uint8_t *read_ptr;
		int read_size;

		read_ptr = (uint8_t *)recv_msg->data[3];
		read_size = recv_msg->data[2];

		if (read_ptr != NULL) {
			if (read_size <= size) {
				memcpy(ptr, read_ptr, read_size);
			} else {
				memcpy(ptr, read_ptr, size);
			}

			_mem_free(read_ptr);

			res = TRUE;
		}
	}

	_msg_free(recv_msg);

	return (res);
}

//=============================================================================
static void EEPROM_HandleRead(GROUPWARE_MESSAGE_STRUCT *msg, MQX_FILE *fd)
{
	uint32_t id;
	uint32_t size;
	uint8_t *buf;
	uint32_t read_size;

	if (msg == NULL) return;

	msg->data[0] = FLASHROM_REQUEST_READ_ERROR;

	id = msg->data[1];
	size = msg->data[2];

	// -------------------------------------------- Check Table & ID & Size ---
	if (gEEPROMTable == NULL || gEEPROMTableSize == 0
		|| id >= gEEPROMTableSize || size == 0) {
		return;
	}

	buf = _msg_alloc_system(size);
	if (buf == NULL) return;

	read_size = EEPROMDev_Read(fd,
							   gEEPROMTable[id].ofs, gEEPROMTable[id].len,
							   buf, size);
	if (read_size > 0) {
		msg->data[0] = FLASHROM_REQUEST_READ_RES;
		msg->data[2] = read_size;
		msg->data[3] = (uint32_t)buf;
	} else {
		_mem_free(buf);
	}
}

//=============================================================================
static void EEPROM_HandleWrite(GROUPWARE_MESSAGE_STRUCT *msg, MQX_FILE *fd)
{
	uint32_t id;
	uint32_t size;
	uint8_t *buf;
	uint32_t write_size;

	if (msg == NULL) return;

	msg->data[0] = FLASHROM_REQUEST_WRITE_ERROR;

	buf = (uint8_t *)msg->data[3];
	if (buf == NULL) return;
	msg->data[3] = 0;

	id = msg->data[1];
	size = msg->data[2];

	// -------------------------------------------- Check Table & ID & Size ---
	if (gEEPROMTable == NULL || gEEPROMTableSize == 0
		|| id >= gEEPROMTableSize || size == 0) {
		_mem_free(buf);
		return;
	}

	write_size = EEPROMDev_Write(fd,
								 gEEPROMTable[id].ofs, gEEPROMTable[id].len,
								 buf, size);
	if (write_size > 0) {
		msg->data[0] = FLASHROM_REQUEST_WRITE_RES;
		msg->data[2] = write_size;
	}

	_mem_free(buf);
}

//=============================================================================
static void EEPROM_Task(uint32_t param)
{
	MQX_FILE *fd;

	_queue_number q_no = (_queue_number)param;

	GROUPWARE_MESSAGE_STRUCT *msg;

	// ------------------------------------------------- Open EEPROM Device ---
	fd = EEPROMDev_Open();
	if (fd == NULL) {
		_task_block();
	}

	// ------------------------------------- Init Message Queue for Request ---
	gEEPROMQueueID = _msgq_open(q_no, 0);
	if (gEEPROMQueueID == MSGQ_NULL_QUEUE_ID) {
		fprintf(stderr, "[EEPROM] Cannot create queue [err=0x%08X]\n",
				_task_get_error());

		EEPROMDev_Close(fd);
		_task_block();
	}

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		msg = _msgq_receive(gEEPROMQueueID, 0);
		if (msg != NULL) {
			switch (msg->data[0]) {
			case FLASHROM_REQUEST_READ:
				EEPROM_HandleRead(msg, fd);
				break;
			case FLASHROM_REQUEST_WRITE:
				EEPROM_HandleWrite(msg, fd);
				break;
			default:
				msg->data[0] = FLASHROM_REQUEST_ERROR_RES;
				break;
			}

			msg->header.TARGET_QID = msg->header.SOURCE_QID;
			msg->header.SOURCE_QID = gEEPROMQueueID;

			if (!_msgq_send(msg)) {
				if (msg->data[0] == FLASHROM_REQUEST_READ_RES) {
			  		_mem_free((void *)msg->data[3]);
				}
				_msg_free(msg);
			}
		}
	}
}

/******************************** END-OF-FILE ********************************/
