/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _ELL_ADAPTER_H
#define _ELL_ADAPTER_H

#include "echonetlitelite.h"
#include "mwa_depend.h"

//*****************************************************************************
// Types and Definitions
//*****************************************************************************

//=============================================================================
// Frame Type
//=============================================================================
typedef enum {
    MWA_FT_RECOGNIZE_IF    = 0xffff,

    MWA_FT_IDENTIFY_IF     = 0x0000,
    MWA_FT_INIT_ADAPTER    = 0x0001,
    MWA_FT_CONSTRUCT_OBJ   = 0x0002,
    MWA_FT_COMM_ECHONET    = 0x0003,

    MWA_FT_NOTIFY_COMM_ERR = 0x00ff
} MWA_FrameType_t;

//=============================================================================
// Middleware Adapter Frame
//=============================================================================
typedef struct {
    uint16_t frame_type;
    uint8_t cmd_no;
    uint8_t frame_no;
    uint16_t len;
    const uint8_t *data;
    uint16_t ptr; // for taking value from data
    uint8_t fcc;

    int uart_len;
    uint8_t *uart_buf;
} MWA_Frame_t;

//=============================================================================
typedef enum {
    MWA_BAUDRATE_2400   = 0x00,
    MWA_BAUDRATE_4800   = 0x01,
    MWA_BAUDRATE_9600   = 0x02,
    MWA_BAUDRATE_19200  = 0x03,
    MWA_BAUDRATE_38400  = 0x04,
    MWA_BAUDRATE_57600  = 0x05,
    MWA_BAUDRATE_115200 = 0x06,
    MWA_BAUDRATE_MAX
} MWA_Baudrate_t;

//=============================================================================
typedef enum {
    // For State Machine
    MWA_EVENT_SEND_FRAME = 0,
    MWA_EVENT_RECV_FRAME,
    MWA_EVENT_TIMEOUT,

    // For Frame Communication
    MWA_EVENT_RECV_UART,

    // For Echonet Lite
    MWA_EVENT_RECV_ENL_PACKET,
    MWA_EVENT_QUERY_PROP,
    MWA_EVENT_RESPONSE_PROP
} MWA_EventId_t;

//=============================================================================
typedef enum {
    MWA_RECV_FRAME_STATUS_SUCCESS = 0,

    MWA_RECV_FRAME_STATUS_PARITY_ERROR,
    MWA_RECV_FRAME_STATUS_FCC_ERROR,
    MWA_RECV_FRAME_STATUS_SHORT_FRAME,
    MWA_RECV_FRAME_STATUS_OVERFLOW
} MWA_RecvFrameStatus_t;

//=============================================================================
// MWA_Event_t
// MWA_SendFramePayload_t
//=============================================================================
typedef struct {
    MWA_Frame_t send_frame;
} MWA_SendFramePayload_t;

//=============================================================================
// MWA_Event_t
// MWA_RecvFramePayload_t
//=============================================================================
typedef struct {
    MWA_Frame_t recv_frame;
} MWA_RecvFramePayload_t;

//=============================================================================
// MWA_Event_t
// MWA_RecvUARTPayload_t
//=============================================================================
typedef struct {
    MWA_RecvFrameStatus_t status;
    int len;
    uint8_t *buf;
} MWA_RecvUARTPayload_t;

//=============================================================================
// MWA_Event_t
// MWA_TimeroutPayload_t
//=============================================================================
typedef struct {
    uint32_t tag;
} MWA_TimeroutPayload_t;

//=============================================================================
typedef struct {
    IPv4Addr_t addr;
    uint8_t *buf;
    int len;
} MWA_ENLPacketPayload_t;

//=============================================================================
typedef struct {
    uint32_t eoj;
    uint8_t epc;
    uint8_t pdc;
    uint8_t *edt;
} MWA_ENLPropertyPayload_t;

//=============================================================================
typedef void MWA_EnterStateCB_t(void);
typedef bool_t MWA_RecvFrameCB_t(MWA_Frame_t *frame);
typedef bool_t MWA_EventHandlerCB_t(MWA_Event_t *event);
typedef bool_t MWA_HandleENLPacketCB_t(MWA_ENLPacketPayload_t *payload);
typedef bool_t MWA_HandleENLPropCB_t(MWA_ENLPropertyPayload_t *payload);

//*****************************************************************************
// Function Declarations
//*****************************************************************************
bool_t MWA_GetByte(MWA_Frame_t *frame, uint8_t *value);
bool_t MWA_GetShort(MWA_Frame_t *frame, uint16_t *value);
bool_t MWA_GetByteArray(MWA_Frame_t *frame, uint8_t *buf, int len);
const uint8_t *MWA_GetPointer(MWA_Frame_t *frame);

bool_t MWA_UnpackFrame(const uint8_t *buf, int len, MWA_Frame_t *frame);
int MWA_PackFrame(MWA_Frame_t *frame, uint8_t *buf, int max);

bool_t MWA_PrepareFrame(MWA_Frame_t *frame, uint16_t frame_type,
                        uint8_t cmd_no, uint8_t frame_no, int fd_len);
bool_t MWA_SetFrameData(MWA_Frame_t *frame, const uint8_t *data, int len);
bool_t MWA_SetFrameFCC(MWA_Frame_t *frame);
void MWA_FreeFrame(MWA_Frame_t *frame);
void MWA_SendErrorFrame(uint8_t cmd_no, uint8_t frame_no);

void MWA_RecvParityError(void);
bool_t MWA_ConfirmRecvFrame(void);
bool_t MWA_RecvData(uint8_t data);
bool_t MWA_SendFrame(uint8_t *buf, int len);
void MWA_OpenFrame(int baudrate);
void MWA_CloseFrame(void);

// ----------------------------------------------------------------------------
bool_t MWA_InitFrameComm(MWA_UARTHandle_t *uart);
bool_t MWA_HandleFrameComm(MWA_Event_t *event);
MWA_Event_t *MWA_GetFrameEvent(void);

// ----------------------------------------------------------------------------
void MWA_InitEchonet(UDP_Handle_t *udp);
void MWA_ResetEchonet(void);
bool_t MWA_StartEchonet(void);

MWA_Event_t *MWA_GetENLEvent(void);
bool_t MWA_SendPacket(void *handle, void *dest, const uint8_t *buf, int len);
bool_t MWA_AddRecvPacketToQueue(const uint8_t *buf, int len, IPv4Addr_t addr);

bool_t MWA_ResponseENLAccess(uint32_t eoj, uint8_t epc,
                             uint8_t pdc, const uint8_t *edt);

bool_t MWA_HandleENLPacket(MWA_Event_t *event);

bool_t MWA_SetProperty(uint32_t eoj,
                       uint8_t epc, uint8_t pdc, const uint8_t *edt);
uint8_t MWA_GetProperty(uint32_t eoj, uint8_t epc, uint8_t *edt, int max);
bool_t MWA_AnnoProperty(uint32_t eoj,
                        uint8_t epc, uint8_t pdc, const uint8_t *edt);

void MWA_CheckAnnounce(void);


// ----------------------------------------------------------------------------
bool_t MWA_InitAdapter(UDP_Handle_t *udp, MWA_UARTHandle_t *uart);
bool_t MWA_HandleAdapter(MWA_Event_t *event);
MWA_Event_t *MWA_GetAdapterEvent(void);

bool_t MWA_RequestENLAccess(uint32_t eoj, uint8_t epc,
                            uint8_t pdc, const uint8_t *edt);
bool_t MWA_SendFrameToStateMachine(const uint8_t *buf, int len);

// ----------------------------------------------------------------------------
void MWADbg_DumpMemory(const uint8_t *buf, int len);
void MWADbg_PrintFrame(MWA_Frame_t *frame);


#endif // !_ELL_ADAPTER_H

/******************************** END-OF-FILE ********************************/

