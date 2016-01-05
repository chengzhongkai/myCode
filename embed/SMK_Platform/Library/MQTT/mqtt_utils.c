/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "mqtt.h"
#include "mqtt_prv.h"

//=============================================================================
void MQTT_Str2UTF8(MQTT_UTF8_t *utf8, const char *str)
{
	if (utf8 == NULL || str == NULL) return;

	utf8->len = MQTT_strlen(str);
	utf8->ptr = (const uint8_t *)str;
}

//=============================================================================
uint16_t MQTT_Bytes2UInt16(const uint8_t *bytes)
{
	uint16_t ret;

	if (bytes == NULL) return (0);

	ret = (((uint16_t)bytes[0]) << 8) | (uint16_t)bytes[1];

	return (ret);
}

//=============================================================================
const uint8_t *MQTT_Bytes2UTF8(MQTT_UTF8_t *utf8, const uint8_t *bytes)
{
	if (utf8 == NULL || bytes == NULL) return (0);

	utf8->len = MQTT_Bytes2UInt16(bytes);
	bytes += 2;
	utf8->ptr = bytes;
	bytes += utf8->len;

	return (bytes);
}

/******************************** END-OF-FILE ********************************/
