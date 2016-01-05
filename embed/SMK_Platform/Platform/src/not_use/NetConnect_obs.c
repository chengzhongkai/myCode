#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <enet.h>
#include <ipcfg.h>
#include <message.h>
#include <assert.h>

//#include "LedOrange.h"
#include "NetConnect.h"

/* DHCP Connection Try  */
//#define DHCP_TRY_TIMES	3

/* net connection state */
enum
{
	NetConnect_Init,			/* initialize */
	NetConnect_DHCP1,			/* trying DHCP connection */
	NetConnect_DHCP2,			/* trying DHCP connection */
	NetConnect_DHCP3,			/* wait for next DHCP connection */
	NetConnect_Connect_DHCP,	/* Connected by DHCP data */
	NetConnect_Static,			/* trying Static IP connection */
	NetConnect_Connect_Static,	/* Connected by Static data */
	NetConnect_Disconnect,		/* Disconnected */
	NetConnect_Error,			/* Fatal error */
};

static void Net_Connect_send_status(_queue_id queueid, _queue_number *targets, IPCFG_IP_ADDRESS_DATA_PTR ipdata);
static void Net_Connect_send_fatal_error(_queue_id queueid, _queue_number *targets);
#if _DEBUG
static void Net_Connect_Led_Control(_queue_id queueid, uint32_t ledcommand);
#endif

static uint32_t dwConnectStatus;		/* state of connection state machine */

void Net_Connect_task(uint32_t param)
{
	NET_CONNECT_INFO_PTR			info = (NET_CONNECT_INFO_PTR)param;	/* connection information */
	uint32_t						error;								/* result of rtcs functions */
	_queue_id						queueid;							/* message queue id */
//	uint32_t						dhcptrycount = 0;					/* count DHCP trying times */
	MQX_TICK_STRUCT					currenttime;						/* current tick time */
	MQX_TICK_STRUCT					starttime;							/* start tick time for periodic announce */
	MQX_TICK_STRUCT					dhcptime;							/* start tick time for DHCP connection */
//	MQX_TICK_STRUCT					staticconnectchecktime;				/* start tick time for static connection */
	bool							overflow;							/* dummy variable for _time_diff function */
	IPCFG_IP_ADDRESS_DATA			currentip = {	.ip      = 0,
													.mask    = 0,
													.gateway = 0};		/* current IP data */
	bool							forcesend = false;					/* flag of announcement for IP status change */
	GROUPWARE_MESSAGE_STRUCT_PTR	receivemsg;							/* receive message */
	IPCFG_IP_ADDRESS_DATA			rtcsip;								/* IP data of current RTCS setting */

	if (info == NULL)
	{
#if _DEBUG
		puts("Net_Connect_task: Fatal Error: Illigal input parameter.\n");
#endif
		_task_destroy(MQX_NULL_TASK_ID);
	}

	dwConnectStatus = NetConnect_Init;

	/* Initialize message queue */
	queueid = _msgq_open_system(info->taskQueueNumber, 0, NULL, 0);
	if (queueid == 0)
	{
#if _DEBUG
		fprintf(stderr, "Net_Connect_task: Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
#endif
		_task_destroy(MQX_NULL_TASK_ID);
	}
#if _DEBUG
	else
	{
	 	puts("Net_Connect_task: Message queue initialized.\n");
	}
#endif

	/* Initialize RTCS */
	if (RTCS_create() != RTCS_OK)
	{
#if _DEBUG
 		fputs("Net_Connect_task: Fatal Error: RTCS initialization failed.", stderr);
#endif
		Net_Connect_send_fatal_error(queueid, info->pQueueNumber);
	}
#if _DEBUG
	else
	{
	 	puts("Net_Connect_task: RTCS Driver initialized.\n");
	}
#endif

	/* Initialize ethernet */
	error = ipcfg_init_device(BSP_DEFAULT_ENET_DEVICE, info->stMacAddress);
	if (error != IPCFG_OK)
	{
#if _DEBUG
		fprintf(stderr, "Net_Connect_task: Fatal Error 0x%X: Network device initialization failed.\n", error);
#endif
		Net_Connect_send_fatal_error(queueid, info->pQueueNumber);
	}
#if _DEBUG
	else
	{
	 	puts("Net_Connect_task: Ethernet Driver initialized.\n");
	}
#endif

	_time_get_ticks(&starttime);

	while (1)
	{
		/* connection */
		switch (dwConnectStatus)
		{
			case NetConnect_Init:

				if (ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE) == false)
				{
#if _DEBUG
					Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_OFF);
#endif
					dwConnectStatus = NetConnect_Disconnect;
					break;
				}
#if _DEBUG
				Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_BLINK);
#endif
				if (info->isUseDhcp == true)
				{
					_time_get_ticks(&dhcptime);
//					dhcptrycount = 0;
					if (ipcfg_bind_dhcp(BSP_DEFAULT_ENET_DEVICE, FALSE) == RTCSERR_IPCFG_BUSY)
					{
						dwConnectStatus = NetConnect_DHCP1;
					}
					else
					{
						dwConnectStatus = NetConnect_DHCP2;
					}
				}
				else
				{
					dwConnectStatus = NetConnect_Static;
				}
				break;

			case NetConnect_DHCP1:

				_time_get_ticks(&currenttime);
				if (_time_diff_milliseconds(&currenttime, &dhcptime, &overflow) >= NET_CONNECT_DHCP_POLL_PERIOD)
				{
					if(ipcfg_bind_dhcp(BSP_DEFAULT_ENET_DEVICE, FALSE) != RTCSERR_IPCFG_BUSY)
					{
						dwConnectStatus = NetConnect_DHCP2;
					}

					dhcptime = currenttime;
				}
				break;

			case  NetConnect_DHCP2:

				_time_get_ticks(&currenttime);
				if (_time_diff_milliseconds(&currenttime, &dhcptime, &overflow) >= NET_CONNECT_DHCP_POLL_PERIOD)
				{
					if((error = ipcfg_poll_dhcp(BSP_DEFAULT_ENET_DEVICE, FALSE, NULL)) != RTCSERR_IPCFG_BUSY)
					{
						if ((error == IPCFG_OK) && (ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &currentip) == true))
						{
							forcesend = true;
							dwConnectStatus = NetConnect_Connect_DHCP;

#ifdef MULTICAST_ENABLE
							//multicastソケットのリセット //追加:紺屋
							MulticastSocket_Reset();
#endif

#if _DEBUG
							Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_ON);
							puts("Net_Connect_task: connected\n");
#endif
						}
						else
						{
//							if (dhcptrycount++ < DHCP_TRY_TIMES)
//							{
								dwConnectStatus = NetConnect_DHCP3;
//							}
//							else
//							{
//								dwConnectStatus = NetConnect_Static;
//#if _DEBUG
//					 			fputs("DHCP error. Trying static IP connection.\n", stderr);
//#endif
//							}
						}
					}

					dhcptime = currenttime;
				}
				break;

			case NetConnect_DHCP3:

			  	if (ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE) == false)
				{
#if _DEBUG
					Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_OFF);
#endif
					dwConnectStatus = NetConnect_Disconnect;
				}
				else
				{
					_time_get_ticks(&currenttime);
					if (_time_diff_seconds(&currenttime, &dhcptime, &overflow) >= NET_CONNECT_DHCP_PERIOD)
					{
						dhcptime = currenttime;
						dwConnectStatus = NetConnect_DHCP1;
					}
				}
				break;

			case NetConnect_Connect_DHCP:

				ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &rtcsip);
				if( (ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE) == false) ||
					(rtcsip.mask != currentip.mask) )
				{
#if _DEBUG
					Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_OFF);
					puts("Net_Connect_task: disconnected\n");
#endif
					forcesend = true;
					currentip.ip      = 0;
					currentip.mask    = 0;
					currentip.gateway = 0;
					(void)ipcfg_unbind(BSP_DEFAULT_ENET_DEVICE);

					dwConnectStatus = NetConnect_Disconnect;
				}
				break;

			case NetConnect_Static:

				if (ipcfg_bind_staticip(BSP_DEFAULT_ENET_DEVICE, &(info->stStaticIpData)) == IPCFG_OK)
				{
					currentip = info->stStaticIpData;
					forcesend = true;
//					_time_get_ticks(&staticconnectchecktime);
					dwConnectStatus = NetConnect_Connect_Static;
#if _DEBUG
					puts("Net_Connect_task: connected\n");
					Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_ON);
#endif
				}
				else
				{
					dwConnectStatus = NetConnect_Error;
#if _DEBUG
					fprintf(stderr, "Fatal Error 0x%X: IP address binding failed.", error);
#endif
				}
				break;

			case NetConnect_Connect_Static:

//				_time_get_ticks(&currenttime);
				if (ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE) == false) //||
//				    ((info->isUseDhcp == true) && (_time_diff_seconds(&currenttime, &staticconnectchecktime, &overflow) >= NET_CONNECT_WATCH_PERIOD)) )
				{
#if _DEBUG
					puts("Net_Connect_task: disconnected\n");
					Net_Connect_Led_Control(queueid, LED_ORANGE_CMD_OFF);
#endif
					forcesend = true;
					currentip.ip      = 0;
					currentip.mask    = 0;
					currentip.gateway = 0;
					(void)ipcfg_unbind(BSP_DEFAULT_ENET_DEVICE);	/* ソケットを閉じた後でないと固まる可能性? */
					dwConnectStatus = NetConnect_Disconnect;
				}
				break;

			case NetConnect_Disconnect:

				if (ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE) == true)
				{
					dwConnectStatus = NetConnect_Init;
				}
				break;

//			case NetConnect_Error:
			default:

				Net_Connect_send_fatal_error(queueid, info->pQueueNumber);
		}

		/* periodic announce */
		_time_get_ticks(&currenttime);
		if ((forcesend == true) || (_time_diff_seconds(&currenttime, &starttime, &overflow) >= NET_CONNECT_WATCH_PERIOD))
		{
			forcesend = false;
			starttime = currenttime;
			Net_Connect_send_status(queueid, info->pQueueNumber, &currentip);
		}

		/* delete receive message */
		if ((receivemsg = _msgq_poll(queueid)) != NULL)
		{
			_msg_free(receivemsg);
		}

		_time_delay(1);
	}
}

static void Net_Connect_send_status(_queue_id queueid, _queue_number *targets, IPCFG_IP_ADDRESS_DATA_PTR ipdata)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	message;	/* message data */
	_queue_id						targetqid;	/* target message queue id */
	uint32_t                        i = 0;		/* roop count */

	assert(queueid);
	assert(targets);
	assert(ipdata);

	while (*(targets+i) != 0)
	{
		targetqid = _msgq_get_id(0, *(targets+i));
		if ((targetqid != MSGQ_NULL_QUEUE_ID) &&
			((message = (GROUPWARE_MESSAGE_STRUCT_PTR)_msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT))) != NULL)	)
		{
			message->header.SIZE       = sizeof(GROUPWARE_MESSAGE_STRUCT);
			message->header.TARGET_QID = targetqid;
			message->header.SOURCE_QID = queueid;
			message->data[0] = NET_CONNECT_MSG_STATUS;
			message->data[1] = (uint32_t)(ipdata->ip);
			message->data[2] = (uint32_t)(ipdata->mask);
			message->data[3] = (uint32_t)(ipdata->gateway);

			if (_msgq_send(message) == false)
			{
				_msg_free(message);
#if _DEBUG
				fputs("Error : message send error.\n", stderr);
#endif
			}
		}
#if _DEBUG
		else
		{
			fputs("Error : message target error.\n", stderr);
		}
#endif
		i++;
	}

	return;
}

static void Net_Connect_send_fatal_error(_queue_id queueid, _queue_number *targets)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	message;	/* message data */
	_queue_id						targetqid;	/* target message queue id */
	uint32_t                        i = 0;		/* roop count */

	assert(queueid);
	assert(targets);

	while (*(targets+i) != 0)
	{
		targetqid = _msgq_get_id(0, *(targets+i));
		if ((targetqid != MSGQ_NULL_QUEUE_ID) &&
			((message = (GROUPWARE_MESSAGE_STRUCT_PTR)_msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT))) != NULL) )
		{
			message->header.SIZE       = sizeof(GROUPWARE_MESSAGE_STRUCT);
			message->header.TARGET_QID = targetqid;
			message->header.SOURCE_QID = queueid;
			message->data[0] = NET_CONNECT_MSG_FATAL_ERROR;

			if (_msgq_send(message) == false)
			{
				_msg_free(message);
#if _DEBUG
				fputs("Error : message send error.\n", stderr);
#endif
			}
		}
#if _DEBUG
		else
		{
			fputs("Error : message target error.\n", stderr);
		}
#endif
		i++;
	}

	_task_block();
}

bool Net_Connect_status(void)
{
	bool result = false;

 	if ((dwConnectStatus == NetConnect_Connect_DHCP) || (dwConnectStatus == NetConnect_Connect_Static))
	{
		result = true;
	}

	return result;
}

#if _DEBUG
static void Net_Connect_Led_Control(_queue_id queueid, uint32_t ledcommand)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	message;	/* message data */

	assert(queueid);
	assert((ledcommand == LED_ORANGE_CMD_ON) || (ledcommand == LED_ORANGE_CMD_OFF) || (ledcommand == LED_ORANGE_CMD_BLINK));

	message = (GROUPWARE_MESSAGE_STRUCT_PTR)_msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT));

	if (message != NULL)
	{
		message->header.SIZE       = sizeof(GROUPWARE_MESSAGE_STRUCT);
		message->header.TARGET_QID = _msgq_get_id(0, LED_ORANGE_MESSAGE_QUEUE);
		message->header.SOURCE_QID = queueid;
		message->data[0] = ledcommand;

		if ( (message->header.TARGET_QID == MSGQ_NULL_QUEUE_ID) ||
			 (_msgq_send(message) == false)                       )
		{
			_msg_free(message);
#if _DEBUG
			fputs("Error : message send error.\n", stderr);
#endif
		}
	}
#if _DEBUG
	else
	{
			fputs("Error : message allocate error.\n", stderr);
	}
#endif
	return;
}
#endif
