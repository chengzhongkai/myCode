/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "flash_utils.h"
#include "eeprom_api.h"

#include "Config.h"

//=============================================================================
#ifndef EXEO_PLATFORM
#define EEPROM_MAP_TABLE_SIZE 10
#else
#define EEPROM_MAP_TABLE_SIZE 13
#endif
static const EEPROM_Table_t gEEPROMMapTable[EEPROM_MAP_TABLE_SIZE] =
{
	{ FLASHROM_MACADDR_ADDR,	FLASHROM_MACADDR_SIZE },
	{ FLASHROM_MACHINE_ADDR,	FLASHROM_MACHINE_SIZE },
	{ FLASHROM_SERIAL_ADDR,		FLASHROM_SERIAL_SIZE },
	{ FLASHROM_FTPID_ADDR,		FLASHROM_FTPID_SIZE },
	{ FLASHROM_SUBDOMAIN_ADDR,	FLASHROM_SUBDOMAIN_SIZE },
	{ FLASHROM_PATH_ADDR,		FLASHROM_PATH_SIZE },
	{ FLASHROM_DHCP_ADDR,		FLASHROM_DHCP_SIZE },
	{ FLASHROM_STATICIP_ADDR,	FLASHROM_STATICIP_SIZE },
	{ FLASHROM_SUBNET_ADDR,		FLASHROM_SUBNET_SIZE },
	{ FLASHROM_GATEWAY_ADDR,	FLASHROM_GATEWAY_SIZE },
#ifdef EXEO_PLATFORM
	{ FLASHROM_CONFIG_ADDR,		FLASHROM_CONFIG_SIZE },
	{ FLASHROM_CALIBRATION_ADDR,FLASHROM_CALIBRATION_SIZE },
	{ FLASHROM_MISC_ADDR,		FLASHROM_MISC_SIZE },
#endif
};

//=============================================================================
#define DATA_CLEAR {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}

//=============================================================================
typedef struct init_data
{
	uint32_t	DataID;
	uint32_t	DataSize;
	uint8_t		Data[32];
} INIT_DATA;

const INIT_DATA default_data[] = {
	{ FLASHROM_MACADDR_ID,	FLASHROM_MACADDR_SIZE,	DEFAULT_MAC_ADDR      },
	{ FLASHROM_MACHINE_ID,	FLASHROM_MACHINE_SIZE,	DATA_CLEAR            },
	{ FLASHROM_SERIAL_ID,	FLASHROM_SERIAL_SIZE,	DATA_CLEAR            },
	{ FLASHROM_FTPID_ID,	FLASHROM_FTPID_SIZE,	DEFAULT_FTP_DOMAIN    },
	{ FLASHROM_SUBDOMAIN_ID,FLASHROM_SUBDOMAIN_SIZE,DEFAULT_FTP_SUBDOMAIN },
	{ FLASHROM_PATH_ID,		FLASHROM_PATH_SIZE,		DEFAULT_FTP_PATH      },
	{ FLASHROM_DHCP_ID,		FLASHROM_DHCP_SIZE,		DEFAULT_DHCP_ENABLE   },
	{ FLASHROM_STATICIP_ID,	FLASHROM_STATICIP_SIZE,	DEFAULT_STATIC_IP     },
	{ FLASHROM_SUBNET_ID,	FLASHROM_SUBNET_SIZE,	DEFAULT_SEBNET_MASK   },
	{ FLASHROM_GATEWAY_ID,	FLASHROM_GATEWAY_SIZE,	DEFAULT_GATEWAY       },

	{ 0,					0,						DATA_CLEAR            },
};

//=============================================================================
void Flash_WriteDefaults(void)
{
	int idx;

	idx = 0;
	while (default_data[idx].Data != 0 && default_data[idx].DataSize != 0) {
		EEPROM_Write(default_data[idx].DataID,
					 default_data[idx].Data, default_data[idx].DataSize);
		idx ++;
	}
}

//=============================================================================
bool Flash_Init(void)
{
	uint8_t mac_addr[6];
	const uint8_t no_mac_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (EEPROM_Init(gEEPROMMapTable, EEPROM_MAP_TABLE_SIZE) != MQX_OK) {
		return (FALSE);
	}

	if (EEPROM_Read(FLASHROM_MACADDR_ID, mac_addr, 6) == 0) {
		return (FALSE);
	}

	if (memcmp(mac_addr, no_mac_addr, 6) == 0) {
		Flash_WriteDefaults();
	}

	return (TRUE);
}

//=============================================================================
bool Flash_Get_Ethernet(IPCFG_IP_ADDRESS_DATA *IP_data,
						uint8_t *MAC_addr, uint8_t *DHCP_mode)
{
	if (EEPROM_Read(FLASHROM_MACADDR_ID, MAC_addr, FLASHROM_MACADDR_SIZE) == 0) {
		return (FALSE);
	}

	if (EEPROM_Read(FLASHROM_DHCP_ID, DHCP_mode, FLASHROM_DHCP_SIZE) == 0) {
		return (FALSE);
	}

	if (EEPROM_Read(FLASHROM_STATICIP_ID, &(IP_data->ip), FLASHROM_STATICIP_SIZE) == 0) {
		return (FALSE);
	}

	if (EEPROM_Read(FLASHROM_SUBNET_ID, &(IP_data->mask), FLASHROM_SUBNET_SIZE) == 0) {
		return (FALSE);
	}

	if (EEPROM_Read(FLASHROM_GATEWAY_ID, &(IP_data->gateway), FLASHROM_GATEWAY_SIZE) == 0) {
		return (FALSE);
	}

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
