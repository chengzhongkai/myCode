/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "mqtt_task.h"

#include "mqx_utils.h"

#include "Config.h"
#include "Tasks.h"

//=============================================================================
static void MQTT_Shutdown(MQTT_Client_t *client);
static void MQTT_ReceiveTask(uint32_t param);
static void MQTT_KeepAliveTask(uint32_t param);
static void MQTT_WatchAckTask(uint32_t param);

static bool MQTT_NotifySendingPacket(MQTT_Client_t *client,
									 uint16_t packet_id, const uint8_t *buf);
static bool MQTT_NotifyAckPacket(MQTT_Client_t *client, uint16_t packet_id);

//=============================================================================
// Start Main Task
//=============================================================================
bool MQTT_StartTask(MQTT_Client_t *client, uint32_t priority)
{
	uint32_t err;
	MQTT_TaskInfo_t *task_info;

	// ------------------------------------------------- Register Callbacks ---
	MQTT_RegisterSendPacketCB(client,
							  (MQTT_OnSendPacketCB_t *)MQTT_NotifySendingPacket);
	MQTT_RegisterRecvAckCB(client,
						   (MQTT_OnRecvAckCB_t *)MQTT_NotifyAckPacket);

	// ------------------------------------------ Alloc Task Dependent Info ---
	task_info = _mem_alloc(sizeof(MQTT_TaskInfo_t));
	if (task_info == NULL) {
		return (FALSE);
	}
	memset(task_info, 0, sizeof(MQTT_TaskInfo_t));
	task_info->msg_pool_id = MSGPOOL_NULL_POOL_ID;
	task_info->msgq_id = MSGQ_NULL_QUEUE_ID;
	client->task_info = task_info;

	// ---------------------------------------------- Create and Start Task ---
	task_info->task_pri = priority;
	err = MQX_CreateTask(&task_info->main_tid,
						 MQTT_ReceiveTask, MQTT_TASK_STACK_SIZE,
						 priority, MQTT_TASK_NAME, (uint32_t)client);
	if (err != MQX_OK) {
		MQTT_Shutdown(client);
		client->state = MQTT_STATE_ERROR;
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Stop Main Task
//=============================================================================
bool MQTT_StopTask(MQTT_Client_t *client)
{
	/*** not implemented ***/
	return (FALSE);
}

//=============================================================================
// Start Keep Alive Task
//=============================================================================
static bool MQTT_StartKeepAliveTask(MQTT_Client_t *client, uint32_t priority)
{
	uint32_t err;
	MQTT_TaskInfo_t *task_info;

	if (client == NULL) return (FALSE);

	task_info = (MQTT_TaskInfo_t *)client->task_info;

	// ---------------------------------------------- Create and Start Task ---
	err = MQX_CreateTask(&task_info->keep_alive_tid,
						 MQTT_KeepAliveTask, MQTT_KA_TASK_STACK_SIZE,
						 priority, MQTT_KA_TASK_NAME, (uint32_t)client);
	if (err != MQX_OK) {
		MQTT_Disconnect(client);
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Start Watch ACK Task
//=============================================================================
static bool MQTT_StartWatchAckTask(MQTT_Client_t *client, uint32_t priority)
{
	uint32_t err;
	MQTT_TaskInfo_t *task_info;

	if (client == NULL) return (FALSE);

	task_info = (MQTT_TaskInfo_t *)client->task_info;

	// ---------------------------------------------- Create and Start Task ---
	err = MQX_CreateTask(&task_info->watch_ack_tid,
						 MQTT_WatchAckTask, MQTT_WA_TASK_STACK_SIZE,
						 priority, MQTT_WA_TASK_NAME, (uint32_t)client);
	if (err != MQX_OK) {
		MQTT_Disconnect(client);
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Call Connection Shutdown Method
//=============================================================================
static void MQTT_Shutdown(MQTT_Client_t *client)
{
	(*client->netif.close)(client->netif.handle);
}

//=============================================================================
// Receive Packet Task
//=============================================================================
static void MQTT_ReceiveTask(uint32_t param)
{
	MQTT_Client_t *client = (MQTT_Client_t *)param;
	MQTT_TaskInfo_t *task_info;

	uint8_t *recv_buf;
	uint32_t recv_len;

	if (client == NULL) return;
	task_info = (MQTT_TaskInfo_t *)client->task_info;
	if (task_info == NULL) return;

	// ----------------------------------------------- Alloc Receive Buffer ---
	recv_buf = MQTT_malloc(MQTT_RECV_BUFFER_SIZE);
	if (recv_buf == NULL) {
		/*** Memory Exhausted ***/
		goto MQTT_RecvTask_err;
	}

	// ------------------------------------------------------- Send CONNECT ---
	if (!MQTT_SendConnect(client)) {
		goto MQTT_RecvTask_err;
	}

	// ------------------------------------------------------- Wait CONNACK ---
	client->state = MQTT_STATE_CONNECTING;
	recv_len = MQTT_RecvPacket(client, recv_buf, MQTT_RECV_BUFFER_SIZE, 0);
	if (recv_len == MQTT_RECV_ERROR || recv_len == MQTT_RECV_TIMEOUT) {
		goto MQTT_RecvTask_err;
	}

	// ----------------------------------------------------- Handle CONNACK ---
	if (!MQTT_HandleConnAck(recv_buf, recv_len)) {
		goto MQTT_RecvTask_err;
	}

	/*** Check ACK Flag (Session is retained or not) ***/

	client->state = MQTT_STATE_CONNECTED;

	// ---------------------------------------------- Start Keep Alive Task ---
	if (client->keep_alive > 0) {
		MQTT_StartKeepAliveTask(client, task_info->task_pri + 1);
	}

	// ----------------------------------------------- Start Watch ACK Task ---
	MQTT_StartWatchAckTask(client, task_info->task_pri + 1);

	// ------------------------------------------ Receive and Handle Packet ---
	while (client->state == MQTT_STATE_CONNECTED) {
		recv_len = MQTT_RecvPacket(client, recv_buf, MQTT_RECV_BUFFER_SIZE, 0);
		if (recv_len == MQTT_RECV_ERROR) {
			/*** ERROR ***/
			break;
		}
		if (recv_len != MQTT_RECV_TIMEOUT) {
			if (!MQTT_HandlePacket(client, recv_buf, recv_len)) {
				/*** ERROR ***/
				break;
			}
		}
	}

	if (client->state != MQTT_STATE_CLOSED) {
		MQTT_Disconnect(client);
	}

	if (recv_buf != NULL) {
		MQTT_free(recv_buf);
	}
	return;

MQTT_RecvTask_err:

	client->state = MQTT_STATE_ERROR;
	MQTT_Shutdown(client);

	if (recv_buf != NULL) {
		MQTT_free(recv_buf);
	}
}

//=============================================================================
// Keep Alive Task
//=============================================================================
static void MQTT_KeepAliveTask(uint32_t param)
{
	MQTT_Client_t *client = (MQTT_Client_t *)param;

	if (client == NULL || client->keep_alive == 0) return;

	// ---------------------------------------------------------- Main Loop ---
	_time_delay((uint32_t)client->keep_alive * 1000);
	while (client->state == MQTT_STATE_CONNECTED) {
		if (!MQTT_SendPingReq(client)) {
			break;
		}
		_time_delay((uint32_t)client->keep_alive * 1000);
	}
}

//=============================================================================
// Callback to Notify Sending Packet With Packet ID
//=============================================================================
static bool MQTT_NotifySendingPacket(MQTT_Client_t *client,
									 uint16_t packet_id, const uint8_t *buf)
{
	MQTT_WatchAckMsg_t *msg;
	MQTT_TaskInfo_t *task_info;

	if (client == NULL) return (FALSE);
	if (packet_id == 0 || buf == NULL) return (FALSE);

	task_info = (MQTT_TaskInfo_t *)client->task_info;
	if (task_info->msg_pool_id == MSGPOOL_NULL_POOL_ID
		|| task_info->msgq_id == MSGQ_NULL_QUEUE_ID) return (FALSE);

	msg = _msg_alloc(task_info->msg_pool_id);
	if (msg == NULL) {
		return (FALSE);
	}

	msg->header.SOURCE_QID = 0;
	msg->header.TARGET_QID = task_info->msgq_id;
	msg->header.SIZE = sizeof(MQTT_WatchAckMsg_t);

	msg->cmd = MQTT_WATCH_ACK_RESCHEDULE;

	msg->packet_id = packet_id;
	msg->buf = (uint8_t *)buf;
	msg->timeout = 3000; /***/

	if (!_msgq_send(msg)) {
		err_printf("[MQTT] Cannot Send Message\n");
		_msg_free(msg);
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Callback to Notify ACK Packet Received
//=============================================================================
static bool MQTT_NotifyAckPacket(MQTT_Client_t *client, uint16_t packet_id)
{
	MQTT_WatchAckMsg_t *msg;
	MQTT_TaskInfo_t *task_info;

	if (client == NULL) return (FALSE);

	task_info = (MQTT_TaskInfo_t *)client->task_info;
	if (task_info->msg_pool_id == MSGPOOL_NULL_POOL_ID
		|| task_info->msgq_id == MSGQ_NULL_QUEUE_ID) return (FALSE);

	msg = _msg_alloc(task_info->msg_pool_id);
	if (msg == NULL) {
		return (FALSE);
	}

	msg->header.SOURCE_QID = 0;
	msg->header.TARGET_QID = task_info->msgq_id;
	msg->header.SIZE = sizeof(MQTT_WatchAckMsg_t);

	msg->cmd = MQTT_WATCH_ACK_RECV_ACK;

	msg->packet_id = packet_id;
	msg->buf = NULL;
	msg->timeout = 0;

	if (!_msgq_send(msg)) {
		err_printf("[MQTT] Cannot Send Message\n");
		_msg_free(msg);
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Watch ACK Task
//=============================================================================
static void MQTT_WatchAckTask(uint32_t param)
{
	MQTT_Client_t *client = (MQTT_Client_t *)param;
	MQTT_TaskInfo_t *task_info;

	uint32_t timeout;
	MQTT_WatchAckMsg_t *msg;
	MQTT_PacketList_t *packet;

	TIME_STRUCT start, cur, diff;
	uint32_t diff_msec;

	if (client == NULL) return;
	task_info = (MQTT_TaskInfo_t *)client->task_info;
	if (task_info == NULL) return;

	// ------------------------------------------------ Create Message Pool ---
	task_info->msg_pool_id = _msgpool_create(sizeof(MQTT_WatchAckMsg_t),
											 MQTT_WATCH_ACK_MSGQ_SIZE,
										   0, 0);
	if (task_info->msg_pool_id == MSGPOOL_NULL_POOL_ID) {
		err_printf("[MQTT] Cannot Create Message Pool\n");
		_task_block();
	}

	// ------------------------------------------------- Open Message Queue ---
	task_info->msgq_id = _msgq_open(MQTT_WATCH_ACK_MESSAGE_QUEUE,
									MQTT_WATCH_ACK_MSGQ_SIZE);
	if (task_info->msgq_id == MSGQ_NULL_QUEUE_ID) {
		err_printf("[MQTT] Cannot Open Message Queue\n");
		_task_block();
	}

	// ---------------------------------------------------------- Main Loop ---
	while (client->state == MQTT_STATE_CONNECTED) {
		timeout = MQTT_GetPacketListTimeout(client);

		_time_get(&start);
		msg = _msgq_receive(task_info->msgq_id, timeout);
		if (msg != NULL) {
			// ------------------------------------- Elapse Time Previously ---
			if (timeout > 0) {
				_time_get(&cur);
				_time_diff(&start, &cur, &diff);
				diff_msec = (diff.SECONDS * 1000) + diff.MILLISECONDS;

				if (diff_msec > timeout) {
					/*** 'diff_msec' should be less than 'timeout' ***/
					err_printf("[MQTT] Elapsed Time is Strange (timeout=%dms, diff=%dms)\n", timeout, diff_msec);
				}

				MQTT_ElapsePacketListTimeout(client, diff_msec);
			}

			// ------------------------------------ Receive Request Message ---
			switch (msg->cmd) {
			case MQTT_WATCH_ACK_RESCHEDULE:
				if (!MQTT_AddPacketList(client, msg->packet_id, msg->buf,
										msg->timeout)) {
					/*** Failed to Add ***/
					_mem_free(msg->buf);
				}
				break;
			case MQTT_WATCH_ACK_RECV_ACK:
				MQTT_DelPacketList(client, msg->packet_id);
				break;
			}
			_msg_free(msg);
		} else if (timeout != 0) {
			// ------------------------------------------- Wait ACK Timeout ---
			MQTT_ElapsePacketListTimeout(client, timeout);
		}

		// -------------------------------------------------------- Timeout ---
		while ((packet = MQTT_GetTimeoutPacket(client)) != NULL) {
			// Remove or Retry Once
			packet->retry ++;
			if (packet->retry > 1
				|| MQTT_ResendPacket(client, packet->buf) == 0) {
				MQTT_DelPacketList(client, packet->packet_id);
			}
		}
	}
}

/******************************** END-OF-FILE ********************************/
