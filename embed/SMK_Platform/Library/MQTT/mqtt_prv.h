/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MQTT_PRV_H
#define _MQTT_PRV_H

//=============================================================================
#define MQTT_CMD_ID_CONNECT		0x10
#define MQTT_CMD_ID_CONNACK		0x20
#define MQTT_CMD_ID_PUBLISH		0x30
#define MQTT_CMD_ID_PUBACK		0x40
#define MQTT_CMD_ID_PUBREC		0x50
#define MQTT_CMD_ID_PUBREL		0x60
#define MQTT_CMD_ID_PUBCOMP		0x70
#define MQTT_CMD_ID_SUBSCRIBE	0x80
#define MQTT_CMD_ID_SUBACK		0x90
#define MQTT_CMD_ID_UNSUBSCRIBE	0xa0
#define MQTT_CMD_ID_UNSUBACK	0xb0
#define MQTT_CMD_ID_PINGREQ		0xc0
#define MQTT_CMD_ID_PINGRESP	0xd0
#define MQTT_CMD_ID_DISCONNECT	0xe0

//=============================================================================
#define MQTT_CmdId(buf) ((*(buf)) & 0xf0)
#define MQTT_QoS(buf) (MQTT_QOS_t)(((*(buf)) >> 1) & 0x03)
#define MQTT_SetDUP(buf) do { (*(buf)) |= 0x08; } while (0)

//=============================================================================
// mqtt_utils.c
//=============================================================================
void MQTT_Str2UTF8(MQTT_UTF8_t *utf8, const char *str);
uint16_t MQTT_Bytes2UInt16(const uint8_t *bytes);
const uint8_t *MQTT_Bytes2UTF8(MQTT_UTF8_t *utf8, const uint8_t *bytes);

//=============================================================================
// mqtt_packet.c
//=============================================================================
uint32_t MQTT_RemainLen(const uint8_t *buf);
uint32_t MQTT_PacketLen(const uint8_t *buf);
const uint8_t *MQTT_PacketPayload(const uint8_t *buf);
uint16_t MQTT_PubPacketId(const uint8_t *payload);
uint8_t *MQTT_MakeConnectPacket(MQTT_UTF8_t *client_id,
								MQTT_UTF8_t *username,
								MQTT_Data_t *password,
								MQTT_UTF8_t *will_topic,
								MQTT_Data_t *will_msg,
								MQTT_QOS_t will_qos,
								MQTT_Retain_t will_retain,
								bool clean_session,
								uint16_t keep_alive);
uint8_t *MQTT_MakePublishPacket(MQTT_UTF8_t *topic,
								MQTT_Data_t *message,
								MQTT_QOS_t qos,
								MQTT_Retain_t retain,
								bool dup);
uint8_t *MQTT_MakePubAckPacket(uint16_t packet_id);
uint8_t *MQTT_MakePubRecPacket(uint16_t packet_id);
uint8_t *MQTT_MakePubRelPacket(uint16_t packet_id);
uint8_t *MQTT_MakePubCompPacket(uint16_t packet_id);
uint8_t *MQTT_MakeSubscribePacket(MQTT_UTF8_t *topic, MQTT_QOS_t qos);
uint8_t *MQTT_MakeUnsubscribePacket(MQTT_UTF8_t *topic);
uint8_t *MQTT_MakePingReqPacket(void);
uint8_t *MQTT_MakeDisconnectPacket(void);

#endif /* !_MQTT_PRV_H */

/******************************** END-OF-FILE ********************************/
