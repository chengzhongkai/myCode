/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "mqtt.h"
#include "mqtt_prv.h"

//=============================================================================
#define utf8_is_null(str) (((str) == NULL) || ((str)->len == 0) || ((str)->ptr == NULL))
#define data_is_null(str) (((str) == NULL) || ((str)->len == 0) || ((str)->ptr == NULL))

//=============================================================================
static uint16_t gMQTT_PacketID = 1;

//=============================================================================
static uint32_t fixed_header_size(uint32_t remain_len)
{
	if (remain_len <= 127) {
		return (2);
	} else if (remain_len <= 16383) {
		return (3);
	} else if (remain_len <= 2097151) {
		return (4);
	} else {
		return (5);
	}
}

//=============================================================================
// Having Enough Buffer Size
//=============================================================================
static uint8_t *set_fixed_header(uint8_t *buf,
								 uint8_t cmd, uint32_t remain_len)
{
	*buf = cmd;
	buf ++;
	do {
		*buf = remain_len & 127;
		remain_len /= 128;
		if (remain_len > 0) {
			*buf |= 0x80;
		}
		buf ++;
	} while (remain_len > 0);

	return (buf);
}

//=============================================================================
static uint8_t *copy_uint16(uint8_t *buf, uint16_t num)
{
	*buf = (num >> 8) & 0xff;
	buf ++;
	*buf = num & 0xff;
	buf ++;

	return (buf);
}

//=============================================================================
static uint8_t *copy_utf8(uint8_t *buf, const MQTT_UTF8_t *utf8_str)
{
	if (utf8_is_null(utf8_str)) {
		*buf = 0;
		buf ++;
		*buf = 0;
		buf ++;
		return (buf);
	}

	buf = copy_uint16(buf, utf8_str->len);

	MQTT_memcpy(buf, utf8_str->ptr, utf8_str->len);
	buf += utf8_str->len;

	return (buf);
}

//=============================================================================
static uint8_t *copy_data(uint8_t *buf, const MQTT_Data_t *data)
{
	if (data_is_null(data)) {
		*buf = 0;
		buf ++;
		*buf = 0;
		buf ++;
		return (buf);
	}

	buf = copy_uint16(buf, data->len);

	MQTT_memcpy(buf, data->ptr, data->len);
	buf += data->len;

	return (buf);
}

//=============================================================================
uint32_t MQTT_RemainLen(const uint8_t *buf)
{
	uint32_t val;
	uint32_t mul;
	uint8_t data;
	bool has_next;

	if (buf == NULL) return (0);

	buf ++;

	mul = 1;
	val = 0;
	has_next = FALSE;
	do {
		data = *buf;
		buf ++;
		val += (data & 127) * mul;
		mul *= 128;
		if (mul > (128 * 128 * 128)) {
			/*** ERROR ***/
			return (0);
		}
		has_next = ((data & 0x80) != 0) ? TRUE : FALSE;
	} while (has_next);

	return (val);
}

//=============================================================================
uint32_t MQTT_PacketLen(const uint8_t *buf)
{
	uint32_t remain_len;

	remain_len = MQTT_RemainLen(buf);

	return (fixed_header_size(remain_len) + remain_len);
}

//=============================================================================
const uint8_t *MQTT_PacketPayload(const uint8_t *buf)
{
	uint32_t remain_len;

	remain_len = MQTT_RemainLen(buf);

	if (remain_len == 0) {
		return (NULL);
	}

	buf += fixed_header_size(remain_len);

	return (buf);
}

//=============================================================================
// PUBLISH's Packet ID (QoS=2 or QoS=3)
//=============================================================================
uint16_t MQTT_PubPacketId(const uint8_t *payload)
{
	uint16_t topic_len;

	if (payload == NULL) return (0);

	topic_len = MQTT_Bytes2UInt16(payload);
	payload += (topic_len + 2);

	return (MQTT_Bytes2UInt16(payload));
}

//=============================================================================
// CONNECT Packet
//=============================================================================
uint8_t *MQTT_MakeConnectPacket(MQTT_UTF8_t *client_id,
								MQTT_UTF8_t *username,
								MQTT_Data_t *password,
								MQTT_UTF8_t *will_topic,
								MQTT_Data_t *will_msg,
								MQTT_QOS_t will_qos,
								MQTT_Retain_t will_retain,
								bool clean_session,
								uint16_t keep_alive)
{
	uint8_t flag = 0;
	uint32_t remain_len, total;
	uint8_t *buf, *work;

	const MQTT_UTF8_t protocol_name = { 4, "MQTT" };
	const uint8_t protocol_level = 4;

	// ---------------------------------------------------- Parameter Check ---
	if (username == NULL && password != NULL) {
		return (NULL);
	}
	if (will_topic != NULL && will_msg != NULL) {
		if (will_qos < MQTT_QOS_0 || will_qos > MQTT_QOS_2) {
			return (NULL);
		}
	} else if (will_topic != NULL && will_msg != NULL) {
		return (NULL);
	}

	// --------------------------------------------------- Calc Packet Size ---
	remain_len = 10; // Variable Header(10)
	if (client_id != NULL) {
		remain_len += (2 + client_id->len);
	} else {
		flag |= 0x02; // if client id doesnt exist, need to be clean session
	}
	if (username != NULL) {
		flag |= 0x80;
		remain_len += (2 + username->len);
	}
	if (password != NULL) {
		flag |= 0x40;
		remain_len += (2 + password->len);
	}
	if (will_topic != NULL && will_msg != NULL) {
		if (will_retain == MQTT_RETAIN) {
			flag |= 0x20;
		}
		flag |= (will_qos & 3) << 3;
		flag |= 0x04;
		remain_len += (2 + will_topic->len) + (2 + will_msg->len);
	}
	if (clean_session) {
		flag |= 0x02;
	}

	// --------------------------------------------- Allocate Packet Buffer ---
	total = remain_len + fixed_header_size(remain_len);
	buf = MQTT_malloc(total);
	if (buf == NULL) {
		return (NULL);
	}

    // --------------------------------------------------- Set Fixed Header ---
	work = set_fixed_header(buf, 0x10, remain_len);

	// ------------------------------------------------ Set Variable Header ---
	work = copy_utf8(work, &protocol_name);
	*work = protocol_level;
	work ++;
	*work = flag;
	work ++;
	work = copy_uint16(work, keep_alive);

	// -------------------------------------------------------- Set Payload ---
	work = copy_utf8(work, client_id);
	if (will_topic != NULL && will_msg != NULL) {
		work = copy_utf8(work, will_topic);
		work = copy_data(work, will_msg);
	}
	if (username != NULL) {
		work = copy_utf8(work, username);
	}
	if (password != NULL) {
		work = copy_data(work, password);
	}

	return (buf);
}

//=============================================================================
// PUBLISH Packet
//=============================================================================
uint8_t *MQTT_MakePublishPacket(MQTT_UTF8_t *topic,
								MQTT_Data_t *message,
								MQTT_QOS_t qos,
								MQTT_Retain_t retain,
								bool dup)
{
	uint32_t remain_len, total;
	uint8_t cmd;
	uint8_t *buf, *work;

	// ---------------------------------------------------- Parameter Check ---
	if (utf8_is_null(topic)) return (NULL);

	// --------------------------------------------------- Calc Packet Size ---
	remain_len = (2 + topic->len);
	if (qos != MQTT_QOS_0) {
		remain_len += 2;
	}
	if (!data_is_null(message)) {
		remain_len += message->len;
	}

	// --------------------------------------------- Allocate Packet Buffer ---
	total = remain_len + fixed_header_size(remain_len);
	buf = MQTT_malloc(total);
	if (buf == NULL) {
		return (NULL);
	}

    // --------------------------------------------------- Set Fixed Header ---
	cmd = 0x30 | ((qos & 3) << 1) | (retain & 1);
	if (dup) {
		cmd |= 0x08;
	}
	work = set_fixed_header(buf, cmd, remain_len);

	// ------------------------------------------------ Set Variable Header ---
	work = copy_utf8(work, topic);
	if (qos != MQTT_QOS_0) {
		work = copy_uint16(work, gMQTT_PacketID);
		gMQTT_PacketID ++;
		if (gMQTT_PacketID == 0) gMQTT_PacketID = 1;
	}

	// -------------------------------------------------------- Set Payload ---
	/*** ***/
	if (!data_is_null(message)) {
		MQTT_memcpy(work, message->ptr, message->len);
	}

	return (buf);
}

//=============================================================================
// PUBACK Packet
//=============================================================================
uint8_t *MQTT_MakePubAckPacket(uint16_t packet_id)
{
	uint8_t *buf;

	buf = MQTT_malloc(4);
	if (buf == NULL) return (NULL);

	buf[0] = 0x40;
	buf[1] = 0x02;
	copy_uint16(&buf[2], packet_id);

	return (buf);
}

//=============================================================================
// PUBREC Packet
//=============================================================================
uint8_t *MQTT_MakePubRecPacket(uint16_t packet_id)
{
	uint8_t *buf;

	buf = MQTT_malloc(4);
	if (buf == NULL) return (NULL);

	buf[0] = 0x50;
	buf[1] = 0x02;
	copy_uint16(&buf[2], packet_id);

	return (buf);
}

//=============================================================================
// PUBREL Packet
//=============================================================================
uint8_t *MQTT_MakePubRelPacket(uint16_t packet_id)
{
	uint8_t *buf;

	buf = MQTT_malloc(4);
	if (buf == NULL) return (NULL);

	buf[0] = 0x62;
	buf[1] = 0x02;
	copy_uint16(&buf[2], packet_id);

	return (buf);
}

//=============================================================================
// PUBCOMP Packet
//=============================================================================
uint8_t *MQTT_MakePubCompPacket(uint16_t packet_id)
{
	uint8_t *buf;

	buf = MQTT_malloc(4);
	if (buf == NULL) return (NULL);

	buf[0] = 0x70;
	buf[1] = 0x02;
	copy_uint16(&buf[2], packet_id);

	return (buf);
}

//=============================================================================
// SUBSCRIBE Packet
//=============================================================================
uint8_t *MQTT_MakeSubscribePacket(MQTT_UTF8_t *topic, MQTT_QOS_t qos)
{
	uint32_t remain_len, total;
	uint8_t *buf, *work;

	// ---------------------------------------------------- Parameter Check ---
	if (utf8_is_null(topic)) return (NULL);

	// --------------------------------------------------- Calc Packet Size ---
	remain_len = 2; // Variable Header(2)
	remain_len += (2 + topic->len) + 1;

	// --------------------------------------------- Allocate Packet Buffer ---
	total = remain_len + fixed_header_size(remain_len);
	buf = MQTT_malloc(total);
	if (buf == NULL) {
		return (NULL);
	}

    // --------------------------------------------------- Set Fixed Header ---
	work = set_fixed_header(buf, 0x82, remain_len);

	// ------------------------------------------------ Set Variable Header ---
	work = copy_uint16(work, gMQTT_PacketID);
	gMQTT_PacketID ++;
	if (gMQTT_PacketID == 0) gMQTT_PacketID = 1;

	// -------------------------------------------------------- Set Payload ---
	work = copy_utf8(work, topic);
	*work = qos;
	work ++;

	return (buf);
}

//=============================================================================
// UNSUBSCRIBE Packet
//=============================================================================
uint8_t *MQTT_MakeUnsubscribePacket(MQTT_UTF8_t *topic)
{
	uint32_t remain_len, total;
	uint8_t *buf, *work;

	// ---------------------------------------------------- Parameter Check ---
	if (utf8_is_null(topic)) return (NULL);

	// --------------------------------------------------- Calc Packet Size ---
	remain_len = 2; // Variable Header(2)
	remain_len += (2 + topic->len);

	// --------------------------------------------- Allocate Packet Buffer ---
	total = remain_len + fixed_header_size(remain_len);
	buf = MQTT_malloc(total);
	if (buf == NULL) {
		return (NULL);
	}

    // --------------------------------------------------- Set Fixed Header ---
	work = set_fixed_header(buf, 0xa2, remain_len);

	// ------------------------------------------------ Set Variable Header ---
	work = copy_uint16(work, gMQTT_PacketID);
	gMQTT_PacketID ++;
	if (gMQTT_PacketID == 0) gMQTT_PacketID = 1;

	// -------------------------------------------------------- Set Payload ---
	work = copy_utf8(work, topic);

	return (buf);
}

//=============================================================================
// PINGREQ Packet
//=============================================================================
uint8_t *MQTT_MakePingReqPacket(void)
{
	uint8_t *buf;

	buf = MQTT_malloc(2);
	if (buf == NULL) return (NULL);

	buf[0] = 0xc0;
	buf[1] = 0x00;

	return (buf);
}

//=============================================================================
// DISCONNECT Packet
//=============================================================================
uint8_t *MQTT_MakeDisconnectPacket(void)
{
	uint8_t *buf;

	buf = MQTT_malloc(2);
	if (buf == NULL) return (NULL);

	buf[0] = 0xe0;
	buf[1] = 0x00;

	return (buf);
}

/******************************** END-OF-FILE ********************************/
