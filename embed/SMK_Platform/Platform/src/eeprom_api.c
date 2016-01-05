/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "eeprom_device.h"
#include "eeprom_api.h"

//=============================================================================
static const EEPROM_Table_t *gEEPROMTable = NULL;
static uint32_t gEEPROMTableSize = 0;

static MQX_FILE *gEEPROMDev = NULL;

static LWSEM_STRUCT gEEPROMSem;

//=============================================================================
uint32_t EEPROM_Init(const EEPROM_Table_t *table, uint32_t table_size)
{
	gEEPROMTable = table;
	gEEPROMTableSize = table_size;

	gEEPROMDev = EEPROMDev_Open();
	if (gEEPROMDev == NULL) {
		return (MQX_ERROR);
	}

	_lwsem_create(&gEEPROMSem, 1);

	return (MQX_OK);
}

//=============================================================================
uint32_t EEPROM_Size(uint32_t id)
{
	if (gEEPROMDev == NULL) return (0);
	if (gEEPROMTable == NULL
		|| gEEPROMTableSize == 0 || id >= gEEPROMTableSize) {
		return (0);
	}

	return (gEEPROMTable[id].len);
}

//=============================================================================
uint32_t EEPROM_Write(uint32_t id, const void *buf, uint32_t size)
{
	uint32_t write_size;

	if (gEEPROMDev == NULL) return (0);
	if (size <= 0 || buf == NULL) return (0);
	if (gEEPROMTable == NULL
		|| gEEPROMTableSize == 0 || id >= gEEPROMTableSize) {
		return (0);
	}

	_lwsem_wait(&gEEPROMSem);

	write_size = EEPROMDev_Write(gEEPROMDev,
								 gEEPROMTable[id].ofs, gEEPROMTable[id].len,
								 buf, size);

	_lwsem_post(&gEEPROMSem);

	return (write_size);
}

//=============================================================================
uint32_t EEPROM_Read(uint32_t id, void *buf, uint32_t size)
{
	uint32_t read_size;

	if (gEEPROMDev == NULL) return (0);
	if (size <= 0 || buf == NULL) return (0);
	if (gEEPROMTable == NULL
		|| gEEPROMTableSize == 0 || id >= gEEPROMTableSize) {
		return (0);
	}

	_lwsem_wait(&gEEPROMSem);

	read_size = EEPROMDev_Read(gEEPROMDev,
							   gEEPROMTable[id].ofs, gEEPROMTable[id].len,
							   buf, size);

	_lwsem_post(&gEEPROMSem);

	return (read_size);
}

/******************************** END-OF-FILE ********************************/
