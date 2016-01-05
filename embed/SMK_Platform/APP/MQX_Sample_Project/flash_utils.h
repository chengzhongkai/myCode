/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _FLASH_UTILS_H
#define _FLASH_UTILS_H

#include <mqx.h>
#include <ipcfg.h>

#include "eeprom_api.h"
#include "Config.h"

//=============================================================================
enum {
	FLASHROM_MACADDR_ID = 0,
	FLASHROM_MACHINE_ID,
	FLASHROM_SERIAL_ID,
	FLASHROM_FTPID_ID,
	FLASHROM_SUBDOMAIN_ID,
	FLASHROM_PATH_ID,
	FLASHROM_DHCP_ID,
	FLASHROM_STATICIP_ID,
	FLASHROM_SUBNET_ID,
	FLASHROM_GATEWAY_ID,

	FLASHROM_SGWS_HOST_ID,
	FLASHROM_SGWS_PORTNO_ID,
	FLASHROM_SGWS_PATH_ID,

#ifdef EXEO_PLATFORM
	FLASHROM_CONFIG_ID,
	FLASHROM_CALIBRATION_ID,
	FLASHROM_MISC_ID,
#endif

	FLASHROM_MAX_ID
};


/* 0: MAC Address */
#define FLASHROM_MACADDR_ADDR	0
#define FLASHROM_MACADDR_SIZE	6
/* 1: Machine code */
#define FLASHROM_MACHINE_ADDR	6
#define FLASHROM_MACHINE_SIZE	12
/* 2: Serial number */
#define FLASHROM_SERIAL_ADDR	18
#define FLASHROM_SERIAL_SIZE	8
/* 3: FTP domain ID */
#define FLASHROM_FTPID_ADDR		26
#define FLASHROM_FTPID_SIZE		1
/* 4: FTP subdomain */
#define FLASHROM_SUBDOMAIN_ADDR	27
#define FLASHROM_SUBDOMAIN_SIZE	16
/* 5: FTP path */
#define FLASHROM_PATH_ADDR		43
#define FLASHROM_PATH_SIZE		32
/* 6: DHCP enable */
#define FLASHROM_DHCP_ADDR		75
#define FLASHROM_DHCP_SIZE		1
/* 7: Static IP Address */
#define FLASHROM_STATICIP_ADDR	76
#define FLASHROM_STATICIP_SIZE	4
/* 8: Static subnet mask */
#define FLASHROM_SUBNET_ADDR	80
#define FLASHROM_SUBNET_SIZE	4
/* 9: Static Gateway Address */
#define FLASHROM_GATEWAY_ADDR	84
#define FLASHROM_GATEWAY_SIZE	4


/* SGW Server Host Name */
#define FLASHROM_SGWS_HOST_ADDR	88
#define FLASHROM_SGWS_HOST_SIZE 32
/* SGW Server PortNo */
#define FLASHROM_SGWS_PORTNO_ADDR	120
#define FLASHROM_SGWS_PORTNO_SIZE	2
/* SGW Server Path */
#define FLASHROM_SGWS_PATH_ADDR	122
#define FLASHROM_SGWS_PATH_SIZE	32


#ifdef EXEO_PLATFORM
/* 10: Configration data (all of above data) */
#define FLASHROM_CONFIG_ADDR	0
#define FLASHROM_CONFIG_SIZE	88
/* 11: Sensor calibration data */
#define FLASHROM_CALIBRATION_ADDR	88
#define FLASHROM_CALIBRATION_SIZE	360
/* 12: Misc setting data */
#define FLASHROM_MISC_ADDR		448
#define FLASHROM_MISC_SIZE		2048
#endif

//=============================================================================
bool Flash_Init(void);
bool Flash_Get_Ethernet(IPCFG_IP_ADDRESS_DATA*,
						uint8_t*,
						uint8_t*);

#endif /* __FLASH_UTILS_H__ */

/******************************** END-OF-FILE ********************************/
