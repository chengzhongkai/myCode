/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MQTT_H
#define _MQTT_H

#include <mqx.h>
#include <bsp.h>

#include "mqtt_dep.h"

//=============================================================================
#define MQTT_VERSION "3.1.1"

#define MQTT_RECV_BUFFER_SIZE 4096

//=============================================================================
#if 0
typedef enum {
	MQTT_CONNECT_TCP_IP,
	MQTT_CONNECT_TLS,
	MQTT_CONNECT_WEBSOCKET
} MQTT_Connection_t;
#endif

//=============================================================================
typedef struct {
	uint16_t len;
	const uint8_t *ptr;
} MQTT_UTF8_t;

//=============================================================================
typedef struct {
	uint16_t len;
	const uint8_t *ptr;
} MQTT_Data_t;

//=============================================================================
typedef enum {
	MQTT_QOS_0 = 0,
	MQTT_QOS_1 = 1,
	MQTT_QOS_2 = 2
} MQTT_QOS_t;

//=============================================================================
typedef enum {
	MQTT_NOT_RETAIN = 0,
	MQTT_RETAIN = 1
} MQTT_Retain_t;

//=============================================================================
typedef uint32_t MQTT_SendCB_t(void *handle, const uint8_t *buf, uint32_t len);
typedef uint32_t MQTT_RecvCB_t(void *handle, uint8_t *buf, uint32_t len, uint32_t timeout);
typedef void MQTT_CloseCB_t(void *handle);

//=============================================================================
typedef struct {
	void *handle;
	MQTT_SendCB_t *send;
	MQTT_RecvCB_t *recv;
	MQTT_CloseCB_t *close;
} MQTT_NetIF_t;

//=============================================================================
typedef void MQTT_OnRecvTopicCB_t(MQTT_UTF8_t *topic, MQTT_Data_t *message);
typedef void MQTT_OnErrorCB_t(uint32_t err_code);

typedef bool MQTT_OnSendPacketCB_t(void *client,
								   uint16_t packet_id, const uint8_t *buf);
typedef bool MQTT_OnRecvAckCB_t(void *client, uint16_t packet_id);

//=============================================================================
typedef struct {
	uint16_t retry;
	uint16_t packet_id;
	uint8_t *buf;
	uint32_t remain_msec;
} MQTT_PacketList_t;
#define MQTT_PACKET_LIST_SIZE 4

//=============================================================================
typedef enum {
	MQTT_STATE_NONE = 0,
	MQTT_STATE_CONNECTING,
	MQTT_STATE_CONNECTED,
	MQTT_STATE_CLOSED,
	MQTT_STATE_ERROR
} MQTT_State_t;

//=============================================================================
typedef struct _mqtt_client_t {

	MQTT_State_t state;

	void *on_recv;

	MQTT_NetIF_t netif;
	mqtt_semaphore_t send_sem;

	MQTT_OnRecvTopicCB_t *on_recv_topic;
	MQTT_OnErrorCB_t *on_error;

	MQTT_OnSendPacketCB_t *on_send_pkt;
	MQTT_OnRecvAckCB_t *on_recv_ack;

	MQTT_UTF8_t client_id;
	MQTT_UTF8_t *username;
	MQTT_Data_t *password;
	MQTT_UTF8_t *will_topic;
	MQTT_Data_t *will_msg;
	MQTT_QOS_t will_qos;
	MQTT_Retain_t will_retain;
	bool clean_session;
	uint16_t keep_alive;

	void *task_info;

	MQTT_PacketList_t packet_list[MQTT_PACKET_LIST_SIZE];
	mqtt_semaphore_t list_sem;
} MQTT_Client_t;

//=============================================================================
// mqtt_api.c
//=============================================================================
MQTT_Client_t *MQTT_CreateHandle(MQTT_NetIF_t *netif);
void MQTT_ReleaseHandle(MQTT_Client_t *client);

bool MQTT_SetClientID(MQTT_Client_t *client, const char *client_id);

void MQTT_RegisterRecvTopicCB(MQTT_Client_t *client,
							  MQTT_OnRecvTopicCB_t *on_recv_topic);
void MQTT_RegisterErrorCB(MQTT_Client_t *client,
						  MQTT_OnErrorCB_t *on_error);
void MQTT_RegisterSendPacketCB(MQTT_Client_t *client,
							   MQTT_OnSendPacketCB_t *on_send_pkt);
void MQTT_RegisterRecvAckCB(MQTT_Client_t *client,
							MQTT_OnRecvAckCB_t *on_recv_ack);

uint32_t MQTT_SendPacket(MQTT_Client_t *client, const uint8_t *buf);
uint32_t MQTT_ResendPacket(MQTT_Client_t *client, const uint8_t *buf);
uint32_t MQTT_RecvPacket(MQTT_Client_t *client,
						 uint8_t *buf, uint32_t max, uint32_t timeout);

bool MQTT_Subscribe(MQTT_Client_t *client,
					const char *topic_str, MQTT_QOS_t qos);
bool MQTT_Unsubscribe(MQTT_Client_t *client, const char *topic_str);
bool MQTT_Publish(MQTT_Client_t *client, const char *topic_str,
				  MQTT_QOS_t qos, MQTT_Retain_t retain,
				  const uint8_t *msg_buf, uint32_t size);
bool MQTT_Disconnect(MQTT_Client_t *client);

//=============================================================================
// mqtt_handle.c
//=============================================================================
bool MQTT_HandleConnAck(const uint8_t *buf, uint32_t len);
bool MQTT_HandlePacket(MQTT_Client_t *client, const uint8_t *buf, uint32_t len);
bool MQTT_SendConnect(MQTT_Client_t *client);
bool MQTT_SendPingReq(MQTT_Client_t *client);

void MQTT_InitPacketList(MQTT_Client_t *client);
bool MQTT_AddPacketList(MQTT_Client_t *client,
						uint16_t packet_id, const uint8_t *buf,
						uint32_t timeout_msec);
bool MQTT_DelPacketList(MQTT_Client_t *client, uint16_t packet_id);
uint32_t MQTT_GetPacketListTimeout(MQTT_Client_t *client);
void MQTT_ElapsePacketListTimeout(MQTT_Client_t *client, uint32_t msec);
MQTT_PacketList_t *MQTT_GetTimeoutPacket(MQTT_Client_t *client);

#endif /* !_MQTT_H */

/******************************** END-OF-FILE ********************************/
