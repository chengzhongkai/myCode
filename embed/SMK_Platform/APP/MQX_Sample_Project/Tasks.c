/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>

#include "NetConnect.h"
#include "FtpClient.h"

#include "Tasks.h"

//=============================================================================
void Init_Task(uint32_t);

void ELL_Task(uint32_t param);
void ELL_Handler_Task(uint32_t param);
void ELL_INF_Task(uint32_t param);
void ELL_AdpCtrl_Task(uint32_t param);
void ELL_AdpSync_Task(uint32_t param);

void ELL_GatewayApp_Task(uint32_t param);
void ELL_GatewaySvr_Task(uint32_t param);

void MWA_ADP_Task(uint32_t param);
void MWA_UDP_Task(uint32_t param);
void MWA_UART_Task(uint32_t param);
void MWA_Frame_Task(uint32_t param);
void MWA_ENL_Task(uint32_t param);
void MWA_INF_Task(uint32_t param);

//=============================================================================
const TASK_TEMPLATE_STRUCT MQX_template_list[] =
{
	// Task Index,         Function,         Stack, Pri, Name,
	// Attributes, Param, Time Slice
	{ TN_INIT_TASK,         Init_Task,         2000,  13, "Init",
	  MQX_AUTO_START_TASK, 0, 0 },

	{ TN_NET_CONNECT_TASK,  Net_Connect_task,  2000,  15, "NetConnect",
	  0, 0, 0 },

	{ TN_DHCP_CHECK_TASK,   DHCP_Check_Task,   1000,  15, "DHCP Check",
	  0, 0, 0 },
	{ TN_FTP_CLIENT_TASK,   FTPClient_task,    8000,  12, "FW Downloader",
	  0, 0, 0 },

#if defined(APP_ENL_ADAPTER) || defined(APP_HAx2_AS_SWITCH_CLASS) || defined(APP_ENL_CONTROLLER)
	{ TN_ELL_TASK,          ELL_Task,          2000,   8, "Echonet",
	  0, 0, 0 },
	{ TN_ELL_PKT_TASK,      ELL_Handler_Task,  2000,  10, "Echonet Handler",
	  0, 0, 0 },
	{ TN_ELL_INF_TASK,      ELL_INF_Task,      2000,  11, "Echonet INF",
	  0, 0, 0 },
	{ TN_ELL_ADP_CTRL_TASK, ELL_AdpCtrl_Task,  1000,  10, "Echonet ADP Ctrl",
	  0, 0, 0 },
	{ TN_ELL_ADP_SYNC_TASK, ELL_AdpSync_Task,  1000,  10, "Echonet ADP Sync",
	  0, 0, 0 },
#endif

#if defined(APP_ENL_CONTROLLER)
	{ TN_ELL_GATEWAY_APP_TASK,  ELL_GatewayApp_Task,  2000,  10, "Echonet GW App",
      0, 0, 0 },
	{ TN_ELL_GATEWAY_SVR_TASK,  ELL_GatewaySvr_Task,  2000,  10, "Echonet GW Server",
      0, 0, 0 },
#endif

#if defined(APP_MID_ADAPTER)
	{ MWA_ADP_TASK,         MWA_ADP_Task,      2000,  10, "MWA State Machine",
	  0, 0, 0 },
	{ MWA_UDP_TASK,         MWA_UDP_Task,      2000,   8, "MWA UDP Recv",
	  0, 0, 0 },
	{ MWA_UART_TASK,        MWA_UART_Task,     2000,   7, "MWA UART Recv",
	  0, 0, 0 },
	{ MWA_FRM_TASK,         MWA_Frame_Task,    2000,   9, "MWA Frame Comm",
      0, 0, 0 },
	{ MWA_ENL_TASK,         MWA_ENL_Task,      2000,   9, "MWA Echonet",
	  0, 0, 0 },
	{ MWA_INF_TASK,         MWA_INF_Task,      2000,  11, "MWA INF Check",
	  0, 0, 0 },
#endif

	/*** Terminator ***/
	{0}
};

/******************************** END-OF-FILE ********************************/
