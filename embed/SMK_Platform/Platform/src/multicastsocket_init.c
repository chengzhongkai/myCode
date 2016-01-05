#include <mqx.h>
#include <rtcs.h>
#include <ipcfg.h>
#include "multicastsocket_init.h"

static uint32_t	socketid;		/* socket ID */
static ip_mreq		group;			/* IP data for IGMP setting */

uint32_t MulticastSocket_init(uint32_t port, _ip_address ip, _ip_address multicastip)
{
	//uint32_t	socketid;		/* socket ID */
	sockaddr	sockaddrdata;	/* socket IP data */
	//ip_mreq		group;			/* IP data for IGMP setting */
	uint32_t	error;			/* function error output */

	/* Input parameter(port number) check */
	if (port > 65535)
	{
		fputs("MulticastSocket: Fatal Error: Port number too big.\n", stderr);
		return RTCS_SOCKET_ERROR;
	}

	/* open UDP socket */
	socketid = socket(PF_INET, SOCK_DGRAM, 0);
	if (socketid == RTCS_SOCKET_ERROR)
	{
		fputs("MulticastSocket: Fatal Error: Unable to create socket for IPv4 connections.\n", stderr);
		return RTCS_SOCKET_ERROR;
 	}
	else
	{
		((sockaddr_in*) &sockaddrdata)->sin_family = AF_INET;
		((sockaddr_in*) &sockaddrdata)->sin_port = (uint16_t)port;
		((sockaddr_in*) &sockaddrdata)->sin_addr.s_addr = INADDR_ANY;

		error = bind(socketid, &sockaddrdata, sizeof(sockaddrdata));
		if (error != RTCS_OK)
		{
			fprintf(stderr, "MulticastSocket: Fatal Error 0x%X: Unable to bind IPv4 socket.\n", error);
			shutdown(socketid, FLAG_ABORT_CONNECTION);
			return RTCS_SOCKET_ERROR;
		}
	}

	/* Multicast setting */
	group.imr_multiaddr.s_addr = multicastip;
	group.imr_interface.s_addr = ip;

	error = setsockopt(socketid, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP, &group, sizeof(group));
	if (error != RTCS_OK)
	{
		fprintf(stderr, "MulticastSocket: Fatal Error 0x%X: IGMP setting error.\n", error);
		shutdown(socketid, FLAG_ABORT_CONNECTION);
		socketid = RTCS_SOCKET_ERROR;
	}

	return socketid;
}

void MulticastSocket_Reset(void)
{

	setsockopt(socketid, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP, &group, sizeof(group));

}