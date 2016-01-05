/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _CONNECTION_H
#define _CONNECTION_H

typedef uint32_t IPv4Addr_t;

//=============================================================================
// Socket Abstraction
//=============================================================================
#ifdef __FREESCALE_MQX__
typedef uint32_t socket_t;
#define isInvalidSocket(sock) (sock == RTCS_SOCKET_ERROR)
#define SOCK_PORTNO(port) (port)
#define SOCK_Close(sock) shutdown(sock, 0)
#define SOCK_IsError(sts) (sts != RTCS_OK)
#else
typedef int socket_t;
#define isInvalidSocket(sock) (sock < 0)
#define SOCK_PORTNO(port) htons(port)
#define SOCK_Close(sock) close(sock)
#define SOCK_IsError(sts) (sts < 0)
#endif

//=============================================================================
// UDP Connection Handle
//=============================================================================
typedef struct {
  socket_t sock;
  int port;
  IPv4Addr_t multicast_addr;
  IPv4Addr_t if_addr;
} UDP_Handle_t;

//=============================================================================
// UDP Connection Functions
//=============================================================================
bool_t UDP_Open(const char *if_addr, const char *group_addr, int port,
                UDP_Handle_t *handle);
int UDP_Recv(UDP_Handle_t *handle,
             uint8_t *buf, int max, int timeout, IPv4Addr_t *recv_addr);
bool_t UDP_Send(UDP_Handle_t *handle,
                IPv4Addr_t send_addr, const uint8_t *buf, int len);
bool_t UDP_Multicast(UDP_Handle_t *handle, const uint8_t *buf, int len);
void UDP_Close(UDP_Handle_t *handle);
bool_t UDP_Reset(UDP_Handle_t *handle);

#endif // !_CONNECTION_H

/******************************** END-OF-FILE ********************************/
