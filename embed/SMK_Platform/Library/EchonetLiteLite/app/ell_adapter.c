/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include <stdio.h>

#include "echonetlitelite.h"
#include "connection.h"
#include "ell_adapter.h"

#include <arpa/inet.h>

//=============================================================================
extern void printNetworkInterfaces(void);

extern MWA_TimerHandle_t gMWA_SMTimer;
extern MWA_TimerHandle_t gMWA_ENLTimer;

#define TIME_SLICE (5000)

//=============================================================================
static uint8_t gRecvUDPBuffer[1024];
static uint8_t gRecvUARTBuffer[256];

//=============================================================================
UDP_Handle_t gUDP;
MWA_UARTHandle_t gUART;

//=============================================================================
// Return Value:
//   -1: Timeout
//    0: UDP
//    1: UART
//=============================================================================
int MWA_Select(UDP_Handle_t *udp, MWA_UARTHandle_t *uart, int timeout)
{
    fd_set fds;
    int maxfd;
    struct timeval tv;

    ELL_Assert(udp != NULL && uart != NULL);

    FD_ZERO(&fds);
    FD_SET(udp->sock, &fds);
    FD_SET(uart->fd, &fds);
    maxfd = (udp->sock > uart->fd) ? udp->sock : uart->fd;

    if (timeout == 0) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    } else {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }

    if (timeout < 0) {
        select(maxfd + 1, &fds, NULL, NULL, NULL);
    } else {
        select(maxfd + 1, &fds, NULL, NULL, &tv);
    }

    if (FD_ISSET(udp->sock, &fds)) {
        return (0);
    } else if (FD_ISSET(uart->fd, &fds)) {
        return (1);
    }
    return (-1);
}

//=============================================================================
int main(int ac, char *av[])
{
    int timeout;
    int sel;

    IPv4Addr_t recv_addr;
    int recv_size;

    int cnt;

    int32_t cur_time, elapsed_time, delta_time;
    int32_t next_sm_timeout, next_enl_timeout;
    MWA_Event_t *event;
    uint8_t event_flag;

    // ------------------------------------------------------ Check Options ---
    if (ac < 3) {
        printf("\nUsage: $ %s IP_ADDRESS UART_DEVFILE\n\n"
               "select IP_ADDRESS from below:\n", av[0]);
        printNetworkInterfaces();
        printf("\n");
        exit(0);
        return (0);
    }

    // ---------------------------------------------------- Open Connection ---
    if (!UDP_Open(av[1], "224.0.23.0", 3610, &gUDP)) {
        printf("Cannot open %s\n", av[1]);
        return (0);
    }

    // -------------------------------------------------- Setup UART Device ---
    MWA_SetupUART(&gUART, av[2]);

    // -------------------------------------- Initialize Middleware Adapter ---
    if (!MWA_InitAdapter(&gUDP, &gUART)) {
        /*** Failed to Initialiezd ***/
    }

    // ---------------------------------------------------------- Main Loop ---
    // timeout = -1;
    timeout = MWA_GetNextTime(&gMWA_ENLTimer);
    delta_time = 0;
    while (1) {
        /*** Wait UDP and UART ***/
        cur_time = MWA_GetMilliSec();

        // printf("wait %dms\n", timeout);

        sel = MWA_Select(&gUDP, &gUART, timeout);

        // printf("...%d elapsed\n", MWA_GetMilliSec() - cur_time);

        if (sel == 0) {
            recv_size = UDP_Recv(&gUDP, gRecvUDPBuffer, 1024, 0, &recv_addr);
            if (!MWA_AddRecvPacketToQueue(gRecvUDPBuffer,
                                          recv_size, recv_addr)) {
                /*** Failed ***/
            }
        } else if (sel == 1) {
            do {
                recv_size = MWA_ReadUART(&gUART, gRecvUARTBuffer, 256);
                for (cnt = 0; cnt < recv_size; cnt ++) {
                    if (MWA_RecvData(gRecvUARTBuffer[cnt])) {
                        MWA_ConfirmRecvFrame();
                    }
                }
            } while (recv_size > 0);
        }
        elapsed_time = MWA_GetMilliSec() - cur_time + delta_time;

        /*** Wait Timer ***/
        if (MWA_ElapseTimer(&gMWA_SMTimer, elapsed_time)) {
            MWA_InvokeTimerCallback(&gMWA_SMTimer);
        }
        if (MWA_ElapseTimer(&gMWA_ENLTimer, elapsed_time)) {
            MWA_InvokeTimerCallback(&gMWA_ENLTimer);
        }

        /*** Wait Internal Event ***/
        cur_time = MWA_GetMilliSec();
        do {
            event_flag = 0;
            event = MWA_GetFrameEvent();
            if (event != NULL) {
                MWA_HandleFrameComm(event);
                event_flag |= 0x01;
            }
            event = MWA_GetENLEvent();
            if (event != NULL) {
                MWA_HandleENLPacket(event);
                event_flag |= 0x02;
            }
            event = MWA_GetAdapterEvent();
            if (event != NULL) {
                MWA_HandleAdapter(event);
                event_flag |= 0x03;
            }
        } while (event_flag != 0);

        delta_time = MWA_GetMilliSec() - cur_time;

        next_sm_timeout = MWA_GetNextTime(&gMWA_SMTimer);
        next_enl_timeout = MWA_GetNextTime(&gMWA_ENLTimer);
        if (next_sm_timeout > 0) {
            if (next_enl_timeout > 0 && next_enl_timeout < next_sm_timeout) {
                timeout = next_enl_timeout;
            } else {
                timeout = next_sm_timeout;
            }
        } else if (next_enl_timeout > 0) {
            timeout = next_enl_timeout;
        } else {
            timeout = -1;
        }

        /*** Adjust Timeout ***/
        if (delta_time < timeout) {
            // printf("timeout (%d), delta (%d)", timeout, delta_time);
            timeout -= delta_time;
        } else if (timeout > 0) {
            // printf("timeout (%d), delta (%d)", timeout, delta_time);
            timeout = 0;
        }

        MWA_CheckAnnounce();
    }

    return (0);
}

/******************************** END-OF-FILE ********************************/
