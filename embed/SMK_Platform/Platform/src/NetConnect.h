#ifndef NET_CONNECT_H_
#define NET_CONNECT_H_

#include <ipcfg.h>
#include "platform.h"

#define MULTICAST_ENABLE

#ifdef MULTICAST_ENABLE
#include "multicastsocket_init.h"
#endif

/* time setting */
#define NET_CONNECT_DHCP_PERIOD			10		/* (s) */
#define NET_CONNECT_DHCP_POLL_PERIOD	200		/* (ms) */
#define NET_CONNECT_WATCH_PERIOD		60		/* (s) */

/* message command */
//#define NET_CONNECT_MSG_STATUS_REQ	1
#define NET_CONNECT_MSG_STATUS		2
//#define NET_CONNECT_MSG_ERROR_RES	3
#define NET_CONNECT_MSG_FATAL_ERROR	4

typedef struct net_connect_info
{
	bool					isUseDhcp;			/* Is DHCP enabled? */
	IPCFG_IP_ADDRESS_DATA	stStaticIpData;		/* IP address, netmask, gateway address for Static mode. */
	_enet_address			stMacAddress;		/* MAC address */

    _queue_number taskQueueNumber;

	_queue_number			*pQueueNumber;		/* queue number vectors for IP infomation announce (0 terminated) */
} NET_CONNECT_INFO, * NET_CONNECT_INFO_PTR;

extern bool Net_Connect_status(void);
extern void Net_Connect_task(uint32_t param);

extern void DHCP_Check_Task(uint32_t);

typedef void DHCP_Check_Calllback(uint32_t ip,
								  uint32_t netmask, uint32_t gateway);
void DHCP_Check_SetCallback(DHCP_Check_Calllback *func);

#endif /* NET_CONNECT_H_ */
