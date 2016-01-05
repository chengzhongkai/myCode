/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "mqtt.h"
#include "mqtt_prv.h"

//=============================================================================
// Create Handle
//=============================================================================
MQTT_Client_t *MQTT_CreateHandle(MQTT_NetIF_t *netif)
{
	MQTT_Client_t *client;

	client = MQTT_malloc(sizeof(MQTT_Client_t));
	if (client == NULL) {
		return (NULL);
	}
	MQTT_memset(client, 0, sizeof(MQTT_Client_t));

	MQTT_memcpy(&client->netif, netif, sizeof(MQTT_NetIF_t));

	MQTT_create_sem(client->send_sem);

	MQTT_InitPacketList(client);

	return (client);
}

//=============================================================================
// Release Handle
//=============================================================================
void MQTT_ReleaseHandle(MQTT_Client_t *client)
{
	if (client == NULL) return;

	/*************************/
	/*** RELEASE RESOURCES ***/
	/*************************/

	MQTT_free(client);
}

//=============================================================================
#define in_char(ch) (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))
bool MQTT_SetClientID(MQTT_Client_t *client, const char *client_id)
{
	int cnt;
	char ch;

	if (client == NULL || client_id == NULL) return (FALSE);

	// ------------------------------------------ Check Client ID Character ---
	for (cnt = 0; cnt < 24; cnt ++) {
		ch = client_id[cnt];
		if (ch == '\0') break;
		if (!in_char(ch)) return (FALSE);
	}
	if (cnt == 0 || cnt == 24) return (FALSE);

	MQTT_Str2UTF8(&client->client_id, client_id);

	return (TRUE);
}

//=============================================================================
void MQTT_RegisterRecvTopicCB(MQTT_Client_t *client,
							  MQTT_OnRecvTopicCB_t *on_recv_topic)
{
	if (client == NULL) return;

	client->on_recv_topic = on_recv_topic;
}

//=============================================================================
void MQTT_RegisterErrorCB(MQTT_Client_t *client,
						  MQTT_OnErrorCB_t *on_error)
{
	if (client == NULL) return;

	client->on_error = on_error;
}

//=============================================================================
void MQTT_RegisterSendPacketCB(MQTT_Client_t *client,
							   MQTT_OnSendPacketCB_t *on_send_pkt)
{
	if (client == NULL) return;

	client->on_send_pkt = on_send_pkt;
}

//=============================================================================
void MQTT_RegisterRecvAckCB(MQTT_Client_t *client,
							MQTT_OnRecvAckCB_t *on_recv_ack)
{
	if (client == NULL) return;

	client->on_recv_ack = on_recv_ack;
}

//=============================================================================
uint32_t MQTT_SendPacket(MQTT_Client_t *client, const uint8_t *buf)
{
	uint32_t len;
	uint32_t send_len;

	if (client == NULL || buf == NULL) return (0);

	len = MQTT_PacketLen(buf);

	MQTT_take_sem(client->send_sem);

	send_len = (*client->netif.send)(client->netif.handle, buf, len);

	MQTT_give_sem(client->send_sem);

	return (send_len);
}

//=============================================================================
uint32_t MQTT_ResendPacket(MQTT_Client_t *client, const uint8_t *buf)
{
	uint32_t len;
	uint32_t send_len;

	uint8_t cmd_id;
	uint8_t *payload;

	if (client == NULL || buf == NULL) return (0);

	len = MQTT_PacketLen(buf);

	cmd_id = MQTT_CmdId(buf);
	payload = (uint8_t *)MQTT_PacketPayload(buf);
	if (cmd_id == MQTT_CMD_ID_PUBLISH) {
		MQTT_SetDUP((uint8_t *)buf);
	}
	else {
		/*** not to be sent, if subscribe or unsubscribe ***/
		return (0);
	}

	MQTT_take_sem(client->send_sem);

	send_len = (*client->netif.send)(client->netif.handle, buf, len);

	MQTT_give_sem(client->send_sem);

	return (send_len);
}

//=============================================================================
uint32_t MQTT_RecvPacket(MQTT_Client_t *client,
						 uint8_t *buf, uint32_t max, uint32_t timeout)
{
	uint32_t recv_len;
	uint32_t recv_idx;

	uint32_t mul;
	uint32_t remain_len;
	uint32_t total_len;
	bool has_next;

	MQTT_assert(client != NULL && buf != NULL && max > 5);

	// ------------------------------- Receive First 1 Byte of Fixed Header ---
	recv_len = (*client->netif.recv)(client->netif.handle, buf, 1, timeout);
	if (SOCK_RecvError(recv_len)) {
		return (MQTT_RECV_ERROR);
	}

	// -------------------------- Receive Remaining-Length of Fiexed Header ---
	recv_idx = 1;
	mul = 1;
	remain_len = 0;
	has_next = FALSE;
	do {
		recv_len = (*client->netif.recv)(client->netif.handle,
										 &buf[recv_idx], 1, 0);
		if (SOCK_RecvError(recv_len)) {
			return (MQTT_RECV_ERROR);
		}
		remain_len += (buf[recv_idx] & 127) * mul;
		mul *= 128;
		if (mul > (128 * 128 * 128)) {
			/*** Too Large ***/
			return (MQTT_RECV_ERROR);
		}
		has_next = ((buf[recv_idx] & 0x80) != 0) ? TRUE : FALSE;
		recv_idx ++;
	} while (has_next);

	total_len = recv_idx + remain_len;
	if (total_len > max) {
		// Receive Buffer is short
		return (MQTT_RECV_ERROR);
	}

	// ------------------------------------------ Receive Remaining Payload ---
	while (remain_len > 0) {
		recv_len = (*client->netif.recv)(client->netif.handle,
										 &buf[recv_idx], remain_len, 0);
		if (SOCK_RecvError(recv_len)) {
			return (MQTT_RECV_ERROR);
		}
		remain_len -= recv_len;
	}

	return (total_len);
}

//=============================================================================
bool MQTT_Subscribe(MQTT_Client_t *client,
					const char *topic_str, MQTT_QOS_t qos)
{
	MQTT_UTF8_t topic;
	uint8_t *buf;
	uint32_t send_len;
	uint16_t packet_id;

	if (topic_str == NULL) return (FALSE);

	MQTT_Str2UTF8(&topic, topic_str);

	buf = MQTT_MakeSubscribePacket(&topic, qos);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	if (send_len == 0) {
		/*** ERROR ***/
		MQTT_free(buf);
		return (FALSE);
	}

	/*** Add this Topic to Subscribed-Topic-List ***/

	// ----------------------------------- Add this Packet to Wait-ACK-List ---
	packet_id = MQTT_Bytes2UInt16(MQTT_PacketPayload(buf));
	if (!(*client->on_send_pkt)(client, packet_id, buf)) {
		// ------------------------- Ignore whether ACK was recevied or not ---
		MQTT_free(buf);
	}

	return (TRUE);
}

//=============================================================================
bool MQTT_Unsubscribe(MQTT_Client_t *client, const char *topic_str)
{
	MQTT_UTF8_t topic;
	uint8_t *buf;
	uint32_t send_len;
	uint16_t packet_id;

	if (topic_str == NULL) return (FALSE);

	MQTT_Str2UTF8(&topic, topic_str);

	buf = MQTT_MakeUnsubscribePacket(&topic);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	if (send_len == 0) {
		/*** ERROR ***/
		MQTT_free(buf);
		return (FALSE);
	}

	/*** Remove this Topic from Subscribed-Topic-List ***/

	// ----------------------------------- Add this Packet to Wait-ACK-List ---
	packet_id = MQTT_Bytes2UInt16(MQTT_PacketPayload(buf));
	if (!(*client->on_send_pkt)(client, packet_id, buf)) {
		// ------------------------- Ignore whether ACK was recevied or not ---
		MQTT_free(buf);
	}

	return (TRUE);
}

//=============================================================================
bool MQTT_Publish(MQTT_Client_t *client, const char *topic_str,
				  MQTT_QOS_t qos, MQTT_Retain_t retain,
				  const uint8_t *msg_buf, uint32_t size)
{
	MQTT_UTF8_t topic;
	MQTT_Data_t message;
	uint8_t *buf;
	uint32_t send_len;
	uint16_t packet_id;

	// ---------------------------------------------------- Parameter Check ---
	if (client == NULL || topic_str == NULL) return (FALSE);
	if (qos < MQTT_QOS_0 || qos > MQTT_QOS_2) return (FALSE);

	// -------------------------------------------------------- Make Packet ---
	MQTT_Str2UTF8(&topic, topic_str);
	message.len = size;
	message.ptr = msg_buf;

	buf = MQTT_MakePublishPacket(&topic, &message, qos, retain, FALSE);
	if (buf == NULL) return (FALSE);

	// -------------------------------------------------------- Send Packet ---
	send_len = MQTT_SendPacket(client, buf);
	if (send_len == 0) {
		/*** ERROR ***/
		MQTT_free(buf);
		return (FALSE);
	}

	if (qos == MQTT_QOS_0) {
		MQTT_free(buf);
	} else {
		// ------------------------------- Add this Packet to Wait-ACK-List ---
		packet_id = MQTT_PubPacketId(MQTT_PacketPayload(buf));
		if (!(*client->on_send_pkt)(client, packet_id, buf)) {
			// --------------------- Ignore whether ACK was recevied or not ---
			MQTT_free(buf);
		}
	}

	return (TRUE);
}

//=============================================================================
bool MQTT_Disconnect(MQTT_Client_t *client)
{
	uint8_t *buf;
	uint32_t len, send_len;

	// ---------------------------------------------------- Parameter Check ---
	if (client == NULL) return (FALSE);

	// -------------------------------------------------------- Make Packet ---
	buf = MQTT_MakeDisconnectPacket();
	if (buf == NULL) return (FALSE);

	// -------------------------------------------------------- Send Packet ---
	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		/*** ERROR ***/
		return (FALSE);
	}

	// ----------------------------------------------- Shutdown Network I/F ---
	(*client->netif.close)(client->netif.handle);

	client->state = MQTT_STATE_CLOSED;

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
