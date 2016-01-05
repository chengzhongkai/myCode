/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include <mqx.h>
#include <bsp.h>
#include <message.h>
#include "platform.h"

#include "uuid.h"

//=============================================================================
#ifdef __FREESCALE_MQX__
typedef uint32_t ws_socket_t;
typedef LWSEM_STRUCT ws_semaphore_t;
#else
typedef int ws_socket_t;
typedef int ws_semaphore_t;
#endif

//=============================================================================
#define WS_DEFAULT_PORTNO_NOSEC 80
#define WS_DEFAULT_PORTNO_SEC 443

//=============================================================================
typedef enum {
	WS_SECURITY_NONE = 0,
	WS_SECURITY_SSL3,
	WS_SECURITY_TLS1,
	WS_SECURITY_TLS2
} WebSocketSecurity_t;

//=============================================================================
typedef enum {
	WS_STATUS_OK = 0,

	WS_STATUS_ERR_SOCKET_OPEN = 0xF000,
	WS_STATUS_ERR_SOCKET_CONNECT = 0xF001,
	WS_STATUS_ERR_SOCKET_SEND = 0xF002,

	WS_STATUS_ERR_HTTP_STATUS = 0xF010,
	WS_STATUS_ERR_ACCEPT_KEY = 0xF011,

	WS_STATUS_ERR_MEMORY_EXHAUSTED = 0xFFFE,
	WS_STATUS_ERR_GENERIC = 0xFFFF
} WebSocketStatus_t;

//=============================================================================
typedef enum {
	WS_STATE_NONE = 0,
	WS_STATE_CONNECTING,
	WS_STATE_OPEN,
	WS_STATE_CLOSING,
	WS_STATE_CLOSED,
	WS_STATE_FAILED
} WebSocketState_t;

//=============================================================================
// Net I/F Callback
//=============================================================================
typedef void *WS_OpenCB_t(void);
typedef bool WS_ConnectCB_t(void *handle, uint32_t ipaddr, uint32_t portno);
typedef void WS_CloseCB_t(void *handle);
typedef uint32_t WS_SendCB_t(void *handle, uint8_t *buf, uint32_t len);
typedef uint32_t WS_RecvCB_t(void *handle, uint8_t *buf, uint32_t len);

typedef struct {
	WS_OpenCB_t *open;
	WS_ConnectCB_t *connect;
	WS_CloseCB_t *close;
	WS_SendCB_t *send;
	WS_RecvCB_t *recv;
} WS_NetIF_t;

//=============================================================================
// WebSocket Handle
//=============================================================================
typedef struct _websocket_t {
	// Server Info
	WebSocketSecurity_t security;
	uint32_t ipaddr;
	uint32_t portno;
	char *domain;
	char *path;
	char *protocols;

	char *query;
	char *origin;

	UUID_t key;

	void *sock;

	WebSocketState_t state;

	ws_semaphore_t send_sem;
#ifdef __FREESCALE_MQX__
	_task_id task_id;
#endif

	void *on_open;
	void *on_close;
	void *on_recv;
	void *on_error;

	WS_NetIF_t netif;
} WebSocket_t;

//=============================================================================
// Client Callback
//=============================================================================
typedef void WS_OnOpenCB_t(WebSocket_t *websock, uint32_t status);
typedef void WS_OnCloseCB_t(WebSocket_t *websock, uint32_t status);
typedef void WS_OnReceiveCB_t(WebSocket_t *websock, uint8_t opcode,
							  const uint8_t *buf, uint32_t len);
typedef void WS_OnErrorCB_t(WebSocket_t *websock, uint32_t status);

//=============================================================================
#define WS_OPCODE_NONE   0x0
#define WS_OPCODE_TEXT   0x1
#define WS_OPCODE_BINARY 0x2
#define WS_OPCODE_CLOSE  0x8
#define WS_OPCODE_PING   0x9
#define WS_OPCODE_PONG   0xA

//=============================================================================
typedef struct {
	bool fin;
	uint8_t opcode;
	uint8_t *payload;
	uint32_t len;
} WebSocketFrame_t;

//=============================================================================
WebSocket_t *WS_CreateHandle(WebSocketSecurity_t security,
							 uint32_t ipaddr, uint32_t portno,
							 char *domain, char *path, char *protocols);
bool WS_Start(WebSocket_t *websock, uint32_t priority);
void WS_Stop(WebSocket_t *websock, uint32_t status);
void WS_ReleaseHandle(WebSocket_t *websock);

//=============================================================================
bool WS_SendTextFrame(WebSocket_t *websock, char *text);
bool WS_SendBinaryFrame(WebSocket_t *websock, uint8_t *buf, uint32_t len);
bool WS_SendPingFrame(WebSocket_t *websock, uint8_t *app_data, uint32_t len);
bool WS_SendPongFrame(WebSocket_t *websock, uint8_t *app_data, uint32_t len);
bool WS_SendCloseFrame(WebSocket_t *websock, uint32_t status, char *reason);

//=============================================================================
void WS_OnOpen(WebSocket_t *websock, WS_OnOpenCB_t *callback);
void WS_OnClose(WebSocket_t *websock, WS_OnCloseCB_t *callback);
void WS_OnReceive(WebSocket_t *websock, WS_OnReceiveCB_t *callback);
void WS_OnError(WebSocket_t *websock, WS_OnErrorCB_t *callback);

#endif /* !_WEBSOCKET_H */

/******************************** END-OF-FILE ********************************/
