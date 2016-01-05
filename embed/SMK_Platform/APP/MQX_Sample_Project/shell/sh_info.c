/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "Config.h"

/*** DEBUG ***/
#include "rand.h"
#include "uuid.h"
#include "base64.h"
#include "sha1.h"

#include "websocket.h"
/*** DEBUG ***/

//=============================================================================
static void onOpen(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Open (0x%X)\n", status);
}

//=============================================================================
static void onClose(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Close (0x%X)\n", status);
}

//=============================================================================
static void onReceive(WebSocket_t *websock,
					  uint8_t opcode, const uint8_t *buf, uint32_t len)
{
	int cnt;
	char *str;

	printf("WebSocket Received (opcode=0x%X):\n", opcode);

	if (opcode == WS_OPCODE_TEXT) {
		str = _mem_alloc(len + 1);
		memcpy(str, buf, len);
		str[len] = '\0';
		printf("%s\n", str);
		_mem_free(str);
	} else {
		for (cnt = 0; cnt < len; cnt ++) {
			printf("0x%02X ", buf[cnt]);
			if (((cnt + 1) & 0xf) == 0) printf("\n");
		}
		if ((cnt & 0xf) != 0) printf("\n");
	}
}

//=============================================================================
static void onError(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Error (0x%X)\n", status);
}

//=============================================================================
static void usage(void)
{
	printf("Usage: info\n   Show firmware information\n");
}

//=============================================================================
int32_t Shell_info(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("info\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 1) {
		usage();
		return (0);
	}

	printf(APP_MAIN_NAME "\n");
	printf(APP_SUB_NAME "\n");
	printf(APP_FW_NAME "\n");

	uint32_t ipaddr = (192 << 24) | (168 << 16) | (11 << 8) | 3;
	WebSocket_t *websock;

#if 0
	websock = WS_CreateHandle(WS_SECURITY_NONE,
							  ipaddr, 3000, "sxa212.smk.co.jp", "/", "chat");
#else
	websock = WS_CreateHandle(WS_SECURITY_TLS1, // WS_SECURITY_NONE,
							  ipaddr, 3000, NULL, NULL, NULL);
#endif
	if (websock != NULL) {
		WS_OnOpen(websock, onOpen);
		WS_OnClose(websock, onClose);
		WS_OnReceive(websock, onReceive);
		WS_OnError(websock, onError);

		if (WS_Start(websock, 9)) {
			WS_SendTextFrame(websock, "hello");
			_time_delay(1000);

			WS_SendTextFrame(websock, "{\n\t\"domain\":\"smk.co.jp\"\n}\n");
			_time_delay(1000);

			uint8_t buf[] = { 0x01, 0x02, 0x03, 0x04 };
			WS_SendBinaryFrame(websock, buf, 4);
			_time_delay(1000);

			WS_SendCloseFrame(websock, 1000, NULL);
			_time_delay(3000);
		}
		WS_ReleaseHandle(websock);
	}


#if 0 // random and crypto function test
	// printf("rand() = %d\n", FSL_GetRandom());

	UUID_t uuid;
	char uuid_str[UUID_STRING_LEN];
	char base64_str[256];
	SHA1_t sha1;
	int cnt;

#if 0
	if (UUID_GetUUID4(&uuid)) {
		if (UUID_GetString(&uuid, uuid_str)) {
			printf("UUID4: %s\n", uuid_str);
		}
	}

	if (Base64_Encode(uuid.bytes, UUID_BYTES_SIZE, base64_str, 256)) {
		printf("base64: %s\n", base64_str);
	}

	strcat(base64_str, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	printf("base64 + key: %s\n", base64_str);

	SHA1_Hash(base64_str, sha1);

	for (cnt = 0; cnt < 5; cnt ++) {
		printf("%08X", sha1[cnt]);
	}
	printf("\n");
#endif

	printf("\nthe sample nonce\n");
	Base64_Encode("the sample nonce", 16, base64_str, 256);
	printf("base64: %s\n", base64_str);

	strcat(base64_str, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	printf("base64 + key: %s\n", base64_str);

	SHA1_Hash(base64_str, sha1);

	for (cnt = 0; cnt < 5; cnt ++) {
		printf("%08X", sha1[cnt]);
	}
	printf("\n");

	uint8_t digest[SHA1_BYTES_SIZE];

	SHA1_GetDigest(sha1, digest);
	if (Base64_Encode(digest, 20, base64_str, 256)) {
		printf("base64: %s\n", base64_str);
	}
#endif

	return (0);
}

/******************************** END-OF-FILE ********************************/
