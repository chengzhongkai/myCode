/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

//*****************************************************************************
// Command Frame Communication (UART)
//*****************************************************************************

//=============================================================================
typedef enum {
    MWA_RECV_FRAME_STATE_WAIT_STX = 0,
    MWA_RECV_FRAME_STATE_RECEIVING,
    MWA_RECV_FRAME_STATE_WAIT_FCC,
    MWA_RECV_FRAME_STATE_COMPLETE,
    MWA_RECV_FRAME_STATE_PARITY_ERROR,
    MWA_RECV_FRAME_STATE_FCC_ERROR,
    MWA_RECV_FRAME_STATE_OVERFLOW
} MWA_RecvFrameState_t;

//=============================================================================
typedef enum {
    MWA_FRAME_IDX_STX = 0,
    MWA_FRAME_IDX_FT0,
    MWA_FRAME_IDX_FT1,
    MWA_FRAME_IDX_CN,
    MWA_FRAME_IDX_FN,
    MWA_FRAME_IDX_DL0,
    MWA_FRAME_IDX_DL1,
    MWA_FRAME_IDX_FD
} MWA_FrameIndex_t;

//=============================================================================
static MWA_RecvFrameState_t gRecvFrameState = MWA_RECV_FRAME_STATE_WAIT_STX;

#define MWA_RECV_FRAME_BUFFER_SIZE 2048
static uint8_t gRecvFrameBuffer[MWA_RECV_FRAME_BUFFER_SIZE];
static uint16_t gRecvFrameIdx;

//=============================================================================
MWA_UARTHandle_t *gpUART = NULL;

MWA_EventQueue_t gMWA_FrameCommEventQ;

//=============================================================================
static void MWA_AddRecvFrameToQueue(MWA_RecvFrameStatus_t status,
                                    const uint8_t *buf, int len)
{
    MWA_Event_t *event;
    MWA_RecvUARTPayload_t *payload;

    if (MWA_IsQueueFull(&gMWA_FrameCommEventQ)) {
        /*** DISCARD ***/
        return;
    }

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_RECV_UART, sizeof(MWA_RecvUARTPayload_t));
    if (event == NULL) return;

    payload = (MWA_RecvUARTPayload_t *)(event + 1);

    // ---------------------------------------------------- Allocate Buffer ---
    payload->buf = MEM_Alloc(len);
    if (payload->buf == NULL) {
        MWA_FreeEvent(event);
        return;
    }

    // ------------------------------------------- Send Event to Frame Comm ---
    payload->status = status;
    payload->len = len;
    MEM_Copy(payload->buf, buf, len);

    if (!MWA_AddEventToQueue(&gMWA_FrameCommEventQ, event)) {
        MEM_Free(payload->buf);
        MWA_FreeEvent(event);
    }
}

//=============================================================================
static void MWA_ResetUARTRecvTimer(void)
{
    /*** Platform Dependent ***/
}

//=============================================================================
#ifdef __FREESCALE_MQX__
static LWSEM_STRUCT gRecvRscSem;
#endif

//=============================================================================
static void MWA_InitLockRecv(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_create(&gRecvRscSem, 1);
#endif
}

//=============================================================================
static void MWA_LockRecv(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_wait(&gRecvRscSem);
#endif
}

//=============================================================================
static void MWA_UnlockRecv(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_post(&gRecvRscSem);
#endif
}

//=============================================================================
void MWA_RecvParityError(void)
{
    MWA_LockRecv();

    if (gRecvFrameState == MWA_RECV_FRAME_STATE_WAIT_STX
        || gRecvFrameState == MWA_RECV_FRAME_STATE_RECEIVING
        || gRecvFrameState == MWA_RECV_FRAME_STATE_WAIT_FCC) {
        gRecvFrameState = MWA_RECV_FRAME_STATE_PARITY_ERROR;
    }

    MWA_UnlockRecv();
}

//=============================================================================
bool_t MWA_ConfirmRecvFrame(void)
{
    bool_t ret = FALSE;

    MWA_LockRecv();

    switch (gRecvFrameState) {
    case MWA_RECV_FRAME_STATE_WAIT_STX:
        break;

    case MWA_RECV_FRAME_STATE_RECEIVING:
    case MWA_RECV_FRAME_STATE_WAIT_FCC:
        MWA_AddRecvFrameToQueue(MWA_RECV_FRAME_STATUS_SHORT_FRAME,
                                gRecvFrameBuffer, gRecvFrameIdx);
        break;

    case MWA_RECV_FRAME_STATE_COMPLETE:
        ELL_Printf("Rx: ");
        MWADbg_DumpMemory(gRecvFrameBuffer, gRecvFrameIdx);

        MWA_AddRecvFrameToQueue(MWA_RECV_FRAME_STATUS_SUCCESS,
                                gRecvFrameBuffer, gRecvFrameIdx);
        ret = TRUE;
        break;

    case MWA_RECV_FRAME_STATE_PARITY_ERROR:
        MWA_AddRecvFrameToQueue(MWA_RECV_FRAME_STATUS_PARITY_ERROR,
                                gRecvFrameBuffer, gRecvFrameIdx);
        break;

    case MWA_RECV_FRAME_STATE_FCC_ERROR:
        MWA_AddRecvFrameToQueue(MWA_RECV_FRAME_STATUS_FCC_ERROR,
                                gRecvFrameBuffer, gRecvFrameIdx);
        break;

    case MWA_RECV_FRAME_STATE_OVERFLOW:
        MWA_AddRecvFrameToQueue(MWA_RECV_FRAME_STATUS_OVERFLOW,
                                gRecvFrameBuffer, gRecvFrameIdx);
        break;
    }

    gRecvFrameState = MWA_RECV_FRAME_STATE_WAIT_STX;

    MWA_UnlockRecv();

    return (ret);
}

//=============================================================================
// Receive One Byte Data
//   It might be called from UART Rx ISR
//=============================================================================
bool_t MWA_RecvData(uint8_t data)
{
    static uint16_t gDL;
    static uint16_t gFD_size;
    static uint8_t gFCC;

    MWA_ResetUARTRecvTimer();

    // MWA_LockRecv();

    switch (gRecvFrameState) {
    case MWA_RECV_FRAME_STATE_WAIT_STX:
        // ---------------------------------------------------- Receive STX ---
        gRecvFrameBuffer[0] = data;
        gRecvFrameIdx = 1;
        if (data == 0x02) {
            gFCC = 0;
            gRecvFrameState = MWA_RECV_FRAME_STATE_RECEIVING;
        } else {
            // Frame Format Error
            // gRecvFrameState = MWA_RECV_FRAME_STATE_PARITY_ERROR;
        }
        break;

    case MWA_RECV_FRAME_STATE_RECEIVING:
        // ---------------------------------------------- Receive DATA Part ---
        gFCC += data;
        gRecvFrameBuffer[gRecvFrameIdx] = data;
        switch (gRecvFrameIdx) {
        case MWA_FRAME_IDX_STX:
        case MWA_FRAME_IDX_FT0:
        case MWA_FRAME_IDX_FT1:
        case MWA_FRAME_IDX_CN:
        case MWA_FRAME_IDX_FN:
            break;
        case MWA_FRAME_IDX_DL0:
            gDL = (uint16_t)data << 8;
            break;
        case MWA_FRAME_IDX_DL1:
            gDL |= (uint16_t)data;
            gFD_size = 0;
            if (gDL == 0) {
                gRecvFrameState = MWA_RECV_FRAME_STATE_WAIT_FCC;
            }
            break;
        default:
            gFD_size ++;
            if ((gRecvFrameIdx + 1) >= MWA_RECV_FRAME_BUFFER_SIZE) {
                gRecvFrameState = MWA_RECV_FRAME_STATE_OVERFLOW;
            }
            else if (gFD_size == gDL) {
                gRecvFrameState = MWA_RECV_FRAME_STATE_WAIT_FCC;
            }
            break;
        }
        gRecvFrameIdx ++;
        break;

    case MWA_RECV_FRAME_STATE_WAIT_FCC:
        // ---------------------------------------------------- Receive FCC ---
        gRecvFrameBuffer[gRecvFrameIdx] = data;
        gRecvFrameIdx ++;
        gFCC = (~gFCC) + 1;
        if (data == gFCC) {
            gRecvFrameState = MWA_RECV_FRAME_STATE_COMPLETE;
        } else {
            gRecvFrameState = MWA_RECV_FRAME_STATE_FCC_ERROR;
        }
        break;

    case MWA_RECV_FRAME_STATE_COMPLETE:
        // ----------------------------------------- Already Complete Frame ---
        /*** Receive Extra Data ***/
        // => It should be FORMAT_ERROR
        break;

    case MWA_RECV_FRAME_STATE_PARITY_ERROR:
        // ------------------------------------------ Parity Error Occurred ---
        break;

    case MWA_RECV_FRAME_STATE_FCC_ERROR:
        // --------------------------------------------- FCC Error Occurred ---
        break;

    case MWA_RECV_FRAME_STATE_OVERFLOW:
        // ---------------------------------------- Receive Buffer Overflow ---
        /*** Skip Data ***/
        break;
    }

    // MWA_UnlockRecv();

    if (gRecvFrameState == MWA_RECV_FRAME_STATE_COMPLETE
        || gRecvFrameState == MWA_RECV_FRAME_STATE_FCC_ERROR) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

//=============================================================================
bool_t MWA_SendFrame(uint8_t *buf, int len)
{
    int send_size;

    if (buf == NULL || len == 0) return (FALSE);

    ELL_Printf("Tx: ");
    MWADbg_DumpMemory(buf, len);

    /*** Wait ***/
    MWA_WaitMilliSec(10);

    send_size = MWA_WriteUART(gpUART, buf, len);

    if (send_size == len) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

//=============================================================================
void MWA_OpenFrame(int baudrate)
{
    if (!MWA_OpenUART(gpUART, baudrate)) {
        MWA_WaitMilliSec(300);
        ELL_Printf("Open UART Error, retry\n");
        MWA_OpenUART(gpUART, baudrate);
    }
}

//=============================================================================
void MWA_CloseFrame(void)
{
    MWA_CloseUART(gpUART);
}

//*****************************************************************************
// Frame Communication Handler
//*****************************************************************************

//=============================================================================
static uint8_t *gSendFrameBuffer = NULL;
static int gSendFrameLen = 0;

//=============================================================================
// Handle Send Frame Event
//   Store the Pointer to UART Buffer, to gSendFrameBuffer
//=============================================================================
static bool_t MWA_HandleFrameComm_SendFrame(MWA_Event_t *event)
{
    MWA_SendFramePayload_t *payload;
    bool_t handled = FALSE;

    ELL_Assert(event != NULL);

    payload = (MWA_SendFramePayload_t *)(event + 1);

    if (payload->send_frame.uart_buf != NULL) {
        // ----------------------------------------------------- Send Frame ---
        handled = MWA_SendFrame(payload->send_frame.uart_buf,
                                payload->send_frame.uart_len);
        if (handled) {
            if (gSendFrameBuffer != NULL) {
                MEM_Free(gSendFrameBuffer);
            }
            gSendFrameBuffer = payload->send_frame.uart_buf;
            gSendFrameLen = payload->send_frame.uart_len;
        } else {
            MEM_Free(payload->send_frame.uart_buf);
        }
    }

    return (handled);
}

//=============================================================================
// Handle Receive UART Frame Event
//=============================================================================
static bool_t MWA_HandleFrameComm_RecvUART(MWA_Event_t *event)
{
    MWA_RecvUARTPayload_t *payload;
    bool_t handled = FALSE;

    uint8_t frame_no;

    ELL_Assert(event != NULL);

    payload = (MWA_RecvUARTPayload_t *)(event + 1);

    switch (payload->status) {
    case MWA_RECV_FRAME_STATUS_SUCCESS:
        /*** Send Event to State Machine ***/
        handled = MWA_SendFrameToStateMachine(payload->buf, payload->len);
        break;

    case MWA_RECV_FRAME_STATUS_PARITY_ERROR:
        /*** Discard ***/
        frame_no = (payload->len >= 5) ? payload->buf[4] : 0x00;
        MWA_SendErrorFrame(0xff, frame_no);
        break;

    case MWA_RECV_FRAME_STATUS_FCC_ERROR:
        /*** Discard ***/
        frame_no = (payload->len >= 5) ? payload->buf[4] : 0x00;
        MWA_SendErrorFrame(0x00, frame_no);
        break;

    case MWA_RECV_FRAME_STATUS_SHORT_FRAME:
        /*** Discard ***/
        frame_no = (payload->len >= 5) ? payload->buf[4] : 0x00;
        MWA_SendErrorFrame(0x03, frame_no);
        break;

    case MWA_RECV_FRAME_STATUS_OVERFLOW:
        /*** Discard ***/
        frame_no = (payload->len >= 5) ? payload->buf[4] : 0x00;
        MWA_SendErrorFrame(0x03, frame_no);
        break;

    default:
        /*** Discard ***/
        frame_no = (payload->len >= 5) ? payload->buf[4] : 0x00;
        MWA_SendErrorFrame(0xff, frame_no);
        break;
    }

    if (!handled) {
        if (payload->buf != NULL) {
            MEM_Free(payload->buf);
            payload->len = 0;
            payload->buf = NULL;
        }
    }

    return (handled);
}

//=============================================================================
// Handle Event for Frame Communication
//=============================================================================
bool_t MWA_HandleFrameComm(MWA_Event_t *event)
{
    bool_t handled = FALSE;

    switch (event->id) {
    case MWA_EVENT_SEND_FRAME:
        /*** from State Machine ***/
        handled = MWA_HandleFrameComm_SendFrame(event);
        break;

    case MWA_EVENT_RECV_UART:
        /*** from UART Frame Receiving ***/
        handled = MWA_HandleFrameComm_RecvUART(event);
        break;
    }

    MWA_FreeEvent(event);

    return (handled);
}

//=============================================================================
// Initailize Frame Communication
//=============================================================================
bool_t MWA_InitFrameComm(MWA_UARTHandle_t *uart)
{
    if (!MWA_InitEventQueue(&gMWA_FrameCommEventQ)) {
        return (FALSE);
    }

    MWA_InitLockRecv();

    gSendFrameBuffer = NULL;

    gpUART = uart;

    return (TRUE);
}

//=============================================================================
// Get Event for Frame Communication
//=============================================================================
MWA_Event_t *MWA_GetFrameEvent(void)
{
    return (MWA_GetEventFromQueue(&gMWA_FrameCommEventQ));
}


//*****************************************************************************
// Middleware Adapter Frame Utilities
//*****************************************************************************

//=============================================================================
bool_t MWA_GetByte(MWA_Frame_t *frame, uint8_t *value)
{
    ELL_Assert(frame != NULL);

    if (value == NULL) return (FALSE);

    if ((frame->ptr + 1) > frame->len) {
        *value = 0;
        return (FALSE);
    }

    *value = frame->data[frame->ptr];
    frame->ptr ++;

    return (TRUE);
}

//=============================================================================
bool_t MWA_GetShort(MWA_Frame_t *frame, uint16_t *value)
{
    ELL_Assert(frame != NULL);

    if (value == NULL) return (FALSE);

    if ((frame->ptr + 2) > frame->len) {
        *value = 0;
        return (FALSE);
    }

    *value = ((uint16_t)frame->data[frame->ptr] << 8)
        | (uint16_t)frame->data[frame->ptr + 1];
    frame->ptr += 2;

    return (TRUE);
}

//=============================================================================
bool_t MWA_GetByteArray(MWA_Frame_t *frame, uint8_t *buf, int len)
{
    ELL_Assert(frame != NULL);

    if (buf == NULL || len == 0) return (FALSE);

    if ((frame->ptr + len) > frame->len) {
        MEM_Set(buf, 0, len);
        return (FALSE);
    }

    MEM_Copy(buf, &frame->data[frame->ptr], len);
    frame->ptr += len;

    return (TRUE);
}

//=============================================================================
const uint8_t *MWA_GetPointer(MWA_Frame_t *frame)
{
    ELL_Assert(frame != NULL);

    if (frame->ptr == frame->len) return (NULL);

    return (&frame->data[frame->ptr]);
}

//=============================================================================
bool_t MWA_UnpackFrame(const uint8_t *buf, int len, MWA_Frame_t *frame)
{
    if (buf == NULL || frame == NULL) return (FALSE);

    if (len < 8) return (FALSE);
    if (buf[0] != 0x02) return (FALSE);

    frame->frame_type = ((uint16_t)buf[1] << 8) | (uint16_t)buf[2];
    frame->cmd_no = buf[3];
    frame->frame_no = buf[4];
    frame->len = ((uint16_t)buf[5] << 8) | (uint16_t)buf[6];

    // Check Frame Data Length
    if ((len - 8) != frame->len) return (FALSE);

    frame->data = &buf[7];

    frame->ptr = 0;

    // Check FCC
    frame->fcc = buf[len - 1];

    // Save Original Data
    frame->uart_len = len;
    frame->uart_buf = (uint8_t *)buf;

    return (TRUE);
}

//=============================================================================
int MWA_PackFrame(MWA_Frame_t *frame, uint8_t *buf, int max)
{
    int cnt;
    uint8_t fcc;

    if (frame == NULL || buf == NULL) return (0);

    if (max < (frame->len + 8)) return (0);

    buf[0] = 0x02;
    buf[1] = (uint8_t)((frame->frame_type >> 8) & 0xff);
    buf[2] = (uint8_t)(frame->frame_type & 0xff);
    buf[3] = frame->cmd_no;
    buf[4] = frame->frame_no;
    buf[5] = (uint8_t)((frame->len >> 8) & 0xff);
    buf[6] = (uint8_t)(frame->len & 0xff);
    MEM_Copy(&buf[7], frame->data, frame->len);

    // ----------------------------------------------------------- Calc FCC ---
    fcc = 0;
    for (cnt = 1; cnt < (frame->len + 7); cnt ++) {
        fcc += buf[cnt];
    }
    frame->fcc = (~fcc) + 1;
    buf[frame->len + 7] = frame->fcc;

    return (frame->len + 8);
}

//=============================================================================
bool_t MWA_PrepareFrame(MWA_Frame_t *frame, uint16_t frame_type,
                        uint8_t cmd_no, uint8_t frame_no, int fd_len)
{
    static uint8_t gNextFrameNo = 0x01;

    if (frame == NULL) return (FALSE);

    frame->uart_len = fd_len + 8;
    frame->uart_buf = MEM_Alloc(frame->uart_len);
    if (frame->uart_buf == NULL) return (FALSE);

    frame->frame_type = frame_type;
    frame->cmd_no = cmd_no;
    if (frame_no == 0) {
        frame->frame_no = gNextFrameNo;
        if (gNextFrameNo == 0xff) {
            gNextFrameNo = 0x01;
        } else {
            gNextFrameNo ++;
        }
    } else {
        frame->frame_no = frame_no;
    }
    frame->len = fd_len;
    frame->data = &frame->uart_buf[7];

    frame->uart_buf[0] = 0x02;
    frame->uart_buf[1] = (uint8_t)((frame->frame_type >> 8) & 0xff);
    frame->uart_buf[2] = (uint8_t)(frame->frame_type & 0xff);
    frame->uart_buf[3] = frame->cmd_no;
    frame->uart_buf[4] = frame->frame_no;
    frame->uart_buf[5] = (uint8_t)((frame->len >> 8) & 0xff);
    frame->uart_buf[6] = (uint8_t)(frame->len & 0xff);

    return (TRUE);
}

//=============================================================================
bool_t MWA_SetFrameData(MWA_Frame_t *frame, const uint8_t *data, int len)
{
    ELL_Assert(frame != NULL);

    if (data == NULL && len != 0) return (FALSE);
    if (len < 0 || len > 0x10000) return (FALSE);

    if (len > frame->len) return (FALSE);

    frame->len = (uint16_t)len;
    frame->uart_buf[5] = (uint8_t)((frame->len >> 8) & 0xff);
    frame->uart_buf[6] = (uint8_t)(frame->len & 0xff);

    MEM_Copy((uint8_t *)frame->data, data, len);

    return (TRUE);
}

//=============================================================================
bool_t MWA_SetFrameFCC(MWA_Frame_t *frame)
{
    int cnt;
    uint8_t fcc;

    ELL_Assert(frame != NULL);

    fcc = 0;
    for (cnt = 1; cnt < (frame->uart_len - 1); cnt ++) {
        fcc += frame->uart_buf[cnt];
    }
    frame->uart_buf[cnt] = (~fcc) + 1;

    return (TRUE);
}

//=============================================================================
void MWA_FreeFrame(MWA_Frame_t *frame)
{
    ELL_Assert(frame != NULL);

    if (frame->uart_buf != NULL) {
        MEM_Free(frame->uart_buf);
        frame->uart_buf = NULL;
    }
}

//=============================================================================
void MWA_SendErrorFrame(uint8_t cmd_no, uint8_t frame_no)
{
    MWA_Frame_t err_frame;

    if (!MWA_PrepareFrame(&err_frame, 0x00ff, cmd_no, frame_no, 0)) {
        return;
    }
    MWA_SetFrameFCC(&err_frame);

    MWA_SendFrame(err_frame.uart_buf, err_frame.uart_len);

    MWA_FreeFrame(&err_frame);
}

/******************************** END-OF-FILE ********************************/
