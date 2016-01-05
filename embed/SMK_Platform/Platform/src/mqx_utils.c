/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "mqx_utils.h"

//=============================================================================
uint32_t MQX_CreateTask(_task_id *task_id,
						TASK_FPTR func, _mem_size stack_size,
						uint32_t priority, const char *name, uint32_t param)
{
	uint32_t org_err;
	TASK_TEMPLATE_STRUCT template;
	_task_id tid;
	uint32_t err;

	if (task_id == NULL) {
		return (MQX_INVALID_POINTER);
	}

	org_err = _task_get_error();

	_mem_zero((uint8_t *)&template, sizeof(template));

	template.TASK_ADDRESS = func;
	template.TASK_STACKSIZE = stack_size;
	template.TASK_PRIORITY = priority;
	template.TASK_NAME = (char *)name;
	template.CREATION_PARAMETER = param;

	tid = _task_create(0, 0, (uint32_t)&template);
	if (tid == MQX_NULL_TASK_ID) {
		err = _task_get_error();
		fprintf(stderr, "[MQX] Creating Task Failed (0x%08x)\n", err);
	} else {
		err = MQX_OK;
		*task_id = tid;
	}

	_task_set_error(org_err);

	return (err);
}

/******************************** END-OF-FILE ********************************/
