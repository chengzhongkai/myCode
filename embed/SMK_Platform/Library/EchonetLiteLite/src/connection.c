/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "echonetlitelite.h"
#include "connection.h"

#ifndef __FREESCALE_MQX__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define USE_SELECT
#endif

#if 0
#define err_printf(format, ...) fprintf(stderr, format, ...)
#else
#define err_printf(format, ...) ((void)0)
#endif

//=============================================================================
bool_t UDP_Open(const char *if_addr, const char *group_addr, int port,
                UDP_Handle_t *handle)
{
    socket_t sock;
    int sts;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
#ifndef __FREESCALE_MQX__
    int flag;
    char loop;
    in_addr_t ipaddr;
#endif

    if (handle == NULL) return (FALSE);

    MEM_Set(handle, 0, sizeof(UDP_Handle_t));

    if (if_addr == NULL) return (FALSE);

    // -------------------------------------------------------- Open Socket ---
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (isInvalidSocket(sock)) {
        err_printf("sock open error\n");
        return (FALSE);
    }

#ifndef __FREESCALE_MQX__
    flag = 1;
    sts = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (SOCK_IsError(sts)) {
        SOCK_Close(sock);
        err_printf("sockopt SO_REUSEADDR error[%d]\n", sts);
        return (FALSE);
    }
#endif

    // -------------------------------------------------------- Bind Socket ---
    MEM_Set(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = SOCK_PORTNO(port);

    sts = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (SOCK_IsError(sts)) {
        SOCK_Close(sock);
        err_printf("sock bind error[%d]\n", sts);
        return (FALSE);
    }

    // --------------------------------------------- Join Multicast Address ---
    if (group_addr != NULL) {
        MEM_Set(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(group_addr);
        mreq.imr_interface.s_addr = inet_addr(if_addr);
#ifdef __FREESCALE_MQX__
        sts = setsockopt(sock, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP,
                         &mreq, sizeof(mreq));
#else
        sts = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                         &mreq, sizeof(mreq));
#endif
        if (SOCK_IsError(sts)) {
            SOCK_Close(sock);
            err_printf("setsockopt error\n");
            return (FALSE);
        }

#ifndef __FREESCALE_MQX__
        // --------------------------------------------------------------------
        ipaddr = inet_addr(if_addr);
        sts = setsockopt(sock,
                         IPPROTO_IP, IP_MULTICAST_IF, &ipaddr, sizeof(ipaddr));
        if (SOCK_IsError(sts)) {
            SOCK_Close(sock);
            err_printf("setsockopt error");
            return (FALSE);
        }
        loop = 0;
        sts = setsockopt(sock,
                         IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (SOCK_IsError(sts)) {
            SOCK_Close(sock);
            err_printf("sockopt IP_MULTICAST_LOOP error[%d]\n", sts);
            return (FALSE);
        }
#endif
    }

    // ------------------------------------------------------------------------
    handle->sock = sock;
    handle->port = port;
    if (group_addr != NULL) {
        handle->multicast_addr = inet_addr(group_addr);
    } else {
        handle->multicast_addr = INADDR_ANY;
    }
    handle->if_addr = inet_addr(if_addr);

    return (TRUE);
}

//=============================================================================
int UDP_Recv(UDP_Handle_t *handle,
             uint8_t *buf, int max, int timeout, IPv4Addr_t *recv_addr)
{
    struct sockaddr_in addr;
#ifdef __FREESCALE_MQX__
    uint16_t addrlen;
#else
    socklen_t addrlen;
#endif

#ifdef USE_SELECT
    fd_set rfd;
#endif

    if (handle == NULL || buf == NULL) return (0);

    MEM_Set(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = SOCK_PORTNO(handle->port);
    addrlen = sizeof(addr);

    int recv_size;

#ifndef __FREESCALE_MQX__
    struct timeval tv;

    if (timeout == 0) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    } else {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }
#ifdef USE_SELECT
    FD_ZERO(&rfd);
    FD_SET(handle->sock, &rfd);
    if (select(handle->sock + 1, &rfd, NULL, NULL, &tv) <= 0) {
        return (0);
    }
#else
    if (setsockopt(handle->sock,
                   SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        return (0);
    }
#endif
#endif

    recv_size = recvfrom(handle->sock, buf, max, 0,
                         (struct sockaddr *)&addr, &addrlen);

    if (recv_size > 0) {
        // printf("from %s\n", inet_ntoa(addr.sin_addr));
        if (handle->if_addr == addr.sin_addr.s_addr) {
            recv_size = 0;
        }

        if (recv_addr != NULL) {
            *recv_addr = addr.sin_addr.s_addr;
        }
    }

    return (recv_size);
}

//=============================================================================
bool_t UDP_Send(UDP_Handle_t *handle,
                IPv4Addr_t send_addr, const uint8_t *buf, int len)
{
    struct sockaddr_in addr;
    int send_size;

    if (handle == NULL || buf == NULL || len <= 0) return (FALSE);

    MEM_Set(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = send_addr;
    addr.sin_port = SOCK_PORTNO(handle->port);

#ifdef __FREESCALE_MQX__
    send_size = sendto(handle->sock, (char *)buf, len, RTCS_MSG_NOLOOP,
                       (struct sockaddr *)&addr, sizeof(addr));
#else
    send_size = sendto(handle->sock, buf, len, 0,
                       (struct sockaddr *)&addr, sizeof(addr));
#endif

    if (send_size < 0) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}

//=============================================================================
bool_t UDP_Multicast(UDP_Handle_t *handle, const uint8_t *buf, int len)
{
    if (handle == NULL || buf == NULL || len <= 0) return (FALSE);

    return (UDP_Send(handle, handle->multicast_addr, buf, len));
}

//=============================================================================
void UDP_Close(UDP_Handle_t *handle)
{
    if (handle == NULL) return;

    SOCK_Close(handle->sock);

    handle->sock = -1;
}

//=============================================================================
bool_t UDP_Reset(UDP_Handle_t *handle)
{
#ifdef __FREESCALE_MQX__
    int sts;
    struct ip_mreq mreq;

	if (handle == NULL) return (FALSE);

	if (handle->multicast_addr != INADDR_ANY) {
		MEM_Set(&mreq, 0, sizeof(mreq));
		mreq.imr_multiaddr.s_addr = handle->multicast_addr;
		mreq.imr_interface.s_addr = handle->if_addr;
		sts = setsockopt(handle->sock, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP,
						 &mreq, sizeof(mreq));
		if (SOCK_IsError(sts)) {
			err_printf("setsockopt error\n");
			return (FALSE);
		}
	}
#endif

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
