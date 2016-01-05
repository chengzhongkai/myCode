/*****************************************************************************/
/* HA Control Driver on Freescale MQX Platform                               */
/*                                                                           */
/* (c) Copyright 2015, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "HaControl.h"

//=============================================================================
typedef struct {
	LWGPIO_PIN_ID pin_id;
	uint32_t pin_mux;
} GPIOAssign_t;

//=== Port Configuration ======================================================
typedef struct {
	GPIOAssign_t c_signal;
	GPIOAssign_t m_signal;
} HaPortConfig_t;

#define BSP_GPIO_PORT_C17  (LWGPIO_PORT_C | LWGPIO_PIN17)
#define BSP_GPIO_MUX_C17   LWGPIO_MUX_C17_GPIO

#define BSP_GPIO_PORT_C18  (LWGPIO_PORT_C | LWGPIO_PIN18)
#define BSP_GPIO_MUX_C18   LWGPIO_MUX_C18_GPIO

#define BSP_GPIO_PORT_C13  (LWGPIO_PORT_C | LWGPIO_PIN13)
#define BSP_GPIO_MUX_C13   LWGPIO_MUX_C13_GPIO

#define BSP_GPIO_PORT_C14  (LWGPIO_PORT_C | LWGPIO_PIN14)
#define BSP_GPIO_MUX_C14   LWGPIO_MUX_C14_GPIO

static const HaPortConfig_t gHaPortConfig[2] = {
	// HA Port A
	{
		{ BSP_GPIO_PORT_C17, BSP_GPIO_MUX_C17 }, // C Signal (OUT)
		{ BSP_GPIO_PORT_C18, BSP_GPIO_MUX_C18 }  // M Signal (IN)
	},
	// HA Port B
	{
		{ BSP_GPIO_PORT_C13, BSP_GPIO_MUX_C13 }, // C Signal (OUT)
		{ BSP_GPIO_PORT_C14, BSP_GPIO_MUX_C14 }  // M Signal (IN)
	},
};

//=== Port Structure ==========================================================
typedef struct {
	LWGPIO_STRUCT c_signal;
	LWGPIO_STRUCT m_signal;
} HaPort_t;

static HaPort_t gHaPort[HA_CONTROL_NUM_PORT];

enum {
	HA_PORT_SIGNAL_LOW = 0,
	HA_PORT_SIGNAL_HIGH,
	HA_PORT_SIGNAL_ERROR
};

//=============================================================================
static LWSEM_STRUCT gHaControlSem;
static HWTIMER gHaControlHWTimer;

static volatile uint16_t gHaSignalChangeCount;

//=============================================================================
static _queue_id gHaControlQueueID = 0;

//=============================================================================
static void HaControl_InitPort(HaPort_t *port, const HaPortConfig_t *config);

//=============================================================================
// Initialize HA Driver
//=============================================================================
uint32_t HaControl_Init(void)
{
	int cnt;
	uint32_t err;

	// ---------------------------------------------------- Initialize GPIO ---
	for (cnt = 0; cnt < HA_CONTROL_NUM_PORT; cnt ++) {
		HaControl_InitPort(&gHaPort[cnt], &gHaPortConfig[cnt]);
	}

	// ----------------------------------------------- Initialize Semaphore ---
	err = _lwsem_create(&gHaControlSem, 1);
	if (err != MQX_OK) {
		return (err);
	}

	// --------------------------- Initialize Hardware Timer (use HWTIMER1) ---
	err = hwtimer_init(&gHaControlHWTimer,
					   &BSP_HWTIMER1_DEV, BSP_HWTIMER1_ID,
					   BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX + 1);
	if (err != MQX_OK) {
		return (err);
	}

	// --------------------------------------------------- Set 100us Period ---
	hwtimer_set_period(&gHaControlHWTimer, BSP_HWTIMER1_SOURCE_CLK, 100);

	return (MQX_OK);
}

//=============================================================================
// M Signal ISR for Sensing Dynamic Signal
//=============================================================================
#ifdef HA_CONTROL_USE_INT
static void HaControl_ISR(void *param)
{
	// uint8_t signal;
	LWGPIO_STRUCT *port = (LWGPIO_STRUCT *)param;

	lwgpio_int_clear_flag(port);

	volatile uint8_t val;
	val = lwgpio_get_value(port);

	gHaSignalChangeCount ++;
	// signal = HaControl_ScanSignal((LWGPIO_STRUCT_PTR)port);
}
#endif

//=============================================================================
// Init Port
//=============================================================================
static void HaControl_InitPort(HaPort_t *port, const HaPortConfig_t *config)
{
	if (port == NULL || config == NULL) {
		return;
	}

	// ------------------------------------------- Initialize C-Signal Port ---
	if (!lwgpio_init(&port->c_signal, config->c_signal.pin_id,
					 LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_HIGH)) {
		/*** ERROR ***/
		return;
	}
	lwgpio_set_functionality(&port->c_signal, config->c_signal.pin_mux);

    lwgpio_set_attribute(&port->c_signal,
						 LWGPIO_ATTR_DRIVE_STRENGTH,
						 LWGPIO_AVAL_DRIVE_STRENGTH_HIGH);

	// ------------------------------------------- Initialize M-Signal Port ---
	if (!lwgpio_init(&port->m_signal, config->m_signal.pin_id,
					 LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE)) {
		/*** ERROR ***/
		return;
	}
	lwgpio_set_functionality(&port->m_signal, config->m_signal.pin_mux);

#ifdef HA_CONTROL_USE_INT
	if (!lwgpio_int_init(&port->m_signal,
						 LWGPIO_INT_MODE_FALLING | LWGPIO_INT_MODE_RISING)) {
		/*** ERROR ***/
		return;
	}

	uint32_t vec = lwgpio_int_get_vector(&port->m_signal);
	if (_int_install_isr(vec,
						 HaControl_ISR, (void *)&port->m_signal) == NULL) {
		/*** ERROR ***/
		return;
	}
	if (_bsp_int_init(vec, 3, 0, TRUE) != MQX_OK) {
		/*** ERROR ***/
		return;
	}
	lwgpio_int_enable(&port->m_signal, FALSE);
#endif
}

//=============================================================================
#if 0
static uint8_t HaControl_ScanMSignal(HaPort_t *port)
{
	uint8_t val1, val2;
	uint16_t last_usec, cur_usec, diff_usec;

	val1 = lwgpio_get_value(&port->m_signal);
	last_usec = _time_get_microseconds();
	do {
		cur_usec = _time_get_microseconds();
		diff_usec = cur_usec - last_usec;
	} while (diff_usec < 20);
	val2 = lwgpio_get_value(&port->m_signal);

	if (val1 != val2) {
		return (HA_PORT_SIGNAL_ERROR);
	}

	if (val1 == LWGPIO_VALUE_LOW) {
		return (HA_PORT_SIGNAL_HIGH);
	} else {
		return (HA_PORT_SIGNAL_LOW);
	}
}
#endif

//=============================================================================
// Scan Port Signal
//=============================================================================
static uint8_t HaControl_ScanSignal(LWGPIO_STRUCT *port)
{
	uint8_t val;

	val = lwgpio_get_value(port);
	if (val == LWGPIO_VALUE_LOW) {
		return (HA_PORT_SIGNAL_HIGH);
	} else {
		return (HA_PORT_SIGNAL_LOW);
	}
}

//=============================================================================
static uint8_t gHaPrevSignal;
static uint16_t gHaSameCount;
static uint16_t gHaPrevCount;
static void HaControl_HWTimerISR(void *param)
{
	LWGPIO_STRUCT *port = (LWGPIO_STRUCT *)param;
	uint8_t signal;

	signal = HaControl_ScanSignal(port);
	if (signal != gHaPrevSignal) {
		if (signal == HA_PORT_SIGNAL_LOW
			&& gHaSignalChangeCount > 0 && gHaSameCount < 8) {
			// HIGH -> LOW
			// less than about 0.8ms (100us x 8),
			// this LOW -> HIGH -> LOW might be noise.
			gHaSignalChangeCount --;

			gHaSameCount = gHaPrevCount + gHaSameCount;
			gHaPrevCount = 0;

			gHaPrevSignal = signal;
		} else {
			if (gHaSignalChangeCount < 0xffff) {
				gHaSignalChangeCount ++;
			}
			gHaPrevCount = gHaSameCount;
			gHaSameCount = 0;

			gHaPrevSignal = signal;
		}
	} else {
		if (gHaSameCount < 0xffff) {
			gHaSameCount ++;
		}
	}
}

//=============================================================================
static uint16_t HaControl_CountEdge(LWGPIO_STRUCT *port)
{
#ifdef HA_CONTROL_USE_INT
	gHaSignalChangeCount = 0;
	lwgpio_int_clear_flag(&port->m_signal);
	lwgpio_int_enable(&port->m_signal, TRUE);

	_time_delay(40);

	lwgpio_int_enable(&port->m_signal, FALSE);

	return (gHaSignalChangeCount);
#else
	gHaSignalChangeCount = 0;
	gHaPrevSignal = HaControl_ScanSignal(port);
	gHaPrevCount = 0;
	gHaSameCount = 0;

	hwtimer_callback_reg(&gHaControlHWTimer, HaControl_HWTimerISR, port);

	hwtimer_start(&gHaControlHWTimer);

	_time_delay(40);

	hwtimer_stop(&gHaControlHWTimer);

	return (gHaSignalChangeCount);
#endif
}

//=============================================================================
// Get HA State
//=============================================================================
static HaState_t HaControl_GetState_inner(uint8_t ha_number)
{
	HaPort_t *port;
	uint8_t signal;
	uint16_t num_edge;

	if (ha_number >= HA_CONTROL_NUM_PORT) {
		return (HA_STATE_ERROR);
	}
	port = &gHaPort[ha_number];

	num_edge = HaControl_CountEdge(&port->m_signal);

#if 0
	printf("HA%d:%d\n", ha_number, num_edge);
#endif

	switch (num_edge) {
	case 0:
		// Stay
	case 1:
		// Toggled
		signal = HaControl_ScanSignal(&port->m_signal);
		if (signal == HA_PORT_SIGNAL_LOW) {
			return (HA_STATE_OFF);
		} else {
			return (HA_STATE_ON);
		}
		break;
	default:
		// Dynamic On
		return (HA_STATE_ON);
	}

	return (HA_STATE_ERROR);
}

//=============================================================================
// Get HA State
//=============================================================================
HaState_t HaControl_GetState(uint8_t ha_number)
{
	HaState_t state;

	_lwsem_wait(&gHaControlSem);

	state = HaControl_GetState_inner(ha_number);

	_lwsem_post(&gHaControlSem);

	return (state);
}

//=============================================================================
// Change HA State
//=============================================================================
HaState_t HaControl_ChangeState(uint8_t ha_number, HaState_t state)
{
	HaPort_t *port;
	HaState_t org_state, new_state;

	if (ha_number >= HA_CONTROL_NUM_PORT) {
		return (HA_STATE_ERROR);
	}
	port = &gHaPort[ha_number];

	_lwsem_wait(&gHaControlSem);

	org_state = HaControl_GetState_inner(ha_number);
	if (org_state == state) {
		_lwsem_post(&gHaControlSem);
		return (org_state);
	}

	/*** Pulse 200ms> ***/
	lwgpio_set_value(&port->c_signal, LWGPIO_VALUE_LOW);
	_time_delay(220);
	lwgpio_set_value(&port->c_signal, LWGPIO_VALUE_HIGH);
	_time_delay(130);

	new_state = HaControl_GetState_inner(ha_number);

	_lwsem_post(&gHaControlSem);

	return (new_state);
}

/******************************** END-OF-FILE ********************************/
