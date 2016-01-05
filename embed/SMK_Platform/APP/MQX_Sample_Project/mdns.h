/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MDNS_H
#define _MDNS_H

#include <mqx.h>
#include <rtcs.h>
#include <ipcfg.h>

//=============================================================================
uint32_t MDNS_StartTask(const char *hostname, uint32_t priority);

#endif /* !_MDNS_H */

/******************************** END-OF-FILE ********************************/
