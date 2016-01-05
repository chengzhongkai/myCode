/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MQTT_TASK_H
#define _MQTT_TASK_H

#include <mqx.h>
#include <rtcs.h>
#include <ipcfg.h>
#include <lwevent.h>

#include "platform.h"
#include "mqtt.h"

//=============================================================================
#define MQTT_TASK_STACK_SIZE 2048
#define MQTT_TASK_NAME "MQTT Recv"
#define MQTT_KA_TASK_STACK_SIZE 1024
#define MQTT_KA_TASK_NAME "MQTT Keep Alive"
#define MQTT_WA_TASK_STACK_SIZE 1024
#define MQTT_WA_TASK_NAME "MQTT Watch ACK"

#define MQTT_WATCH_ACK_MSGQ_SIZE (10)

//=============================================================================
typedef enum {
	MQTT_WATCH_ACK_RESCHEDULE,
	MQTT_WATCH_ACK_RECV_ACK
} MQTT_WatchAckCmd_t;

//=============================================================================
typedef struct {
	MESSAGE_HEADER_STRUCT header;

	MQTT_WatchAckCmd_t cmd;
	uint16_t packet_id;
	uint8_t *buf;
	uint32_t timeout;
} MQTT_WatchAckMsg_t;

//=============================================================================
typedef struct {
	uint32_t task_pri;
	_task_id main_tid;
	_task_id keep_alive_tid;
	_task_id watch_ack_tid;

	_pool_id msg_pool_id;
	_queue_id msgq_id;
} MQTT_TaskInfo_t;

//=============================================================================
bool MQTT_StartTask(MQTT_Client_t *client, uint32_t priority);

#endif /* !_MQTT_TASK_H */

/******************************** END-OF-FILE ********************************/
