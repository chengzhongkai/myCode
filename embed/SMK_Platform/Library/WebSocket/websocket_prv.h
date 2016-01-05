/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _WEBSOCKET_PRV_H
#define _WEBSOCKET_PRV_H

#ifdef __FREESCALE_MQX__
#include <rtcs.h>
#else
#error for Freescale MQX Platform
#endif

//=============================================================================
#ifdef __FREESCALE_MQX__
#define isInvalidSocket(sock) (sock == RTCS_SOCKET_ERROR)
#define SOCK_PORTNO(port) (port)
#define SOCK_Close(sock) shutdown(sock, 0)
#define SOCK_IsError(sts) (sts != RTCS_OK)
#define SOCK_INVALID (RTCS_SOCKET_ERROR)
#define SOCK_SendError(len) (len == RTCS_ERROR || len == 0)
#define SOCK_RecvError(len) (len == RTCS_ERROR || len == 0)
#define WS_malloc _mem_alloc
#define WS_free _mem_free
#define WS_memset memset
#define WS_memcpy memcpy
#define WS_strlen strlen
#define WS_strcat strcat
#define WS_strcmp strcmp
#define WS_strncmp strncmp
#define WS_strstr strstr
#define WS_sprintf sprintf
#define WS_rand FSL_GetRandom
#define WS_create_sem(sem) _lwsem_create(&(sem), 1)
#define WS_take_sem(sem) _lwsem_wait(&(sem))
#define WS_give_sem(sem) _lwsem_post(&(sem))
#else
#define isInvalidSocket(sock) (sock < 0)
#define SOCK_PORTNO(port) htons(port)
#define SOCK_Close(sock) close(sock)
#define SOCK_IsError(sts) (sts < 0)
#define SOCK_INVALID (-1)
#define SOCK_SendError(len) (len == 0)
#define SOCK_RecvError(len) (len == 0)
#define WS_malloc malloc
#define WS_free free
#define WS_memset memset
#define WS_memcpy memcpy
#define WS_strlen strlen
#define WS_strcat strcat
#define WS_strcmp strcmp
#define WS_strncmp strncmp
#define WS_strstr strstr
#define WS_sprintf sprintf
#define WS_rand rand
#define WS_create_sem(sem)
#define WS_take_sem(sem)
#define WS_give_sem(sem)
#endif

//=============================================================================
bool WS_Create(WebSocket_t *websock,
			   WebSocketSecurity_t security,
			   uint32_t ipaddr, uint32_t portno, char *domain, char *path,
			   char *protocols);
bool WS_Open(WebSocket_t *websock);
void WS_InitFrame(WebSocketFrame_t *frame);
void WS_FreeFrame(WebSocketFrame_t *frame);
bool WS_ReceiveFrame(WebSocket_t *websock, WebSocketFrame_t *frame,
					 uint8_t *recv_buf, uint32_t recv_size);
void WS_HandleFrame(WebSocket_t *websock,
					WebSocketFrame_t *frame, WebSocketFrame_t *stored);
void WS_Close(WebSocket_t *websock);

//=============================================================================
void WS_SetNetIF_Sock(WS_NetIF_t *netif);
void WS_SetNetIF_SSL(WS_NetIF_t *netif);

#endif /* !_WEBSOCKET_PRV_H */

/******************************** END-OF-FILE ********************************/
