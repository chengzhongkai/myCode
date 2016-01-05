/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "led_task.h"

//=============================================================================
#define LED_TASK_NUM_LED (2)

#define LED_TASK_QUEUE_SIZE (4)

#define BSP_LED_LD1				(LWGPIO_PORT_D | LWGPIO_PIN5)
#define BSP_LED_LD1_MUX_GPIO	LWGPIO_MUX_D5_GPIO

#define BSP_LED_LD2				(LWGPIO_PORT_D | LWGPIO_PIN4)
#define BSP_LED_LD2_MUX_GPIO	LWGPIO_MUX_D4_GPIO

const GPIOAssign_t gLEDGPIO[LED_TASK_NUM_LED] = {
	{ BSP_LED_LD1, BSP_LED_LD1_MUX_GPIO },
	{ BSP_LED_LD2, BSP_LED_LD2_MUX_GPIO }
};

//=============================================================================
typedef struct {
	_task_id task_id;
	uint32_t queue[sizeof(LWMSGQ_STRUCT) / sizeof(uint32_t)
				   + (2 * LED_TASK_QUEUE_SIZE)];
} LEDTaskHandle_t;

static LEDTaskHandle_t gLEDTaskHandle;

static uint8_t gLEDMode[LED_TASK_NUM_LED];

//=============================================================================
static void LED_Task(uint32_t param);

//=============================================================================
uint32_t LED_StartTask(_task_id *task_id, uint32_t priority)
{
	_mqx_uint err;
	LEDTaskHandle_t *handle = &gLEDTaskHandle;

	_mem_zero((uint8_t *)handle, sizeof(LEDTaskHandle_t));

	err = _lwmsgq_init(handle->queue, LED_TASK_QUEUE_SIZE, 2);
	if (err != MQX_OK) {
		fprintf(stderr, "[LED] cannot create lwmsgq [err=0x%08X]\n", err);
		return (err);
	}

	err = MQX_CreateTask(&handle->task_id, LED_Task, 500,
						 priority, "LED", (uint32_t)handle);
	if (err != MQX_OK) {
		fprintf(stderr, "[LED] cannot create task [err=0x%08X]\n", err);
		return (err);
	}

	if (task_id != NULL) {
		*task_id = handle->task_id;
	}

	return (MQX_OK);
}

//=============================================================================
void LED_SetMode(uint8_t led_number, uint8_t mode)
{
	LEDTaskHandle_t *handle = &gLEDTaskHandle;
	uint32_t msg[2];

	if (led_number >= LED_TASK_NUM_LED) {
		return;
	}

	msg[0] = led_number;
	msg[1] = mode;
	_lwmsgq_send(handle->queue, msg, LWMSGQ_SEND_BLOCK_ON_FULL);
}

//=============================================================================
uint8_t LED_GetMode(uint8_t led_number)
{
	if (led_number >= LED_TASK_NUM_LED) {
		return (LED_MODE_NONE);
	}

	return (gLEDMode[led_number]);
}

//=============================================================================
static void LED_Task(uint32_t param)
{
	LWGPIO_STRUCT led[LED_TASK_NUM_LED];
	LEDTaskHandle_t *handle = (LEDTaskHandle_t *)param;
	const GPIOAssign_t *gpio = gLEDGPIO;

	uint32_t msg[2];
	_mqx_uint err;
	uint8_t led_number;
	uint8_t counter;

	int cnt;

	// ------------------------------------------------------ Init LED GPIO ---
	for (cnt = 0; cnt < LED_TASK_NUM_LED; cnt ++) {
		if (!lwgpio_init(&led[cnt], gpio[cnt].pin_id,
						 LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW)) {
			_task_block();
		}

		lwgpio_set_functionality(&led[cnt], gpio[cnt].pin_mux);
		lwgpio_set_attribute(&led[cnt],
							 LWGPIO_ATTR_DRIVE_STRENGTH,
							 LWGPIO_AVAL_DRIVE_STRENGTH_HIGH);

		lwgpio_set_value(&led[cnt], LED_OFF);
		gLEDMode[cnt] = LED_MODE_OFF;
	}

	// ---------------------------------------------------------- Main Loop ---
	counter = 0;
	while (1) {
		for (cnt = 0; cnt < LED_TASK_NUM_LED; cnt ++) {
			switch (gLEDMode[cnt]) {
			case LED_MODE_OFF:
				lwgpio_set_value(&led[cnt], LED_OFF);
				break;
			case LED_MODE_ON:
				lwgpio_set_value(&led[cnt], LED_ON);
				break;
			case LED_MODE_BLINK_FAST:
				if ((counter & 1) == 0) {
					lwgpio_set_value(&led[cnt], LED_ON);
				} else {
					lwgpio_set_value(&led[cnt], LED_OFF);
				}
				break;
			case LED_MODE_BLINK_SLOW:
				if ((counter & 2) == 0) {
					lwgpio_set_value(&led[cnt], LED_ON);
				} else {
					lwgpio_set_value(&led[cnt], LED_OFF);
				}
				break;
			default:
				lwgpio_set_value(&led[cnt], LED_OFF);
				break;
			}
		}

		err = _lwmsgq_receive(handle->queue, msg, LWMSGQ_TIMEOUT_FOR, 1, 0);
		if (err == MQX_OK) {
			led_number = (uint8_t)msg[0];
			if (led_number < LED_TASK_NUM_LED) {
				gLEDMode[led_number] = (uint8_t)msg[1];
			}
		}
		_time_delay(500);
		counter ++;
	}
}

/******************************** END-OF-FILE ********************************/
