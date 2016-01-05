/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H

#include "platform.h"

//=============================================================================
// Message Queue Number Definitions
//=============================================================================
enum {
	// followings are for Platform
    MAIN_TASK_MESSAGE_QUEUE = 9,

    NET_CONNECT_TASK_MESSAGE_QUEUE,
    DHCP_CHECK_MESSAGE_QUEUE,

    INIT_TASK_MESSAGE_QUEUE,
    FTP_MESSAGE_QUEUE,
    TELNET_MESSAGE_QUEUE,

	// followings are for Application
	ELL_RECV_PACKET_QUEUE,

	ELL_ADP_CTRL_REQ_QUEUE,

	APP_LAST_QUEUE
};

//=============================================================================
// EEPROM Default Initialize Values
//=============================================================================
#define DEFAULT_MAC_ADDR      {0x00, 0x01, 0x90, 0xF0, 0x95, 0x00}
#define DEFAULT_DHCP_ENABLE   {0}
#define DEFAULT_STATIC_IP     {100,   1, 254, 169}
#define DEFAULT_SEBNET_MASK   {  0,   0, 255, 255}
#define DEFAULT_GATEWAY       {  1,   1, 254, 169}
#define DEFAULT_FTP_DOMAIN    {0}
#define DEFAULT_FTP_SUBDOMAIN {'s','l','b','-','f','t','p'}
#define DEFAULT_FTP_PATH      {'a','p','p'}

//=============================================================================
// Hardware Resouces
//=============================================================================
#define GREEN_LED_EXIST		0
#define ORANGE_LED_EXIST	1
#define RAIN_SENSOR_EXIST	0
#define JEMA_Control_EXIST	1
#define UART_1_EXIST		0
#define RS232C_EXIST		0
#define UART_1_DEBUG		0
#define RS232C_DEBUG		0

//=============================================================================
// Device to Build
//=============================================================================

// Adapter Type
#define APP_ENL_ADAPTER
// #define APP_MID_ADAPTER

#define APP_MAIN_NAME "SwBox Type Ethernet Adapter"

#if defined(APP_ENL_ADAPTER)
#define APP_SUB_NAME "ECHONET Lite Adapter (HA)"
#elif defined(APP_MID_ADAPTER)
#define APP_SUB_NAME "ECHONET Lite Middleware Adapter"
#else
#error need to define APP_ENL_ADAPTER or APP_MID_ADAPTER
#endif

// Firmware
// #define APP_OKAYA_HAx1_AIRCON
// #define APP_OKAYA_HAx1_WATER_HEATER
// #define APP_OKAYA_HAx1_FLOOR_HEATER
#define APP_OKAYA_HAx1_ELECTRIC_LOCK

#if defined(APP_OKAYA_HAx1_AIRCON)
  #define APP_FW_NAME "OKAYA HA Switch (Aircon)"
#elif defined(APP_OKAYA_HAx1_WATER_HEATER)
  #define APP_FW_NAME "OKAYA HA Switch (Water Heater)"
#elif defined(APP_OKAYA_HAx1_FLOOR_HEATER)
  #define APP_FW_NAME "OKAYA HA Switch (Floor Heater)"
#elif defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
  #define APP_FW_NAME "OKAYA HA Electric Lock"
#endif

//=============================================================================
// Version Number and Build Date (BCD Coded)
//=============================================================================
#if defined(APP_OKAYA_HAx1_AIRCON)
  #define VERSION_MAJOR	0x01	/* Major version */		
#elif defined(APP_OKAYA_HAx1_WATER_HEATER)
  #define VERSION_MAJOR	0x03	/* Major version */		
#elif defined(APP_OKAYA_HAx1_FLOOR_HEATER)
  #define VERSION_MAJOR	0x04	/* Major version */		
#elif defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
  #define VERSION_MAJOR	0x02	/* Major version */		
#endif

#define VERSION_MINOR	0x00	/* Minor version */
#define VERSION_YEAR	0x2015	/* Year */
#define VERSION_MONTH	0x04	/* Month */
#define VERSION_DATE	0x20	/* Date */


//=============================================================================
// Build Switches
//=============================================================================

// Use HTTP Server (0 = Not Use, 1 = Use)
#define APP_USE_HTTP_SERVER 0

// Use LED Task (0 = Not Use, 1 = Use)
#define APP_USE_LED_TASK 1

// Debug (0 = No Debug, 1 = Debug)
#define APP_DEBUG 0

#endif /* _CONFIG_H */

/******************************** END-OF-FILE ********************************/
