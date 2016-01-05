#ifndef _Telnet_user_H_
#define _Telnet_user_H_

#include <mqx.h>

// #define DEBUG_UPDATE

#define SHELL_COMMAND_USERNAME	"smk00"
#define SHELL_COMMAND_PASSWORD	"testmode"

/* task name */
#ifndef SHELL_TELNETD_TASK_NAME
#define SHELL_TELNETD_TASK_NAME	"Telnet_server"
#endif

/* priority */
#ifndef SHELL_TELNETD_PRIORITY
#define SHELL_TELNETD_PRIORITY	8
#endif

/* stack size */
#ifndef SHELL_TELNETD_STACK
#define SHELL_TELNETD_STACK		2000
#endif

/* Default UART test data */
#define SHELL_UART_TEST_DATA	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

/* command setting */
/* Green LED */
#ifndef GREEN_LED_EXIST
#define GREEN_LED_EXIST		0
#endif

/* Orange LED */
#ifndef ORANGE_LED_EXIST
#define ORANGE_LED_EXIST	0
#endif

/* Rain Sensor */
#ifndef RAIN_SENSOR_EXIST
#define RAIN_SENSOR_EXIST	0
#endif

/* UART1 */
#ifndef UART_1_EXIST
#define UART_1_EXIST	0
#endif

/* UART1 Speed */
#ifndef UART_1_DEBUG
#define UART_1_DEBUG	0
#elif !UART_1_EXIST
#undef UART_1_DEBUG
#define UART_1_DEBUG	0
#endif

/* RS232C(UART4) */
#ifndef RS232C_EXIST
#define RS232C_EXIST	0
#endif

/* RS232C(UART4) Speed */
#ifndef RS232C_DEBUG
#define RS232C_DEBUG	0
#elif !RS232C_EXIST
#undef RS232C_DEBUG
#define RS232C_DEBUG	0
#endif

_mqx_int initialize_Telnet(void);

#endif
