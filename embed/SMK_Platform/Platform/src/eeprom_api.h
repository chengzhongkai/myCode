/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _EEPROM_API_H
#define _EEPROM_API_H

#include <mqx.h>
#include <bsp.h>

//=============================================================================
typedef struct {
	uint32_t ofs;
	uint32_t len;
} EEPROM_Table_t;

//=============================================================================
uint32_t EEPROM_Init(const EEPROM_Table_t *table, uint32_t table_size);

uint32_t EEPROM_Size(uint32_t id);
uint32_t EEPROM_Write(uint32_t id, const void *buf, uint32_t size);
uint32_t EEPROM_Read(uint32_t id, void *buf, uint32_t size);

#endif /* !_EEPROM_API_H */

/******************************** END-OF-FILE ********************************/
