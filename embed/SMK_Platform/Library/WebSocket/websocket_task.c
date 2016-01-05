/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"
#include "websocket.h"
#include "websocket_prv.h"

#include "mqx_utils.h"

//=============================================================================
#define WS_TASK_STACK_SIZE 2048
#define WS_TASK_NAME "WebSocket"

#define WS_RECV_BUFFER_SIZE 1024

//=============================================================================
static void WS_ReceiveTask(uint32_t param);

//=============================================================================
// Create WebSocket Handle
//=============================================================================
WebSocket_t *WS_CreateHandle(WebSocketSecurity_t security,
							 uint32_t ipaddr, uint32_t portno,
							 char *domain, char *path, char *protocols)
{
	WebSocket_t *websock;

	// ---------------------------------------------------- Alloc WebSocket ---
	websock = WS_malloc(sizeof(WebSocket_t));
	if (websock == NULL) {
		return (NULL);
	}

	// -------------------------------------------- Create WebSocket Handle ---
	if (!WS_Create(websock,
				   security, ipaddr, portno, domain, path, protocols)) {
		WS_free(websock);
		return (NULL);
	}

	return (websock);
}

//=============================================================================
// Start WebSocket
//
//   Open WebSocket and Start Receiving-Frame Task
//=============================================================================
bool WS_Start(WebSocket_t *websock, uint32_t priority)
{
	uint32_t err;

	// ----------------------------------------------------- Open WebSocket ---
	if (!WS_Open(websock)) {
		return (FALSE);
	}

	// ------------------------------ Create and Start Receiving-Frame Task ---
	err = MQX_CreateTask(&websock->task_id, WS_ReceiveTask, WS_TASK_STACK_SIZE,
						 priority, WS_TASK_NAME, (uint32_t)websock);
	if (err != MQX_OK) {
		if (websock->sock != NULL) {
			(*websock->netif.close)(websock->sock);
			websock->sock = NULL;
		}
		websock->state = WS_STATE_FAILED;
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
// Stop WebSocket
//=============================================================================
void WS_Stop(WebSocket_t *websock, uint32_t status)
{
	if (websock->state == WS_STATE_OPEN) {
		if (status == 0) {
			status = 1000;
		}
		WS_SendCloseFrame(websock, status, NULL);
		websock->state = WS_STATE_CLOSING;
	}
}

//=============================================================================
// Release WebSocket Handle
//=============================================================================
void WS_ReleaseHandle(WebSocket_t *websock)
{
	if (websock == NULL) {
		return;
	}

	if (websock->sock != NULL) {
		WS_Close(websock);
		_time_delay(50);
	}

	if (_task_get_index_from_id(websock->task_id) != 0) {
		_task_destroy(websock->task_id);
	}

	WS_free(websock);
}

//=============================================================================
// Receive and Handle Frame Task
//=============================================================================
static void WS_ReceiveTask(uint32_t param)
{
	WebSocket_t *websock = (WebSocket_t *)param;

	uint8_t *recv_buf;

	WebSocketFrame_t frame;
	WebSocketFrame_t stored_frame;

	// ----------------------------------------------- Alloc Receive Buffer ---
	recv_buf = WS_malloc(WS_RECV_BUFFER_SIZE);
	if (recv_buf == NULL) {
		return;
	}

	WS_InitFrame(&frame);
	WS_InitFrame(&stored_frame);

	// ---------------------------------------------------------- Main Loop ---
	while (websock->state == WS_STATE_OPEN
		   || websock->state == WS_STATE_CLOSING) {
		if (WS_ReceiveFrame(websock, &frame, recv_buf, WS_RECV_BUFFER_SIZE)) {
			WS_HandleFrame(websock, &frame, &stored_frame);
		}
	}

	// ------------------------------------------------ Free Receive Buffer ---
	if (recv_buf != NULL) {
		WS_free(recv_buf);
	}
}

/******************************** END-OF-FILE ********************************/
