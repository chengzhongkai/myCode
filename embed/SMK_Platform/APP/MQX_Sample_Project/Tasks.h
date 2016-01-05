/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _TASKS_H
#define _TASKS_H

#include <mqx.h>

#include "Config.h"

//=============================================================================
// Task Number
//=============================================================================
enum {
    TN_INIT_TASK = 1,

    TN_NET_CONNECT_TASK,
    TN_DHCP_CHECK_TASK,
    TN_FTP_CLIENT_TASK,

#if defined(APP_ENL_ADAPTER) || defined(APP_HAx2_AS_SWITCH_CLASS) || defined(APP_ENL_CONTROLLER)
	// ECHONET Lite
	TN_ELL_TASK,
	TN_ELL_PKT_TASK,
	TN_ELL_INF_TASK,
	TN_ELL_ADP_CTRL_TASK,
	TN_ELL_ADP_SYNC_TASK,
#endif

#if defined(APP_ENL_CONTROLLER)
	TN_ELL_GATEWAY_APP_TASK,
	TN_ELL_GATEWAY_SVR_TASK,
#endif

#if defined(APP_MID_ADAPTER)
	// ECHONET Lite Middleware Adapter
    MWA_ADP_TASK,
    MWA_UDP_TASK,
    MWA_UART_TASK,
    MWA_FRM_TASK,
    MWA_ENL_TASK,
    MWA_INF_TASK,
#endif
};

#endif /* !_TASKS_H */

/******************************** END-OF-FILE ********************************/
