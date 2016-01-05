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

#include "rand.h"
#include "bignum.h"
#include "hmac.h"
#include "aes.h"
#include "des.h"

#include <assert.h>

//=============================================================================
#define TLS_setUint16(buf, num) do { \
	(buf)[0] = (((num) >> 8) & 0xff);  \
	(buf)[1] = ((num) & 0xff);   \
} while (0)
#define TLS_getUint16(buf) (((uint16_t)((buf)[0]) << 8) | (uint16_t)((buf)[1]))

#define TLS_setUint24(buf, num) do { \
	(buf)[0] = (((num) >> 16) & 0xff);  \
	(buf)[1] = (((num) >> 8) & 0xff);  \
	(buf)[2] = ((num) & 0xff);   \
} while (0)
#define TLS_getUint24(buf) (((uint32_t)((buf)[0]) << 16) | ((uint32_t)((buf)[1]) << 8) | (uint16_t)((buf)[2]))

//=============================================================================
#ifdef __FREESCALE_MQX__
#define isInvalidSocket(sock) (sock == RTCS_SOCKET_ERROR)
#define SOCK_PORTNO(port) (port)
#define SOCK_Close(sock) shutdown(sock, 0)
#define SOCK_IsError(sts) (sts != RTCS_OK)
#define SOCK_INVALID (RTCS_SOCKET_ERROR)
#define SOCK_SendError(len) (len == RTCS_ERROR || len == 0)
#define SOCK_RecvError(len) (len == RTCS_ERROR || len == 0)
#define TLS_malloc _mem_alloc
#define TLS_free _mem_free
#define TLS_memset memset
#define TLS_memcpy memcpy
#define TLS_memcmp memcmp
#define TLS_strlen strlen
#define TLS_strcpy strcpy
#define TLS_strcat strcat
#define TLS_strcmp strcmp
#define TLS_strncmp strncmp
#define TLS_strstr strstr
#define TLS_sprintf sprintf
#define TLS_rand FSL_GetRandom
#define TLS_create_sem(sem) _lwsem_create(&(sem), 1)
#define TLS_take_sem(sem) _lwsem_wait(&(sem))
#define TLS_give_sem(sem) _lwsem_post(&(sem))
#define TLS_assert assert
#else
#define isInvalidSocket(sock) (sock < 0)
#define SOCK_PORTNO(port) htons(port)
#define SOCK_Close(sock) close(sock)
#define SOCK_IsError(sts) (sts < 0)
#define SOCK_INVALID (-1)
#define SOCK_SendError(len) (len == 0)
#define SOCK_RecvError(len) (len == 0)
#define TLS_malloc malloc
#define TLS_free free
#define TLS_memset memset
#define TLS_memcpy memcpy
#define TLS_memcmp memcmp
#define TLS_strlen strlen
#define TLS_strcpy strcpy
#define TLS_strcat strcat
#define TLS_strcmp strcmp
#define TLS_strncmp strncmp
#define TLS_strstr strstr
#define TLS_sprintf sprintf
#define TLS_rand rand
#define TLS_create_sem(sem)
#define TLS_take_sem(sem)
#define TLS_give_sem(sem)
#define TLS_assert assert
#endif

#endif /* !_WEBSOCKET_PRV_H */

/******************************** END-OF-FILE ********************************/
