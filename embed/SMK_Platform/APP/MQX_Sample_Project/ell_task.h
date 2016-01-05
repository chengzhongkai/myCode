/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _ELL_TASK_H
#define _ELL_TASK_H

#include <mqx.h>
#include <rtcs.h>
#include <ipcfg.h>
#include <lwevent.h>

#include "echonetlitelite.h"
#include "connection.h"

//=============================================================================
#define ELL_RECV_BUF_SIZE (1024)
#define ELL_MAX_RECV_PACKET_QUEUE_SIZE (10)

//=============================================================================
typedef struct {
	MESSAGE_HEADER_STRUCT header;

	uint8_t *buf;
	int size;
	IPv4Addr_t addr;
} ELL_RecvPacketMsg_t;

//=============================================================================
bool_t ELL_SendPacket(void *handle, void *dest, const uint8_t *buf, int len);

void ELL_Task(uint32_t param);
void ELL_Handler_Task(uint32_t param);
void ELL_INF_Task(uint32_t param);

void ELL_StartTask(uint32_t flag);
#define ELL_TASK_START_PKT      0x0001
#define ELL_TASK_START_INF      0x0002
#define ELL_TASK_START_ADP_CTRL 0x0004
#define ELL_TASK_START_ADP_SYNC 0x0008
#define ELL_TASK_START_ALL      0x000f

void ELL_NotifyReset(void);

// Defined in ell_app.c
void ELL_AdpCtrl_Task(uint32_t param);
void ELL_AdpSync_Task(uint32_t param);

bool_t ELL_InitApp(void);
bool_t ELL_ConstructObjects(void);

#endif /* !_ELL_TASK_H */

/******************************** END-OF-FILE ********************************/
