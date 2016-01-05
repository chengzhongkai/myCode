/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "mqtt.h"
#include "mqtt_prv.h"

#include "mqx_utils.h"

//=============================================================================
// Handle CONNACK
//=============================================================================
bool MQTT_HandleConnAck(const uint8_t *buf, uint32_t len)
{
	uint8_t cmd_id;
	uint32_t remain_len;
	const uint8_t *payload;

	if (buf == NULL) return (FALSE);

	cmd_id = MQTT_CmdId(buf);
	remain_len = MQTT_RemainLen(buf);
	if (cmd_id != MQTT_CMD_ID_CONNACK || remain_len != 2) {
		return (FALSE);
	}
	payload = MQTT_PacketPayload(buf);
	if (payload == NULL || payload[1] != 0x00) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Send PUBACK
//=============================================================================
static bool MQTT_SendPubAck(MQTT_Client_t *client, uint16_t packet_id)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakePubAckPacket(packet_id);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Send PUBREC
//=============================================================================
static bool MQTT_SendPubRec(MQTT_Client_t *client, uint16_t packet_id)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakePubRecPacket(packet_id);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Send PUBREL
//=============================================================================
static bool MQTT_SendPubRel(MQTT_Client_t *client, uint16_t packet_id)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakePubRelPacket(packet_id);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Send PUBCOMP
//=============================================================================
static bool MQTT_SendPubComp(MQTT_Client_t *client, uint16_t packet_id)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakePubCompPacket(packet_id);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Handle Received Packet
//=============================================================================
bool MQTT_HandlePacket(MQTT_Client_t *client, const uint8_t *buf, uint32_t len)
{
	uint8_t cmd_id;
	uint32_t remain_len;
	const uint8_t *payload;

	MQTT_QOS_t qos;
	MQTT_UTF8_t topic;
	MQTT_Data_t message;

	MQTT_assert(client != NULL && buf != NULL);

	if (len < 2) return (FALSE); // not MQTT Packet

	cmd_id = MQTT_CmdId(buf);
	remain_len = MQTT_RemainLen(buf);
	payload = MQTT_PacketPayload(buf);

	switch (cmd_id) {
	case MQTT_CMD_ID_CONNACK:
		// Already Received
		return (FALSE);

	case MQTT_CMD_ID_PUBLISH:
		// ------------------------------------------------- Handle PUBLISH ---
		qos = MQTT_QoS(buf);
		switch (qos) {
		case MQTT_QOS_0:
			message.ptr = MQTT_Bytes2UTF8(&topic, payload);
			message.len = remain_len - (topic.len + 2);
			// -------------------------------------- Notify Topic Received ---
			if (client->on_recv_topic != NULL) {
				(*client->on_recv_topic)(&topic, &message);
			}
			break;
		case MQTT_QOS_1:
			// ------------------------------------------------ Send PubAck ---
			if (!MQTT_SendPubAck(client, MQTT_PubPacketId(payload))) {
				return (FALSE);
			}
			message.ptr = MQTT_Bytes2UTF8(&topic, payload);
			message.ptr += 2; // skip Packet ID
			message.len = remain_len - (topic.len + 4);
			// Need to Check DUP Flag
			// -------------------------------------- Notify Topic Received ---
			if (client->on_recv_topic != NULL) {
				(*client->on_recv_topic)(&topic, &message);
			}
			break;
		case MQTT_QOS_2:
			// ------------------------------------------------ Send PubRec ---
			if (!MQTT_SendPubRec(client, MQTT_PubPacketId(payload))) {
				return (FALSE);
			}
			message.ptr = MQTT_Bytes2UTF8(&topic, payload);
			message.ptr += 2; // skip Packet ID
			message.len = remain_len - (topic.len + 4);
			// Need to Check DUP Flag
			// -------------------------------------- Notify Topic Received ---
			if (client->on_recv_topic != NULL) {
				(*client->on_recv_topic)(&topic, &message);
			}
			break;
		}
		break;

	case MQTT_CMD_ID_PUBACK:
		// ----------------------------------------- Notify PubAck Received ---
		if (client->on_recv_ack != NULL) {
			(*client->on_recv_ack)(client, MQTT_Bytes2UInt16(payload));
		}
		break;

	case MQTT_CMD_ID_PUBREC:
		// ---------------------------------------------------- Send PubRel ---
		if (!MQTT_SendPubRel(client, MQTT_Bytes2UInt16(payload))) {
			return (FALSE);
		}
		break;

	case MQTT_CMD_ID_PUBREL:
		// --------------------------------------------------- Send PubComp ---
		if (!MQTT_SendPubComp(client, MQTT_Bytes2UInt16(payload))) {
			return (FALSE);
		}
		break;

	case MQTT_CMD_ID_PUBCOMP:
		// ---------------------------------------- Notify PubComp Received ---
		if (client->on_recv_ack != NULL) {
			(*client->on_recv_ack)(client, MQTT_Bytes2UInt16(payload));
		}
		break;

	case MQTT_CMD_ID_SUBACK:
		// ----------------------------------------- Notify SubAck Received ---
		if (client->on_recv_ack != NULL) {
			(*client->on_recv_ack)(client, MQTT_Bytes2UInt16(payload));
		}
		break;

	case MQTT_CMD_ID_UNSUBACK:
		// --------------------------------------- Notify UnsubAck Received ---
		if (client->on_recv_ack != NULL) {
			(*client->on_recv_ack)(client, MQTT_Bytes2UInt16(payload));
		}
		break;

	case MQTT_CMD_ID_PINGRESP:
		// Dequeue Wait-Ack-List (QoS = ?)
		break;

	case MQTT_CMD_ID_CONNECT:
	case MQTT_CMD_ID_SUBSCRIBE:
	case MQTT_CMD_ID_UNSUBSCRIBE:
	case MQTT_CMD_ID_PINGREQ:
	case MQTT_CMD_ID_DISCONNECT:
		// for Broker(Server)
		return (FALSE);

	default:
		/*** Unknown Command ***/
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool MQTT_SendConnect(MQTT_Client_t *client)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakeConnectPacket(&client->client_id,
								 client->username,
								 client->password,
								 client->will_topic,
								 client->will_msg,
								 client->will_qos,
								 client->will_retain,
								 client->clean_session,
								 client->keep_alive);
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool MQTT_SendPingReq(MQTT_Client_t *client)
{
	uint8_t *buf;
	uint32_t send_len;

	buf = MQTT_MakePingReqPacket();
	if (buf == NULL) return (FALSE);

	send_len = MQTT_SendPacket(client, buf);
	MQTT_free(buf);
	if (send_len == 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
void MQTT_InitPacketList(MQTT_Client_t *client)
{
	int cnt;

	if (client == NULL) return;

	MQTT_create_sem(client->list_sem);

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		client->packet_list[cnt].retry = 0;
		client->packet_list[cnt].packet_id = 0;
		client->packet_list[cnt].buf = NULL;
		client->packet_list[cnt].remain_msec = 0;
	}
}

//=============================================================================
bool MQTT_AddPacketList(MQTT_Client_t *client,
						uint16_t packet_id, const uint8_t *buf,
						uint32_t timeout_msec)
{
	int cnt;
	bool ret = FALSE;

	if (client == NULL || packet_id == 0 || buf == NULL) return (FALSE);

	MQTT_take_sem(client->list_sem);

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id == packet_id) {
			// Already Added
			MQTT_give_sem(client->list_sem);
			return (FALSE);
		}
	}

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id == 0) {
			client->packet_list[cnt].retry = 0;
			client->packet_list[cnt].packet_id = packet_id;
			client->packet_list[cnt].buf = (uint8_t *)buf;
			client->packet_list[cnt].remain_msec = timeout_msec;
			ret = TRUE;
			break;
		}
	}

	MQTT_give_sem(client->list_sem);

	return (ret);
}

//=============================================================================
bool MQTT_DelPacketList(MQTT_Client_t *client, uint16_t packet_id)
{
	int cnt;
	bool ret = FALSE;

	if (client == NULL || packet_id == 0) return (FALSE);

	MQTT_take_sem(client->list_sem);

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id == packet_id) {
			client->packet_list[cnt].retry = 0;
			client->packet_list[cnt].packet_id = 0;
			if (client->packet_list[cnt].buf != NULL) {
				MQTT_free(client->packet_list[cnt].buf);
				client->packet_list[cnt].buf = NULL;
			}
			client->packet_list[cnt].remain_msec = 0;
			ret = TRUE;
			break;
		}
	}

	MQTT_give_sem(client->list_sem);

	return (ret);
}

//=============================================================================
uint32_t MQTT_GetPacketListTimeout(MQTT_Client_t *client)
{
	int cnt;
	uint32_t timeout = 0xffffffffL;

	if (client == NULL) return (0);

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id != 0) {
			if (client->packet_list[cnt].remain_msec < timeout) {
				timeout = client->packet_list[cnt].remain_msec;
			}
		}
	}

	if (timeout == 0xffffffffL) {
		/*** Not Need to Wait ***/
		return (0);
	}

	return (timeout);
}

//=============================================================================
void MQTT_ElapsePacketListTimeout(MQTT_Client_t *client, uint32_t msec)
{
	int cnt;

	if (client == NULL) return;

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id != 0) {
			if (client->packet_list[cnt].remain_msec < msec) {
				client->packet_list[cnt].remain_msec = 0;
			} else {
				client->packet_list[cnt].remain_msec -= msec;
			}
		}
	}
}

//=============================================================================
MQTT_PacketList_t *MQTT_GetTimeoutPacket(MQTT_Client_t *client)
{
	int cnt;

	if (client == NULL) return (0);

	for (cnt = 0; cnt < MQTT_PACKET_LIST_SIZE; cnt ++) {
		if (client->packet_list[cnt].packet_id != 0) {
			if (client->packet_list[cnt].remain_msec == 0) {
				return (&client->packet_list[cnt]);
			}
		}
	}

	return (NULL);
}

/******************************** END-OF-FILE ********************************/
