/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"
#include "websocket.h"
#include "websocket_prv.h"

#include "rand.h"
#include "base64.h"
#include "sha1.h"

#include <stdlib.h>
#include <assert.h>

//=============================================================================
static const char *gExKey = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#define callOnOpenCB(ws, st) do {							\
		if ((ws)->on_open != NULL) {						\
			(*(WS_OnOpenCB_t *)(ws)->on_open)((ws), (st));	\
		}													\
	} while (0)
#define callOnCloseCB(ws, st) do {								\
		if ((ws)->on_close != NULL) {							\
			(*(WS_OnCloseCB_t *)(ws)->on_close)((ws), (st));	\
		}														\
	} while (0)
#define callOnRecvCB(ws, op, buf, len) do {								\
		if ((ws)->on_recv != NULL) {									\
			(*(WS_OnReceiveCB_t *)(ws)->on_recv)((ws), (op), (buf), (len));	\
		}																\
	} while (0)
#define callOnErrorCB(ws, st) do {								\
		if ((ws)->on_error != NULL) {							\
			(*(WS_OnErrorCB_t *)(ws)->on_error)((ws), (st));	\
		}														\
	} while (0)

//=============================================================================
bool WS_Create(WebSocket_t *websock,
			   WebSocketSecurity_t security,
			   uint32_t ipaddr, uint32_t portno, char *domain, char *path,
			   char *protocols)
{
	if (websock == NULL) {
		return (FALSE);
	}

	WS_memset(websock, 0, sizeof(WebSocket_t));

	WS_create_sem(websock->send_sem);

	websock->security = security;
	websock->ipaddr = ipaddr;

	if (portno == 0) {
		if (security == WS_SECURITY_NONE) {
			websock->portno = WS_DEFAULT_PORTNO_NOSEC;
		} else {
			websock->portno = WS_DEFAULT_PORTNO_SEC;
		}
	} else {
		websock->portno = portno;
	}

	websock->domain = domain;
	if (path != NULL) {
		websock->path = path;
	} else {
		websock->path = "/";
	}
	websock->protocols = protocols;

	websock->query = NULL;
	websock->origin = NULL;

	websock->sock = NULL;

	if (!UUID_GetUUID4(&websock->key)) {
		err_printf("[WS] failed to create key\n");
		return (FALSE);
	}

	websock->state = WS_STATE_CONNECTING;

	if (websock->security == WS_SECURITY_NONE) {
		WS_SetNetIF_Sock(&websock->netif);
	} else {
		WS_SetNetIF_SSL(&websock->netif);
	}

	return (TRUE);
}

//=============================================================================
static char *make_header(WebSocket_t *websock)
{
	char *header;
	char *host;
	char ipaddr[RTCS_IP4_ADDR_STR_SIZE];

	assert(websock != NULL);

	header = WS_malloc(1024);
	if (header == NULL) {
		return (NULL);
	}

	if (websock->domain != NULL) {
		host = websock->domain;
	} else {
		host = inet_ntop(AF_INET, &websock->ipaddr, ipaddr, sizeof(ipaddr));
	}

	WS_sprintf(header, "GET %s HTTP/1.1\r\n", websock->path);
	if ((websock->security == WS_SECURITY_NONE
		 && websock->portno == WS_DEFAULT_PORTNO_NOSEC)
		|| (websock->security != WS_SECURITY_NONE
			&& websock->portno == WS_DEFAULT_PORTNO_SEC)) {
		WS_sprintf(header + WS_strlen(header), "Host: %s\r\n", host);
	} else {
		WS_sprintf(header + WS_strlen(header),
				   "Host: %s:%d\r\n", host, websock->portno);
	}
	WS_strcat(header,
			  "Upgrade: websocket\r\n"
			  "Connection: Upgrade\r\n"
			  "Sec-WebSocket-Key: ");
	Base64_Encode(websock->key.bytes, UUID_BYTES_SIZE,
				  header + WS_strlen(header), 1024 - WS_strlen(header));
	WS_strcat(header, "\r\n"
			  "Sec-WebSocket-Version: 13\r\n");

	if (websock->origin != NULL) {
		WS_sprintf(header + WS_strlen(header),
				   "Origin: %s\r\n", websock->origin);
	}
	if (websock->protocols != NULL) {
		WS_sprintf(header + WS_strlen(header),
				   "Sec-WebSocket-Protocol: %s\r\n", websock->protocols);
	}
	WS_strcat(header, "\r\n");

	/*** DEBUG ***/
	dbg_printf("*** SEND ***\n%s*** SEND ***\n", header);
	/*** DEBUG ***/

	return (header);
}

//=============================================================================
static char *recv_header(WebSocket_t *websock)
{
	char *header;
	uint32_t recv_len;

	assert(websock != NULL);

	header = WS_malloc(1024);
	if (header == NULL) {
		return (NULL);
	}

	recv_len = (*websock->netif.recv)(websock->sock, (uint8_t *)header, 1024);
	if (SOCK_RecvError(recv_len)) {
		recv_len = 0;
	}
	if (recv_len < 1024) {
		header[recv_len] = '\0';
	} else {
		header[1023] = '\0';
	}

	/*** DEBUG ***/
	dbg_printf("*** RECV ***\n%s*** RECV ***\n", header);
	/*** DEBUG ***/

	return (header);
}

//=============================================================================
static uint32_t get_status(char *header)
{
	char *ptr;
	char num_work[12];
	int cnt;

	assert(header != NULL);

	if (WS_strncmp("HTTP/1.1", header, 8) != 0) {
		return (0);
	}

	ptr = header;
	while (*ptr != ' ' && *ptr != '\0') {
		ptr ++;
	}
	if (*ptr == '\0') {
		return (0);
	}
	while (*ptr == ' ' && *ptr != '\0') {
		ptr ++;
	}

	for (cnt = 0; cnt < (12 - 1); cnt ++) {
		if (*ptr < '0' || *ptr > '9') break;
		num_work[cnt] = *ptr;
		ptr ++;
	}
	num_work[cnt] = '\0';

	return (atoi(num_work));
}

//=============================================================================
bool check_accept_key(WebSocket_t *websock, char *header)
{
	char *match;
	char *ptr;
	char accept_key[30]; // 160 bit SHA-1 to base64 => 28 byte chars

	char org_key[26 + 36];
	SHA1_t sha1;
	uint8_t sha1_digest[SHA1_BYTES_SIZE];
	char cmp_key[30];

	assert(websock != NULL);
	assert(header != NULL);

	match = WS_strstr(header, "Sec-WebSocket-Accept");
	if (match == NULL) {
		return (FALSE);
	}

	ptr = match;
	while (*ptr != ':' && *ptr != '\0') {
		ptr ++;
	}
	if (*ptr == '\0') {
		return (FALSE);
	}
	ptr ++;
	while (*ptr == ' ' && *ptr != '\0') {
		ptr ++;
	}

	WS_memcpy(accept_key, ptr, 28);
	accept_key[28] = '\0';

	if (!Base64_Encode(websock->key.bytes, UUID_BYTES_SIZE, org_key, 26)) {
		return (FALSE);
	}
	WS_strcat(org_key, gExKey);
	if (!SHA1_Hash(org_key, sha1)) {
		return (FALSE);
	}
	if (!SHA1_GetDigest(sha1, sha1_digest)) {
		return (FALSE);
	}
	if (!Base64_Encode(sha1_digest, SHA1_BYTES_SIZE, cmp_key, 30)) {
		return (FALSE);
	}

	if (WS_strcmp(accept_key, cmp_key) != 0) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool WS_Open(WebSocket_t *websock)
{
	char *header;
	uint32_t status;
	bool key_check;
	uint32_t open_status;
	uint32_t send_len;

    // -------------------------------------------------------- Open Socket ---
	websock->sock = (*websock->netif.open)();
    if (websock->sock == NULL) {
        err_printf("[WS] sock open error\n");
		callOnOpenCB(websock, WS_STATUS_ERR_SOCKET_OPEN);
        return (FALSE);
    }

#if 0
	uint32_t timeout;
	setsockopt(websock->sock, SOL_TCP, OPT_CONNECT_TIMEOUT,
			   &timeout, sizeof(timeout));
#endif

    // ----------------------------------------------------- Connect Server ---
	if (!(*websock->netif.connect)(websock->sock,
								   websock->ipaddr, websock->portno)) {
		open_status = WS_STATUS_ERR_SOCKET_CONNECT;
		goto WS_Open_err;
	}

	// -------------------------------------------------- Send HTTP Request ---
	header = make_header(websock);
	if (header == NULL) {
		open_status = WS_STATUS_ERR_MEMORY_EXHAUSTED;
		goto WS_Open_err;
	}
	send_len = (*websock->netif.send)(websock->sock,
									  (uint8_t *)header, WS_strlen(header));
	WS_free(header);
	if (SOCK_SendError(send_len)) {
		open_status = WS_STATUS_ERR_SOCKET_SEND;
	}

	// ------------------------------------------------- Wait HTTP Response ---
	header = recv_header(websock);
	if (header == NULL) {
		open_status = WS_STATUS_ERR_MEMORY_EXHAUSTED;
		goto WS_Open_err;
	}

	// -------------------------------------------------- Check HTTP Status ---
	status = get_status(header);
	if (status != 101) {
		dbg_printf("[WS] status=%d\n", status);
		WS_free(header);
		open_status = WS_STATUS_ERR_HTTP_STATUS;
		goto WS_Open_err;
	}

	// --------------------------------------- Compare WebSocket Accept Key ---
	key_check = check_accept_key(websock, header);
	WS_free(header);
	if (!key_check) {
		open_status = WS_STATUS_ERR_ACCEPT_KEY;
		goto WS_Open_err;
	}

	// --------------------------------------------- WebSocket Open Success ---
	websock->state = WS_STATE_OPEN;
	callOnOpenCB(websock, 0);

	return (TRUE);

WS_Open_err:
	// --------------------------------------------- WebSocket Open Failure ---
	(*websock->netif.close)(websock->sock);
	websock->sock = NULL;

	websock->state = WS_STATE_FAILED;
	callOnOpenCB(websock, open_status);

	return (FALSE);
}

//=============================================================================
static void dbg_print_frame(uint8_t *frame, uint32_t len)
{
	int cnt;

	for (cnt = 0; cnt < len; cnt ++) {
		dbg_printf("0x%02X ", frame[cnt]);
		if (((cnt + 1) & 0xf) == 0) dbg_printf("\n");
	}
	if ((cnt & 0xf) != 0) printf("\n");
}

//=============================================================================
static bool send_frame(WebSocket_t *websock, uint8_t opcode,
					   uint8_t *payload, uint32_t len, bool mask)
{
	uint32_t frame_len;
	uint8_t *frame;
	uint32_t idx;
	uint32_t send_len;

	// ----------------------------------------------------- Entering Check ---
	if (opcode >= 0x10 || websock->state != WS_STATE_OPEN) {
		return (FALSE);
	}

	// -------------------------------------------------- Calc Frame Length ---
	frame_len = 1;
	if (len < 126) {
		frame_len += 1;
	} else if (len < 65536) {
		frame_len += 3;
	} else {
		frame_len += 9;
	}
	if (mask) {
		frame_len += 4;
	}
	frame_len += len;

	// -------------------------------------------------------- Alloc Frame ---
	frame = WS_malloc(frame_len);
	if (frame == NULL) {
		return (FALSE);
	}

	// --------------------------------------------------------- Make Frame ---
	frame[0] = 0x80 | opcode;

	idx = 1;
	if (len < 126) {
		frame[idx ++] = len;
	} else if (len < 65536) {
		frame[idx ++] = 126;
		frame[idx ++] = (len >> 8) & 0xff;
		frame[idx ++] = len & 0xff;
	} else {
		frame[idx ++] = 127;
		frame[idx ++] = 0;
		frame[idx ++] = 0;
		frame[idx ++] = 0;
		frame[idx ++] = 0;
		frame[idx ++] = (len >> 24) & 0xff;
		frame[idx ++] = (len >> 16) & 0xff;
		frame[idx ++] = (len >> 8) & 0xff;
		frame[idx ++] = len & 0xff;
	}

	if (mask) {
		uint32_t mask_data;
		uint8_t mkey[4];
		uint32_t cnt;

		mask_data = WS_rand();
		mkey[0] = (mask_data >> 24) & 0xff;
		mkey[1] = (mask_data >> 16) & 0xff;
		mkey[2] = (mask_data >> 8) & 0xff;
		mkey[3] = mask_data & 0xff;

		for (cnt = 0; cnt < 4; cnt ++) {
			frame[idx ++] = mkey[cnt];
		}
		for (cnt = 0; cnt < len; cnt ++) {
			frame[idx ++] = payload[cnt] ^ mkey[cnt & 3];
		}
		frame[1] |= 0x80;
	} else {
		if (payload != NULL && len > 0) {
			WS_memcpy(&frame[idx], payload, len);
			idx += len;
		}
	}

	assert(idx == frame_len);

	dbg_printf("[WS] send frame:\n");
	dbg_print_frame(frame, frame_len);

	WS_take_sem(websock->send_sem);

	send_len = (*websock->netif.send)(websock->sock, frame, frame_len);
	if (SOCK_SendError(send_len)) {
		/*** Failed ***/
	}

	WS_give_sem(websock->send_sem);

	WS_free(frame);

	return (TRUE);
}

//=============================================================================
void WS_InitFrame(WebSocketFrame_t *frame)
{
	assert(frame != NULL);

	frame->opcode = WS_OPCODE_NONE;
	frame->payload = NULL;
	frame->len = 0;
}

//=============================================================================
void WS_FreeFrame(WebSocketFrame_t *frame)
{
	if (frame == NULL) {
		return;
	}

	if (frame->payload != NULL) {
		WS_free(frame->payload);
	}

	WS_InitFrame(frame);
}

//=============================================================================
bool WS_ReceiveFrame(WebSocket_t *websock, WebSocketFrame_t *frame,
					 uint8_t *recv_buf, uint32_t recv_size)
{
	uint32_t recv_len;

	bool fin;
	uint8_t opcode;
	bool mask;
	uint32_t cnt;

	uint32_t len;
	uint32_t idx;

	assert(websock != NULL);
	assert(frame != NULL);
	assert(recv_buf != NULL);
	assert(recv_size > 0);

	WS_InitFrame(frame);

	// ------------------------------------------------------- Receive Data ---
	recv_len = (*websock->netif.recv)(websock->sock, recv_buf, recv_size);
	if (SOCK_RecvError(recv_len)) {
		WS_Close(websock);

		return (FALSE);
	}

	dbg_printf("[WS] recv frame:\n");
	dbg_print_frame(recv_buf, recv_len);

	fin = ((recv_buf[0] & 0x80) != 0);
	opcode = recv_buf[0] & 0x0f;
	mask = ((recv_buf[1] & 0x80) != 0);

	len = recv_buf[1] & 0x7f;
	if (len == 126) {
		len = ((uint32_t)recv_buf[2] << 8) + (uint32_t)recv_buf[3];
		idx = 4;
	} else if (len == 127) {
		len = ((uint32_t)recv_buf[6] << 24) + ((uint32_t)recv_buf[7] << 16)
			+ ((uint32_t)recv_buf[8] << 8) + (uint32_t)recv_buf[9];
		if (recv_buf[2] != 0
			|| recv_buf[3] != 0 || recv_buf[4] != 0 || recv_buf[5] != 0) {
			/*** too large, discard this Frame ***/
			return (FALSE);
		}
		idx = 10;
	} else {
		idx = 2;
	}
#if 0
	if ((!mask && (idx + len) != recv_len)
		|| (mask && (idx + len + 4) != recv_len)) {
		/*** ERROR ***/
		continue;
	}
#endif
	frame->fin = fin;
	frame->opcode = opcode;
	frame->len = len;

	if (len > 0) {
		frame->payload = WS_malloc(len);
		if (frame->payload == NULL) {
			return (FALSE);
		}

		if (mask) {
			uint8_t mkey[4];
			mkey[0] = recv_buf[idx ++];
			mkey[1] = recv_buf[idx ++];
			mkey[2] = recv_buf[idx ++];
			mkey[3] = recv_buf[idx ++];

			for (cnt = 0; cnt < len; cnt ++) {
				frame->payload[cnt] = recv_buf[idx ++] ^ mkey[cnt & 3];
			}
		} else {
			WS_memcpy(frame->payload, &recv_buf[idx], len);
			idx += len;
		}
	} else {
		frame->payload = NULL;
	}

	return (TRUE);
}

//=============================================================================
void WS_HandleFrame(WebSocket_t *websock,
					WebSocketFrame_t *frame, WebSocketFrame_t *stored)
{
	if (frame->opcode != WS_OPCODE_NONE) {
		if (frame->fin) {
			// ----------------------------------------------- Single Frame ---
			switch (frame->opcode) {
			case WS_OPCODE_PING:
				// ------------------------------------- Receive PING Frame ---
				WS_SendPongFrame(websock, frame->payload, frame->len);
				callOnRecvCB(websock,
							 frame->opcode, frame->payload, frame->len);
				break;
			case WS_OPCODE_PONG:
				// ------------------------------------- Receive PONG Frame ---
				callOnRecvCB(websock,
							 frame->opcode, frame->payload, frame->len);
				break;
			case WS_OPCODE_CLOSE:
				// ------------------------------------ Receive CLOSE Frame ---
				if (websock->state == WS_STATE_OPEN) {
					WS_SendCloseFrame(websock, 1000, "");
					websock->state = WS_STATE_CLOSING;
				}
				WS_Close(websock);
				break;
			case WS_OPCODE_TEXT:
			case WS_OPCODE_BINARY:
				// --------------------------- Receive TEXT or BINARY Frame ---
				callOnRecvCB(websock,
							 frame->opcode, frame->payload, frame->len);
				break;
			default:
				/*** ERROR ***/
				break;
			}
		}
		else {
			// ------------------------------------- Multiple Frame (First) ---
			switch (frame->opcode) {
			case WS_OPCODE_TEXT:
			case WS_OPCODE_BINARY:
				WS_FreeFrame(stored);

				stored->opcode = frame->opcode;
				stored->payload = frame->payload;
				stored->len = frame->len;

				WS_InitFrame(frame);
				break;
			default:
				/*** ERROR ***/
				break;
			}
		}
	}
	else {
		// -------------------------------- Multiple Frame (Middle or Last) ---
		if (stored->opcode != WS_OPCODE_NONE && stored->payload != NULL) {
			uint8_t *tmp_payload;
			tmp_payload = realloc(stored->payload,
								  stored->len + frame->len);
			if (tmp_payload != NULL) {
				WS_memcpy(tmp_payload + stored->len,
						  frame->payload, frame->len);
				stored->len += frame->len;
				stored->payload = tmp_payload;

				WS_InitFrame(frame);

				if (frame->fin) {
					// ------------------------------ Multiple Frame (Last) ---
					callOnRecvCB(websock,
								 stored->opcode, stored->payload, stored->len);

					WS_FreeFrame(stored);
				}
			} else {
				/*** no memory, discard ***/
				WS_FreeFrame(stored);
			}
		}
	}

	// ----------------------------------------------------------- Clean up ---
	WS_FreeFrame(frame);
}

//=============================================================================
void WS_Close(WebSocket_t *websock)
{
	if (websock == NULL) {
		return;
	}

	if (websock->sock != NULL) {
		(*websock->netif.close)(websock->sock);
		websock->sock = NULL;
	}

	websock->state = WS_STATE_CLOSED;

	callOnCloseCB(websock, 0);
}

//=============================================================================
bool WS_SendTextFrame(WebSocket_t *websock, char *text)
{
	if (websock == NULL || text == NULL) {
		return (FALSE);
	}

	return (send_frame(websock, WS_OPCODE_TEXT,
					   (uint8_t *)text, WS_strlen(text), TRUE));
}

//=============================================================================
bool WS_SendBinaryFrame(WebSocket_t *websock, uint8_t *buf, uint32_t len)
{
	if (websock == NULL || buf == NULL || len == 0) {
		return (FALSE);
	}

	return (send_frame(websock, WS_OPCODE_BINARY, buf, len, TRUE));
}

//=============================================================================
bool WS_SendPingFrame(WebSocket_t *websock, uint8_t *app_data, uint32_t len)
{
	if (websock == NULL || len > 125) {
		return (FALSE);
	}

	return (send_frame(websock, WS_OPCODE_PING, app_data, len, TRUE));
}

//=============================================================================
bool WS_SendPongFrame(WebSocket_t *websock, uint8_t *app_data, uint32_t len)
{
	if (websock == NULL || len > 125) {
		return (FALSE);
	}

	return (send_frame(websock, WS_OPCODE_PONG, app_data, len, TRUE));
}

//=============================================================================
bool WS_SendCloseFrame(WebSocket_t *websock, uint32_t status, char *reason)
{
	uint32_t len;
	bool ret;
	uint8_t payload[125];

	if (websock == NULL) {
		return (FALSE);
	}

	if (websock->state != WS_STATE_OPEN) {
		return (FALSE);
	}

	if (reason == NULL) {
		len = 2;
	} else {
		len = WS_strlen(reason) + 2;
		if (len > 125) {
			return (FALSE);
		}
	}

	payload[0] = (status >> 8) & 0xff;
	payload[1] = status & 0xff;
	if (reason != NULL && len > 2) {
		WS_memcpy(payload + 2, reason, len - 2);
	}

	ret = send_frame(websock, WS_OPCODE_CLOSE, payload, len, TRUE);

	websock->state = WS_STATE_CLOSING;

	return (ret);
}

//=============================================================================
void WS_OnOpen(WebSocket_t *websock, WS_OnOpenCB_t *callback)
{
	if (websock == NULL) return;

	websock->on_open = (void *)callback;
}

//=============================================================================
void WS_OnClose(WebSocket_t *websock, WS_OnCloseCB_t *callback)
{
	if (websock == NULL) return;

	websock->on_close = (void *)callback;
}

//=============================================================================
void WS_OnReceive(WebSocket_t *websock, WS_OnReceiveCB_t *callback)
{
	if (websock == NULL) return;

	websock->on_recv = (void *)callback;
}

//=============================================================================
void WS_OnError(WebSocket_t *websock, WS_OnErrorCB_t *callback)
{
	if (websock == NULL) return;

	websock->on_error = (void *)callback;
}

/******************************** END-OF-FILE ********************************/
