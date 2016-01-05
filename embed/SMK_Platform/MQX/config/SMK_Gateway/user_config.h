/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other restrictions.
*****************************************************************************
*
* Comments:
*
*   User configuration for MQX components
*
*
*END************************************************************************/

#ifndef __user_config_h__
#define __user_config_h__

/* mandatory CPU identification */
#define MQX_CPU                 PSP_CPU_MK64F120M

/* MGCT: <generated_code> */

/* Use I2C0 instead of UART4 */
//#define BSPCFG_ENABLE_I2C0       1
//#define BSPCFG_ENABLE_II2C0      1

/* Choose clock source for USB module */
#define BSPCFG_USB_USE_IRC48M    0 /* Do not use internal referent clock source */

#define BSPCFG_HAS_SRAM_POOL     1
#define BSPCFG_ENET_SRAM_BUF     1

#define MQX_USE_IDLE_TASK               1
#define MQX_ENABLE_LOW_POWER            1
#define MQXCFG_ENABLE_FP                1
#define MQX_INCLUDE_FLOATING_POINT_IO   1

#define RTCSCFG_ENABLE_ICMP      1
#define RTCSCFG_ENABLE_UDP       1
#define RTCSCFG_ENABLE_TCP       1
#define RTCSCFG_ENABLE_STATS     1
#define RTCSCFG_ENABLE_GATEWAYS  1
#define FTPDCFG_USES_MFS         1
#define RTCSCFG_ENABLE_SNMP      0

#define RTCSCFG_ENABLE_DNS       1
#define RTCSCFG_SOCKET_PART_INIT	8

#define TELNETDCFG_NOWAIT        FALSE

#define MQX_TASK_DESTRUCTION     1

#define HTTPDCFG_POLL_MODE       0
#define HTTPDCFG_STATIC_TASKS    0
#define HTTPDCFG_DYNAMIC_TASKS   1
/* MGCT: </generated_code> */

/*
** include common settings
*/

#define RTCSCFG_ENABLE_IGMP      1
#define MQX_USE_MESSAGES         1
#define MQX_USE_SEMAPHORES		 1
#define MQX_USE_TIMER            1

/* use the rest of defaults from small-RAM-device profile */
#include "small_ram_config.h"

/* and enable verification checks in kernel */
#include "verif_enabled_config.h"

#endif /* __user_config_h__ */
