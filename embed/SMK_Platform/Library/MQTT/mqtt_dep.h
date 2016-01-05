/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _MQTT_DEP_H
#define _MQTT_DEP_H

#include <assert.h>
#define MQTT_assert assert

#ifdef __FREESCALE_MQX__
#include <rtcs.h>
#else
#error for Freescale MQX Platform
#endif

//=============================================================================
#ifdef __FREESCALE_MQX__
typedef LWSEM_STRUCT mqtt_semaphore_t;
#define MQTT_create_sem(sem) _lwsem_create(&(sem), 1)
#define MQTT_take_sem(sem) _lwsem_wait(&(sem))
#define MQTT_give_sem(sem) _lwsem_post(&(sem))
#else
typedef int mqtt_semaphore_t;
#define MQTT_create_sem(sem)
#define MQTT_take_sem(sem)
#define MQTT_give_sem(sem)
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
#define MQTT_RECV_TIMEOUT (0L)
#define MQTT_RECV_ERROR (0xffffffffL)
#else
#define isInvalidSocket(sock) (sock < 0)
#define SOCK_PORTNO(port) htons(port)
#define SOCK_Close(sock) close(sock)
#define SOCK_IsError(sts) (sts < 0)
#define SOCK_INVALID (-1)
#define SOCK_SendError(len) (len == 0)
#define SOCK_RecvError(len) (len == 0)
#define MQTT_RECV_TIMEOUT (0)
#define MQTT_RECV_ERROR (-1)
#endif

//=============================================================================
#ifdef __FREESCALE_MQX__
#define MQTT_malloc _mem_alloc
#define MQTT_free _mem_free
#define MQTT_memset memset
#define MQTT_memcpy memcpy
#define MQTT_strlen strlen
#define MQTT_strcat strcat
#define MQTT_strcmp strcmp
#define MQTT_strncmp strncmp
#define MQTT_strstr strstr
#define MQTT_sprintf sprintf
#define MQTT_rand FSL_GetRandom
#else
#define MQTT_malloc malloc
#define MQTT_free free
#define MQTT_memset memset
#define MQTT_memcpy memcpy
#define MQTT_strlen strlen
#define MQTT_strcat strcat
#define MQTT_strcmp strcmp
#define MQTT_strncmp strncmp
#define MQTT_strstr strstr
#define MQTT_sprintf sprintf
#define MQTT_rand rand
#endif

#endif /* !_MQTT_DEP_H */

/******************************** END-OF-FILE ********************************/
