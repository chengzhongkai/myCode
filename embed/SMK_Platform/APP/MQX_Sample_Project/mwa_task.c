/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "Config.h"

#include "echonetlitelite.h"
#include "connection.h"
#include "ell_adapter.h"

#include <mqx.h>
#include <rtcs.h>
#include <ipcfg.h>
#include <timer.h>

#include "Tasks.h"

//=============================================================================
#define RECV_UDP_BUFFER_SIZE (1024)
#define RECV_UART_BUFFER_SIZE (256)

#define UART_DEVIVCE_NAME "ittyb:"

//=============================================================================
extern MWA_TimerHandle_t gMWA_SMTimer;
extern MWA_TimerHandle_t gMWA_ENLTimer;

//=============================================================================
static uint8_t gRecvUDPBuffer[RECV_UDP_BUFFER_SIZE];
static uint8_t gRecvUARTBuffer[RECV_UART_BUFFER_SIZE];

//=============================================================================
UDP_Handle_t gUDP;
MWA_UARTHandle_t gUART;

//=============================================================================
// Middleware Adapter Task
//=============================================================================
void MWA_ADP_Task(uint32_t param)
{
    _mqx_uint sts;
    MWA_Event_t *event;

    // --------------------------------------- Initialzie OS Dependent Part ---
    sts = _timer_create_component(TIMER_DEFAULT_TASK_PRIORITY,
                                  TIMER_DEFAULT_STACK_SIZE);
    if (sts != MQX_OK) {
        err_printf("Cannot Create Timer Component\n");
        _task_block();
    }

    // ---------------------------------------------------- Open Connection ---
    IPCFG_IP_ADDRESS_DATA ip_data;
    ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &ip_data);
    char prn_addr[RTCS_IP4_ADDR_STR_SIZE];
    inet_ntop(AF_INET, &ip_data.ip, prn_addr, sizeof(prn_addr));
    if (!UDP_Open(prn_addr, "224.0.23.0", 3610, &gUDP)) {
        err_printf("Cannot open %s\n", prn_addr);
        _task_block();
    }

    // -------------------------------------------------- Setup UART Device ---
    MWA_SetupUART(&gUART, UART_DEVIVCE_NAME);

    // -------------------------------------- Initialize Middleware Adapter ---
    if (!MWA_InitAdapter(&gUDP, &gUART)) {
        /*** Failed to Initialiezd ***/
        err_printf("Failed to Initialize Middleware Adapter...\n");
        _task_block();
    }

    _task_create(0, MWA_UDP_TASK, 0);
    _task_create(0, MWA_UART_TASK, 0);
    _task_create(0, MWA_FRM_TASK, 0);
    _task_create(0, MWA_ENL_TASK, 0);
    _task_create(0, MWA_INF_TASK, 0);

    while (1) {
        event = MWA_GetAdapterEvent();
        if (event != NULL) {
            MWA_HandleAdapter(event);
        }
        _time_delay(50);
    }
}

//=============================================================================
void MWA_UDP_Task(uint32_t param)
{
    IPv4Addr_t recv_addr;
    int recv_size;

    while (1) {
        recv_size = UDP_Recv(&gUDP, gRecvUDPBuffer, RECV_UDP_BUFFER_SIZE,
                             0, &recv_addr);
        if (recv_size > 0) {
            ELLDbg_PrintPacket("from", recv_addr, gRecvUDPBuffer, recv_size);

            if (!MWA_AddRecvPacketToQueue(gRecvUDPBuffer,
                                          recv_size, recv_addr)) {
                /*** Failed ***/
                err_printf("Cannot UDP Packet Queing...\n");
            }
        }
    }
}

//=============================================================================
void MWA_UART_Task(uint32_t param)
{
    int recv_size;
    int cnt;

    while (1) {
        recv_size = MWA_ReadUART(&gUART,
                                 gRecvUARTBuffer, RECV_UART_BUFFER_SIZE);
        for (cnt = 0; cnt < recv_size; cnt ++) {
            if (MWA_RecvData(gRecvUARTBuffer[cnt])) {
                if (!MWA_ConfirmRecvFrame()) {
                    break;
                }
            }
        }
        if (recv_size <= 0) {
            _time_delay(30);
        }
    }
}

//=============================================================================
void MWA_Frame_Task(uint32_t param)
{
    MWA_Event_t *event;

    while (1) {
        event = MWA_GetFrameEvent();
        if (event != NULL) {
            MWA_HandleFrameComm(event);
        }
        _time_delay(50);
    }
}

//=============================================================================
void MWA_ENL_Task(uint32_t param)
{
    MWA_Event_t *event;

    while (1) {
        event = MWA_GetENLEvent();
        if (event != NULL) {
            MWA_HandleENLPacket(event);
        }
        _time_delay(50);
    }
}

//=============================================================================
void MWA_INF_Task(uint32_t param)
{
    while (1) {
        MWA_CheckAnnounce();
        _time_delay(1000);
    }
}

/******************************** END-OF-FILE ********************************/
