/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _MWA_DEPEND_H
#define _MWA_DEPEND_H

#include "ell_adapter.h"

#ifndef __FREESCALE_MQX__
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>
#else
#include <timer.h>
#endif

//=============================================================================
#ifdef __FREESCALE_MQX__
typedef struct {
    MQX_FILE *fd;
    int baudrate;
    char *dev_file;
} MWA_UARTHandle_t;
#else
typedef struct {
    int fd;
    int baudrate;
    struct termios oldtio;
    char *dev_file;
} MWA_UARTHandle_t;
#endif

//=============================================================================
typedef struct {
    uint8_t id; // MWA_EventId_t
    uint8_t reserved;
    uint16_t len;
} MWA_Event_t;

//=============================================================================
#define MWA_EVENT_QUEUE_SIZE 8
typedef struct {
    MWA_Event_t *queue[MWA_EVENT_QUEUE_SIZE];
    int w_ptr;
    int r_ptr;
    int size;
#ifdef __FREESCALE_MQX__
    LWSEM_STRUCT sem;
#endif
} MWA_EventQueue_t;

//=============================================================================
typedef void MWA_TimerCallback_t(void *data, uint32_t tag, bool_t cancel);

//=============================================================================
typedef struct {
#ifdef __FREESCALE_MQX__
    _timer_id id;
#else
    uint32_t cur_time;
    uint32_t next_time;
#endif
    MWA_TimerCallback_t *func;
    void *data;
    uint32_t tag;
} MWA_TimerHandle_t;

//*****************************************************************************
// Function Declarations
//*****************************************************************************

//=============================================================================
void MWA_SetupUART(MWA_UARTHandle_t *uart, void *device);
bool_t MWA_OpenUART(MWA_UARTHandle_t *uart, int baudrate);
bool_t MWA_CloseUART(MWA_UARTHandle_t *uart);
int MWA_WriteUART(MWA_UARTHandle_t *uart, const uint8_t *buf, int len);
int MWA_ReadUART(MWA_UARTHandle_t *uart, uint8_t *buf, int max);

//=============================================================================
bool_t MWA_InitEventQueue(MWA_EventQueue_t *queue);
bool_t MWA_AddEventToQueue(MWA_EventQueue_t *queue, MWA_Event_t *event);
MWA_Event_t *MWA_GetEventFromQueue(MWA_EventQueue_t *queue);
bool_t MWA_IsQueueFull(MWA_EventQueue_t *queue);
MWA_Event_t *MWA_AllocEvent(uint8_t id, int payload_len);
void MWA_FreeEvent(MWA_Event_t *event);

//=============================================================================
bool_t MWA_InitTimer(MWA_TimerHandle_t *timer,
                     MWA_TimerCallback_t *func, void *data);
void MWA_StartTimer(MWA_TimerHandle_t *timer, int milli_sec, uint32_t tag);
void MWA_CancelTimer(MWA_TimerHandle_t *timer);

uint32_t MWA_GetNextTime(MWA_TimerHandle_t *timer);
bool_t MWA_ElapseTimer(MWA_TimerHandle_t *timer, uint32_t milli_sec);
void MWA_InvokeTimerCallback(MWA_TimerHandle_t *timer);

uint32_t MWA_GetMilliSec(void);
void MWA_WaitMilliSec(uint32_t milli_sec);

#endif // !_MWA_DEPEND_H

/******************************** END-OF-FILE ********************************/
