/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _EEPROM_DEVICE_H
#define _EEPROM_DEVICE_H

#include "platform.h"

#include <flashx.h>
#include <flash_ftfe.h>

//=============================================================================
MQX_FILE *EEPROMDev_Open(void);
void EEPROMDev_Close(MQX_FILE *fd);

uint32_t EEPROMDev_Read(MQX_FILE *fd, uint32_t ofs, uint32_t len,
						uint8_t *buf, uint32_t size);
uint32_t EEPROMDev_Write(MQX_FILE *fd, uint32_t ofs, uint32_t len,
						 const uint8_t *buf, uint32_t size);

#endif /* !_EEPROM_DEVICE_H */

/******************************** END-OF-FILE ********************************/
