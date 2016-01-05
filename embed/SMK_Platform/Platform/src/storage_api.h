/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _STORAGE_API_H
#define _STORAGE_API_H

#include "platform.h"

//=============================================================================
MQX_FILE *Storage_Open(void);
void Storage_Close(MQX_FILE *fd);

uint32_t Storage_Read(MQX_FILE *fd,
					  uint32_t addr, uint32_t size, uint8_t *buf);
uint32_t Storage_Write(MQX_FILE *fd,
					   uint32_t addr, uint32_t size, const uint8_t *buf);

bool Storage_EraseSector(MQX_FILE *fd, uint32_t start_sec, uint32_t end_sec);
bool Storage_IsEmpty(MQX_FILE *fd, uint32_t addr, uint32_t size);

#endif /* !_STORAGE_API_H */

/******************************** END-OF-FILE ********************************/
