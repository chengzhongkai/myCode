/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <ipcfg.h>
#include <rtc.h>
#include <rtcs.h>
#include <message.h>

#include "Tasks.h"
#include "Config.h"

#include "rand.h"

#include "flash_utils.h"

#include "NetConnect.h"

#include "Telnet.h"
#include "version.h"
#include "http_server/web.h"

#include "HaControl.h"

#include "ell_task.h"
#include "led_task.h"

#include "mdns.h"

#include "console_task.h"

//=============================================================================
static void network_changed_notification(uint32_t ip, uint32_t netmask,
										 uint32_t gateway)
{
	if (ip != 0 && netmask != 0 && gateway != 0) {
		// ------------------------------------------- IP Address is binded ---
		msg_printf("\n[Net] IpAddress : %d.%d.%d.%d\n",
				   (ip & 0xFF000000) >> 24, (ip & 0x00FF0000) >> 16,
				   (ip & 0x0000FF00) >> 8, (ip & 0x000000FF));

		msg_printf("[Net] SubNetMask: %d.%d.%d.%d\n",
				   (netmask & 0xFF000000) >> 24, (netmask & 0x00FF0000) >> 16,
				   (netmask & 0x0000FF00) >> 8, (netmask & 0x000000FF));

		msg_printf("[Net] GateWay   : %d.%d.%d.%d\n\n",
				   (gateway & 0xFF000000) >> 24, (gateway & 0x00FF0000) >> 16,
				   (gateway & 0x0000FF00) >> 8, (gateway & 0x000000FF));

		ELL_NotifyReset();

#if APP_USE_LED_TASK
		LED_SetMode(LED_ORANGE, LED_MODE_ON);
#endif
	} else {
		// --------------------------------------- IP Address is not binded ---
		msg_printf("\n[Net] IpAddress: Error\n\n");

#if APP_USE_LED_TASK
		LED_SetMode(LED_ORANGE, LED_MODE_BLINK_FAST);
#endif
	}
}

//=============================================================================
// Initialize Network
//=============================================================================
void initialize_networking(IPCFG_IP_ADDRESS_DATA IP_data,
                           uint8_t *MAC_addr, uint8_t DHCP_mode)
{
	static _queue_number pQueueNumber[] = {DHCP_CHECK_MESSAGE_QUEUE, 0};
	static NET_CONNECT_INFO param;

    // -------------------------------------------- Create Net Connect Task ---
	param.isUseDhcp			= DHCP_mode;
	param.stStaticIpData	= IP_data;
	param.stMacAddress[0]	= MAC_addr[0];
	param.stMacAddress[1]	= MAC_addr[1];
	param.stMacAddress[2]	= MAC_addr[2];
	param.stMacAddress[3]	= MAC_addr[3];
	param.stMacAddress[4]	= MAC_addr[4];
	param.stMacAddress[5]	= MAC_addr[5];
    param.taskQueueNumber   = NET_CONNECT_TASK_MESSAGE_QUEUE;
	param.pQueueNumber		= pQueueNumber;

	_task_create(0, TN_NET_CONNECT_TASK, (uint32_t)&param);

    // --------------------------------------------- Create DHCP Check Task ---
	DHCP_Check_SetCallback(network_changed_notification);
	_task_create(0, TN_DHCP_CHECK_TASK, (uint32_t)DHCP_CHECK_MESSAGE_QUEUE);
}

//=============================================================================
// Start DNS Resolver Task
//=============================================================================
uint32_t initialize_DomainNameServer(void)
{
	uint32_t error;

	error = DNS_init();

	return error;
}

//=============================================================================
// Init Task
//=============================================================================
void Init_Task(uint32_t data)
{
	uint32_t error;
	uint8_t MAC_addr[6];
	uint8_t DHCP_mode;
	static IPCFG_IP_ADDRESS_DATA IP_addr;
	VERSION_DATA *ver = (VERSION_DATA *)&stSoftwareVersion;

	FSL_InitRandom();

	// ------------------------------------------------------ Start Message ---
	fflush(stdout);
	msg_printf("\n\n" APP_MAIN_NAME "\n" APP_SUB_NAME "\n");
	msg_printf("Version: %x.%02x\n", ver->bMajorVersion, ver->bMinorVersion);
	msg_printf("Build Date: %x-%02x-%02x\n",
			   ver->wYear, ver->bMonth, ver->bDate);
	msg_printf("\n");

    // -------------------------------------------------------- Init EEPROM ---
	Flash_Init();

	// ------------------------------------------------- Start Console Task ---
	Console_StartTask(11);

#if APP_DEBUG
	msg_printf("Wait 5 seconds, press ENTER then stop initializing...\n");

	int loop;
	for (loop = 0; loop < 10; loop ++) {
		_time_delay(500);
		if (Console_InShell()) break;
	}
	if (Console_InShell()) {
		msg_printf("\n[Init] *** Stop Initializing ***\n\n");
		_task_block();
	}
#endif

	// ------------------------------------------------ Init Message System ---
	_msgpool_create_system(sizeof(GROUPWARE_MESSAGE_STRUCT), 30, 2, 0);
	_msgq_open_system(INIT_TASK_MESSAGE_QUEUE, 10, 0, 0);

	// ------------------------------------------------- Init HA Controller ---
	error = HaControl_Init();
	if (error != MQX_OK) {
		err_printf("[HA] failed to init [err=0x%08X]\n", error);
		_task_block();
	}
	msg_printf("[Init] Start HA Controller\n");

	// ------------------------------------------------ Get Network Setting ---
	Flash_Get_Ethernet(&IP_addr, MAC_addr, &DHCP_mode);

	msg_printf("[Init] Load Network Setting\n");
	msg_printf("  MAC Address: %02X-%02X-%02X-%02X-%02X-%02X\n",
		   MAC_addr[0], MAC_addr[1], MAC_addr[2], 
		   MAC_addr[3], MAC_addr[4], MAC_addr[5]);
	msg_printf("  DHCP: %s\n", (DHCP_mode) ? "ON" : "OFF");

#if APP_USE_LED_TASK
	// ----------------------------------------------------- Start LED Task ---
	error = LED_StartTask(NULL, 11);

	LED_SetMode(LED_ORANGE, LED_MODE_BLINK_FAST);
#endif

	// ------------------------------------------------------ Start Network ---
	initialize_networking(IP_addr, MAC_addr, DHCP_mode);

	// ------------------------------------------------ Wait for Connecting ---
	while (!Net_Connect_status()) {
		_time_delay(1);
	}

	// ------------------------------ Wait Network Stable, if not DHCP Mode ---
	if (!DHCP_mode) {
		err_printf("[Init] Wait Network Stable... (about 10 sec)\n");
		_time_delay(10000);
	}

#if APP_USE_LED_TASK
	// ------------------------------------------------------- Set LED Mode ---
	LED_SetMode(LED_ORANGE, LED_MODE_ON);
#endif

#if APP_USE_HTTP_SERVER
	// -------------------------------------------------- Start HTTP Server ---
	error = initialize_HTTPServer();
	if (error != MQX_OK) {
		err_printf("[HTTP] failed to start [err=0x%08X]\n", error);
		_task_block();
	}
	msg_printf("[Init] Start HTTP Server\n");
#endif

	// ---------------------------------------- Start Telnet(Testmode) Task ---
	error = initialize_Telnet();
	if (error != MQX_OK) {
		err_printf("[Telnet] failed to start [err=0x%08X]\n", error);
		_task_block();
	}
	msg_printf("[Init] Start Telnet Server\n");

	// -------------------------------------------- Start DNS Resolver Task ---
	error = initialize_DomainNameServer();
	if (error != MQX_OK) {
		err_printf("[DNS] failed to start [err=0x%08X]\n", error);
		_task_block();
	}
	msg_printf("[Init] Start DNS Resolver\n");

	// ------------------------------------------ Start mDNS Responder Task ---
	char hostname[32];
	sprintf(hostname, "smk_%02x%02x%02x",
			MAC_addr[3], MAC_addr[4], MAC_addr[5]);
	error = MDNS_StartTask(hostname, 11);
	if (error != MQX_OK) {
		err_printf("[mDNS] failed to start [err=0x%08X]\n", error);
		_task_block();
	}
	msg_printf("[Init] Start mDNS Responder\n");
	msg_printf("[mDNS] Hostname is \"%s.local\"\n", hostname);

	// -------------------------------------------------- Start Application ---
#if defined(APP_ENL_ADAPTER) || defined(APP_HAx2_AS_SWITCH_CLASS) || defined(APP_ENL_CONTROLLER)
	_task_create(0, TN_ELL_TASK, 0);
#elif defined(APP_MID_ADAPTER)
    _task_create(0, MWA_ADP_TASK, 0);
#endif

	msg_printf("\n");

	/*** Block forever since init done ***/
	_task_block();
}

/******************************** END-OF-FILE ********************************/
