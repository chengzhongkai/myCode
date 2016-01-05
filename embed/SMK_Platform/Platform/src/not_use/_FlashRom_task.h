#ifndef FLASHROM_TASK_H_
#define FLASHROM_TASK_H_

#include "platform.h"

// #define FLASH_CLEAR

/* FlexNVM parttition setting */
#define FLEXNVM_EE_SPLIT	FLEXNVM_EE_SPLIT_1_1			/* EEPROM Split Factor */
#define FLEXNVM_EE_SIZE		FLEXNVM_EE_SIZE_4096			/* EEPROM Size */
#ifndef FLASH_CLEAR
	#define FLEXNVM_PARTITION	FLEXNVM_PART_CODE_DATA0_EE128	/* FlexNVM Partition Code */
#else
	#define FLEXNVM_PARTITION	50	/* FlexNVM Partition Code */
#endif
/* If you want to change partition, you have to erase entire internalflash memory before.
   See K64 Sub-Family Reference Manual 29.4.12.15,*/

/* Request command (in message) */
#define FLASHROM_REQUEST_READ			0			/* READ request */
#define FLASHROM_REQUEST_READ_RES		1			/* Response for READ request */
#define FLASHROM_REQUEST_READ_ERROR		0x80000001	/* Error Response for READ request */
#define FLASHROM_REQUEST_WRITE			2			/* WRITE request */
#define FLASHROM_REQUEST_WRITE_RES		3			/* Response for WRITE request */
#define FLASHROM_REQUEST_WRITE_ERROR	0x80000003	/* Error Response for WRITE request */
#define FLASHROM_REQUEST_ERROR_RES		0xFFFFFFFF	/* Response for unknown command */

/* Data */
/* MAC Address */
#define FLASHROM_MACADDR_ID		0
#define FLASHROM_MACADDR_ADDR	0
#define FLASHROM_MACADDR_SIZE	6
/* Machine code */
#define FLASHROM_MACHINE_ID		1
#define FLASHROM_MACHINE_ADDR	6
#define FLASHROM_MACHINE_SIZE	12
/* Serial number */
#define FLASHROM_SERIAL_ID		2
#define FLASHROM_SERIAL_ADDR	18
#define FLASHROM_SERIAL_SIZE	8
/* FTP domain ID */
#define FLASHROM_FTPID_ID		3
#define FLASHROM_FTPID_ADDR		26
#define FLASHROM_FTPID_SIZE		1
/* FTP subdomain */
#define FLASHROM_SUBDOMAIN_ID	4
#define FLASHROM_SUBDOMAIN_ADDR	27
#define FLASHROM_SUBDOMAIN_SIZE	16
/* FTP path */
#define FLASHROM_PATH_ID		5
#define FLASHROM_PATH_ADDR		43
#define FLASHROM_PATH_SIZE		32
/* DHCP enable */
#define FLASHROM_DHCP_ID		6
#define FLASHROM_DHCP_ADDR		75
#define FLASHROM_DHCP_SIZE		1
/* Static IP Address */
#define FLASHROM_STATICIP_ID	7
#define FLASHROM_STATICIP_ADDR	76
#define FLASHROM_STATICIP_SIZE	4
/* Static subnet mask */
#define FLASHROM_SUBNET_ID		8
#define FLASHROM_SUBNET_ADDR	80
#define FLASHROM_SUBNET_SIZE	4
/* Static Gateway Address */
#define FLASHROM_GATEWAY_ID		9
#define FLASHROM_GATEWAY_ADDR	84
#define FLASHROM_GATEWAY_SIZE	4
/* Configration data (all of above data) */
#define FLASHROM_CONFIG_ID		10
#define FLASHROM_CONFIG_ADDR	0
#define FLASHROM_CONFIG_SIZE	88
/* Sensor calibration data */
#define FLASHROM_CALIBRATION_ID		11
#define FLASHROM_CALIBRATION_ADDR	88
#define FLASHROM_CALIBRATION_SIZE	360
/* Misc setting data */
#define FLASHROM_MISC_ID		12
#define FLASHROM_MISC_ADDR		448
#define FLASHROM_MISC_SIZE		2048

extern _task_id FlashRom_GetTaskID(void);
extern _queue_id FlashRom_GetQueueID(void);

extern void FlashRom_task (uint32_t dummy);

#endif
