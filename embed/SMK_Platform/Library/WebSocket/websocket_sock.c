/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"
#include "websocket.h"
#include "websocket_prv.h"

//=============================================================================
static void *WS_OpenSock(void)
{
	ws_socket_t sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (isInvalidSocket(sock)) {
        return (NULL);
    }

	return ((void *)sock);
}

//=============================================================================
static bool WS_ConnectSock(void *handle, uint32_t ipaddr, uint32_t portno)
{
	ws_socket_t sock = (ws_socket_t)handle;

	struct sockaddr_in addr;
	bool err;

    WS_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ipaddr;
    addr.sin_port = SOCK_PORTNO(portno);

	err = SOCK_IsError(connect(sock, (struct sockaddr *)&addr, sizeof(addr)));

	return (!err);
}

//=============================================================================
static void WS_CloseSock(void *handle)
{
	ws_socket_t sock = (ws_socket_t)handle;

	SOCK_Close(sock);
}

//=============================================================================
static uint32_t WS_SendSock(void *handle, uint8_t *buf, uint32_t len)
{
	ws_socket_t sock = (ws_socket_t)handle;

	return (send(sock, buf, len, 0));
}

//=============================================================================
static uint32_t WS_RecvSock(void *handle, uint8_t *buf, uint32_t len)
{
	ws_socket_t sock = (ws_socket_t)handle;

	return (recv(sock, buf, len, 0));
}

//=============================================================================
void WS_SetNetIF_Sock(WS_NetIF_t *netif)
{
	netif->open = WS_OpenSock;
	netif->connect = WS_ConnectSock;
	netif->close = WS_CloseSock;
	netif->send = WS_SendSock;
	netif->recv = WS_RecvSock;
}

/******************************** END-OF-FILE ********************************/
