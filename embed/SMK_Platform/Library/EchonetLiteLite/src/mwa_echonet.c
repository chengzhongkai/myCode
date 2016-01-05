/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

//=============================================================================
UDP_Handle_t *gpUDP = NULL;

MWA_EventQueue_t gMWA_ENLPacketEventQ;
MWA_EventQueue_t gMWA_ENLHandlerEventQ;

MWA_TimerHandle_t gMWA_ENLTimer;

typedef enum {
    MWA_ENL_STATE_NONE,
    MWA_ENL_STATE_WAIT,
    MWA_ENL_STATE_RESPOND,
    MWA_ENL_STATE_MAX
} MWA_ENLState_t;

static MWA_ENLState_t gENLState;

//=============================================================================
typedef struct {
    ELL_Header_t header;
    ELL_Iterator_t it;
    uint8_t *buf;
    int len;
    IPv4Addr_t addr;

    ELL_Object_t *cur_obj;
    uint8_t res_esv;
    ELL_EDATA_t send_edata;

    uint8_t set_get;

    /*** EPC ***/
    ELL_Access_t access;
    ELL_Property_t prop;
} MWA_ENLResponse_t;

MWA_ENLResponse_t gMWAENLResponse;

//=============================================================================
static uint8_t gSendBuffer[ELL_SEND_BUF_SIZE];
static uint8_t gEDTWork[ELL_EDT_WORK_SIZE];

//=============================================================================
static void MWA_EnterENLState_Wait(void);
static void MWA_EnterENLState_Respond(void);

//=============================================================================
#ifdef __FREESCALE_MQX__
static LWSEM_STRUCT gELLRscSem;
#endif

//=============================================================================
static void MWA_InitLockENL(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_create(&gELLRscSem, 1);
#endif
}

//=============================================================================
static void MWA_LockENL(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_wait(&gELLRscSem);
#endif
}

//=============================================================================
static void MWA_UnlockENL(void)
{
    /*** Platform Dependent ***/
#ifdef __FREESCALE_MQX__
    _lwsem_post(&gELLRscSem);
#endif
}

//=============================================================================
MWA_Event_t *MWA_GetENLEvent(void)
{
    MWA_Event_t *event = NULL;

    switch (gENLState) {
    case MWA_ENL_STATE_WAIT:
        // gMWA_ENLPacketEventQ => gMWA_ENLHandlerEventQ
        event = MWA_GetEventFromQueue(&gMWA_ENLPacketEventQ);
        if (event == NULL) {
            event = MWA_GetEventFromQueue(&gMWA_ENLHandlerEventQ);
        }
        break;

    case MWA_ENL_STATE_RESPOND:
        // gMWA_ENLHandlerEventQ
        event = MWA_GetEventFromQueue(&gMWA_ENLHandlerEventQ);
        break;

    default:
        break;
    }

    return (event);
}

//=============================================================================
static void MWA_ENLTimerCallback(void *data, uint32_t tag, bool_t cancel)
{
    MWA_Event_t *event;
    MWA_TimeroutPayload_t *payload;

    if (!cancel) {
        event = MWA_AllocEvent(MWA_EVENT_TIMEOUT,
                               sizeof(MWA_TimeroutPayload_t));
        if (event == NULL) return;

        payload = (MWA_TimeroutPayload_t *)(event + 1);
        payload->tag = tag;

        if (!MWA_AddEventToQueue(&gMWA_ENLHandlerEventQ, event)) {
            MWA_FreeEvent(event);
        }
    }
}

//=============================================================================
bool_t MWA_SendPacket(void *handle, void *dest, const uint8_t *buf, int len)
{
    if (handle == NULL) return (FALSE);

    if (dest == NULL) {
        ELLDbg_PrintPacket("to", 0, buf, len);
        return (UDP_Multicast(handle, buf, len));
    } else {
        IPv4Addr_t addr = *((IPv4Addr_t *)dest);
        ELLDbg_PrintPacket("to", addr, buf, len);
        return (UDP_Send(handle, addr, buf, len));
    }
}

//=============================================================================
// Add Received Packet to Receiving Queue
//=============================================================================
bool_t MWA_AddRecvPacketToQueue(const uint8_t *buf, int len, IPv4Addr_t addr)
{
    MWA_Event_t *event;
    MWA_ENLPacketPayload_t *payload;

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_RECV_ENL_PACKET,
                           sizeof(MWA_ENLPacketPayload_t));
    if (event == NULL) {
        return (FALSE);
    }

    payload = (MWA_ENLPacketPayload_t *)(event + 1);

    // --------------------------------------------- Allocate Packet Buffer ---
    payload->addr = addr;
    payload->buf = MEM_Alloc(len);
    if (payload->buf == NULL) {
        MWA_FreeEvent(event);
        return (FALSE);
    }
    MEM_Copy(payload->buf, buf, len);
    payload->len = len;

    // --------------------------------------------------------- Send Event ---
    if (!MWA_AddEventToQueue(&gMWA_ENLPacketEventQ, event)) {
        MEM_Free(payload->buf);
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
void MWA_ResetEchonet(void)
{
}

//=============================================================================
void MWA_InitEchonet(UDP_Handle_t *udp)
{
    gpUDP = udp;

    MWA_InitEventQueue(&gMWA_ENLPacketEventQ);
    MWA_InitEventQueue(&gMWA_ENLHandlerEventQ);

    MWA_InitTimer(&gMWA_ENLTimer, MWA_ENLTimerCallback, NULL);

    MWA_InitLockENL();

    ELL_InitObjects();

    gENLState = MWA_ENL_STATE_NONE;
}

//=============================================================================
// Announce 0xD5 Property of Node Profile
//=============================================================================
static void MWA_StartUpENL(void *handle)
{
    int size;
    const uint8_t first_epc[] = { 0xd5, 0x00 };

    MWA_LockENL();

    size = ELL_MakeINFPacket(0, 0x0ef001, 0x0ef001,
                             first_epc, gSendBuffer, sizeof(gSendBuffer));

    MWA_SendPacket(handle, NULL, gSendBuffer, size);

    MWA_UnlockENL();
}

//=============================================================================
// Start ECHONET Lite (State None => State Wait)
//=============================================================================
bool_t MWA_StartEchonet(void)
{
    MWA_StartUpENL(gpUDP);

    MWA_EnterENLState_Wait();

    return (TRUE);
}

//=============================================================================
// Enter Wait State
//=============================================================================
static void MWA_EnterENLState_Wait(void)
{
    ELL_Printf("== [ENL] enter WAIT ===\n");

    gENLState = MWA_ENL_STATE_WAIT;
}

//=============================================================================
// Handle Receive ENL Packet (Wait)
//=============================================================================
static bool_t MWA_HandleENLPacket_Wait(MWA_ENLPacketPayload_t *payload)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    ELL_ESVType_t esv_type;

    if (payload == NULL) return (FALSE);

    // ----------------------------------------------------- Check ESV Type ---
    esv_type = ELL_ESVType(ELL_ESV(payload->buf));
    if (esv_type != ESV_TYPE_REQUEST) {
        MEM_Free(payload->buf);
        payload->buf = NULL;
        return (TRUE);
    }

    // ------------------------------------------------------- Parse Packet ---
    if (!ELL_SetPacketIterator(payload->buf,
                               payload->len, &res->it)) {
        MEM_Free(payload->buf);
        payload->buf = NULL;
        return (TRUE);
    }
    if (!ELL_GetHeader(payload->buf, payload->len, &res->header)) {
        MEM_Free(payload->buf);
        payload->buf = NULL;
        return (TRUE);
    }
    if (ELL_FindObject(res->header.deoj) == NULL) {
        MEM_Free(payload->buf);
        payload->buf = NULL;
        return (TRUE);
    }

    res->buf = payload->buf;
    res->len = payload->len;
    res->addr = payload->addr;
    payload->buf = NULL;

    MWA_EnterENLState_Respond();

    return (TRUE);
}

//=============================================================================
// Handle Access Property Event (Wait)
//=============================================================================
static bool_t MWA_HandleENLProp_Wait(MWA_ENLPropertyPayload_t *payload)
{
    return (FALSE);
}

//=============================================================================
// Handle Timeout (Wait)
//=============================================================================
static bool_t MWA_HandleENLTimeout_Wait(MWA_Event_t *event)
{
    return (TRUE);
}

//=============================================================================
// 
//=============================================================================
static bool_t MWA_StartENLResponse(void)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    // ------------------------------------------- Prepare Send Packet Work ---
    res->cur_obj = ELL_FindObject(res->header.deoj);
    if (res->cur_obj == NULL) {
        /*** ERROR ***/
        ELL_Printf("no object %06X\n\n", res->header.deoj);
        return (FALSE);
    }
    res->res_esv = ELL_GetESVRes(res->header.esv);

    ELL_PrepareEDATA(gSendBuffer, sizeof(gSendBuffer), &res->send_edata);

    return (TRUE);
}

//=============================================================================
// Send ENL Access Response Event to ECHONET Lite Communication
//=============================================================================
bool_t MWA_ResponseENLAccess(uint32_t eoj, uint8_t epc,
                             uint8_t pdc, const uint8_t *edt)
{
    MWA_Event_t *event;;
    MWA_ENLPropertyPayload_t *payload;

    if (edt == NULL && pdc != 0) return (FALSE);

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_RESPONSE_PROP,
                           sizeof(MWA_ENLPropertyPayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_ENLPropertyPayload_t *)(event + 1);

    // --------------------------------------------------------- Make Frame ---
    payload->eoj = eoj;
    payload->epc = epc;
    payload->pdc = pdc;

    if (payload->pdc > 0) {
        payload->edt = MEM_Alloc(pdc);
        if (payload->edt == NULL) {
            MWA_FreeEvent(event);
            return (FALSE);
        }
        MEM_Copy(payload->edt, edt, pdc);
    } else {
        payload->edt = NULL;
    }

    // --------------------------- Send Event to ECHONET Lite Communication ---
    if (!MWA_AddEventToQueue(&gMWA_ENLHandlerEventQ, event)) {
        if (payload->edt != NULL) {
            MEM_Free(payload->edt);
        }
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// 
//=============================================================================
static bool_t MWA_ProcessENLResponse(void)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    uint8_t pdc;
    uint8_t flag;
    int send_size;
    bool_t set_ok;

    MWA_LockENL();

    // ------------------------------------------------ Process Each Requet ---
    while (ELL_HasNextProperty(&res->it)) {
        res->access = ELL_GetNextProperty(&res->it, &res->prop);
        ELLDbg_PrintProperty(&res->prop);

        // ---------------- Proccess Each Property and Make Response Packet ---
        if (res->header.esv == ESV_SetGet
            && res->set_get == 0 && res->access == ELL_Get) {
            ELL_SetNextOPC(&res->send_edata);
            res->set_get = 1;
        }

        if (res->access == ELL_Get) {
            // --------------------------------------- Get Property Request ---
            flag = ELL_GetPropertyFlag(res->cur_obj, res->prop.epc);
            if ((flag & EPC_FLAG_RULE_GET) == 0 || res->prop.pdc != 0) {
                // ------------------------------------- No Get Access Rule ---
                pdc = 0;
            } else if ((flag & EPC_FLAG_RULE_GETUP) != 0) {
                // -------------------------------- Get Property by IAGetup ---
                if (MWA_RequestENLAccess(res->cur_obj->eoj,
                                         res->prop.epc, 0, NULL)) {
                    MWA_UnlockENL();
                    return (FALSE);
                }
                // IAGetup Failed
            } else {
                // ---------------------------------- Get Property Directly ---
                pdc = ELL_GetProperty(res->cur_obj, res->prop.epc,
                                      gEDTWork, ELL_EDT_WORK_SIZE);
            }

            ELL_AddEPC(&res->send_edata, res->prop.epc, pdc, gEDTWork);
            if (pdc == 0) {
                res->res_esv = ELL_GetESVSNA(res->header.esv);
            }
        }
        else if (res->access == ELL_Anno) {
            // ---------------------------------- Announce Property Request ---
            flag = ELL_GetPropertyFlag(res->cur_obj, res->prop.epc);
            if ((flag & (EPC_FLAG_RULE_ANNO | EPC_FLAG_RULE_GET)) == 0
                || res->prop.pdc != 0) {
                // ------------------------------------ No Anno Access Rule ---
                pdc = 0;
            } else if ((flag & EPC_FLAG_RULE_GETUP) != 0) {
                // -------------------------------- Get Property by IAGetup ---
                if (MWA_RequestENLAccess(res->cur_obj->eoj,
                                         res->prop.epc, 0, NULL)) {
                    MWA_UnlockENL();
                    return (FALSE);
                }
                // IAGetup Failed
            } else {
                // ---------------------------------- Get Property Directly ---
                pdc = ELL_GetProperty(res->cur_obj, res->prop.epc,
                                      gEDTWork, ELL_EDT_WORK_SIZE);
            }

            ELL_AddEPC(&res->send_edata, res->prop.epc, pdc, gEDTWork);
            if (pdc == 0) {
                res->res_esv = ELL_GetESVSNA(res->header.esv);
            }
        }
        else if (res->access == ELL_Set) {
            // --------------------------------------- Set Property Request ---
            pdc = res->prop.pdc;
            set_ok = FALSE;
            flag = ELL_GetPropertyFlag(res->cur_obj, res->prop.epc);
            if ((flag & EPC_FLAG_RULE_SET) == 0) {
                // ------------------------------------- No Set Access Rule ---
                ELL_Printf("No Set Rule [EPC:%02X]\n\n", res->prop.epc);
            }
            else if (res->prop.pdc == 0) {
                // ---------------------------------------- PDC = 0, no EDT ---
                ELL_Printf("PDC is 0\n\n");
            }
            else if ((flag & EPC_FLAG_RULE_SETUP) != 0) {
                // -------------------------------- Set Property by IASetup ---
                if (MWA_RequestENLAccess(res->cur_obj->eoj, res->prop.epc,
                                         res->prop.pdc, res->prop.edt)) {
                    MWA_UnlockENL();
                    return (FALSE);
                }
                // IASetup Failed
            }
            else if (!ELL_CheckProperty(res->cur_obj, res->prop.epc,
                                        res->prop.pdc, res->prop.edt)) {
                // ---------------------------- Failed to Check Value Range ---
                ELL_Printf("Check Property NG\n\n");
            }
            else {
                // ---------------------------------- Set Property Directly ---
                if (ELL_SetProperty(res->cur_obj, res->prop.epc,
                                    res->prop.pdc, res->prop.edt)) {
                    pdc = 0;
                    set_ok = TRUE;
                }
            }

            ELL_AddEPC(&res->send_edata, res->prop.epc, pdc, res->prop.edt);
            if (!set_ok) {
                res->res_esv = ELL_GetESVSNA(res->header.esv);
            }
        }
    }

    // ----------------------------------------------- Send Response Packet ---
    if (res->send_edata.packet[0] > 0 && res->res_esv != ESV_None) {
        if (res->res_esv == ESV_INF) {
            send_size = ELL_SetHeaderToEDATA(&res->send_edata, res->header.tid,
                                             res->header.deoj,
                                             res->header.seoj, res->res_esv);

            MWA_SendPacket(gpUDP, NULL, gSendBuffer, send_size);
        } else {
            send_size = ELL_SetHeaderToEDATA(&res->send_edata, res->header.tid,
                                             res->header.deoj,
                                             res->header.seoj, res->res_esv);

            MWA_SendPacket(gpUDP, &res->addr, gSendBuffer, send_size);
        }
    }

    MWA_UnlockENL();

    return (TRUE);
}

//=============================================================================
// 
//=============================================================================
static bool_t MWA_ProcessENLResponseLoop(void)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    while (1) {
        if (!MWA_ProcessENLResponse()) {
            // Wait Access Object Response
            return (FALSE);
        } else {
            if (res->header.multi_deoj) {
                ELL_SetPacketIterator(res->buf, res->len, &res->it);
                res->header.deoj = ELL_GetNextInstance(res->header.deoj);
                if (res->header.deoj != 0) {
                    // Process Event, Again
                } else {
                    // Finish
                    break;
                }
            } else {
                // Finish
                break;
            }
        }
    }

    MEM_Free(res->buf);
    return (TRUE);
}

//=============================================================================
// Enter Respond State
//=============================================================================
static void MWA_EnterENLState_Respond(void)
{
    ELL_Printf("== [ENL] enter RESPOND ===\n");

    if (!MWA_StartENLResponse()) {
        // No Response
        MEM_Free(gMWAENLResponse.buf);
        return;
    }

    if (MWA_ProcessENLResponseLoop()) {
        return;
    } else {
        MWA_StartTimer(&gMWA_ENLTimer, 3000, 0);
        gENLState = MWA_ENL_STATE_RESPOND;
    }
}

//=============================================================================
// Handle Receive ENL Packet (Respond)
//=============================================================================
static bool_t MWA_HandleENLPacket_Respond(MWA_ENLPacketPayload_t *payload)
{
    return (FALSE);
}

//=============================================================================
// Handle Access Property (Respond)
//=============================================================================
static bool_t MWA_HandleENLProp_Respond(MWA_ENLPropertyPayload_t *payload)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    uint8_t epc;
    uint8_t pdc;
    uint8_t *edt;

    /*** check each prop parameter ***/
    if (payload->epc >= 0x80) {
        epc = payload->epc;
        pdc = payload->pdc;
        edt = payload->edt;
    } else {
        epc = res->prop.epc;
        pdc = res->prop.pdc;
        edt = (uint8_t*)res->prop.edt;
    }

    switch (res->access) {
    case ELL_Get:
        ELL_AddEPC(&res->send_edata, epc, pdc, edt);
        if (pdc == 0) {
            res->res_esv = ELL_GetESVSNA(res->header.esv);
        }
        break;

    case ELL_Anno:
        ELL_AddEPC(&res->send_edata, epc, pdc, edt);
        if (pdc == 0) {
            res->res_esv = ELL_GetESVSNA(res->header.esv);
        }
        break;

    case ELL_Set:
        ELL_AddEPC(&res->send_edata, epc, pdc, edt);
        if (pdc != 0) {
            res->res_esv = ELL_GetESVSNA(res->header.esv);
        }
        break;

    default:
        break;
    }

    if (MWA_ProcessENLResponseLoop()) {
        MWA_CancelTimer(&gMWA_ENLTimer);
        MWA_EnterENLState_Wait();
    }

    return (TRUE);
}

//=============================================================================
// Handle Timeout (Respond)
//=============================================================================
static bool_t MWA_HandleENLTimeout_Respond(MWA_Event_t *event)
{
    MWA_ENLResponse_t *res = &gMWAENLResponse;

    // ---------------------- Process Remain All Process as Failed Response ---
    ELL_AddEPC(&res->send_edata, res->prop.epc, res->prop.pdc, res->prop.edt);
    res->res_esv = ELL_GetESVSNA(res->header.esv);

    MWA_ProcessENLResponseLoop();

    MWA_EnterENLState_Wait();

    return (TRUE);
}


//=============================================================================
// Event Handler Table
//=============================================================================
typedef struct {
    MWA_HandleENLPacketCB_t *recv_packet;
    MWA_HandleENLPropCB_t *resp_prop;
    MWA_EventHandlerCB_t *timeout;
} MWA_ENLHandlerTable_t;

static const MWA_ENLHandlerTable_t gENLHandlerTable[] = {
    { NULL, NULL },
    { MWA_HandleENLPacket_Wait,
      MWA_HandleENLProp_Wait,
      MWA_HandleENLTimeout_Wait },
    { MWA_HandleENLPacket_Respond,
      MWA_HandleENLProp_Respond,
      MWA_HandleENLTimeout_Respond }
};

//=============================================================================
bool_t MWA_HandleENLPacket(MWA_Event_t *event)
{
    bool_t handled = FALSE;

    switch (event->id) {
    case MWA_EVENT_RECV_ENL_PACKET:
        // ------------------------ From ECHONET Lite Packet Receiver (UDP) ---
        {
            MWA_ENLPacketPayload_t *payload;
            payload = (MWA_ENLPacketPayload_t *)(event + 1);

            if (gENLState < MWA_ENL_STATE_MAX) {
                MWA_HandleENLPacketCB_t *handler;
                handler = gENLHandlerTable[gENLState].recv_packet;
                if (handler != NULL) {
                    handled = (*handler)(payload);
                }
            }
        }
        break;

    case MWA_EVENT_RESPONSE_PROP:
        // --------------------------------------------- From State Machine ---
        {
            MWA_ENLPropertyPayload_t *payload;
            payload = (MWA_ENLPropertyPayload_t *)(event + 1);

            if (gENLState < MWA_ENL_STATE_MAX) {
                MWA_HandleENLPropCB_t *handler;
                handler = gENLHandlerTable[gENLState].resp_prop;
                if (handler != NULL) {
                    handled = (*handler)(payload);
                }
            }

            if (payload->edt != NULL) {
                MEM_Free(payload->edt);
            }
        }
        break;

    case MWA_EVENT_TIMEOUT:
        // -------------------------------------------------------- Timeout ---
        if (gENLState < MWA_ENL_STATE_MAX) {
            MWA_EventHandlerCB_t *handler;
            handler = gENLHandlerTable[gENLState].timeout;
            if (handler != NULL) {
                handled = (*handler)(event);
            }
        }
        break;

    default:
        break;
    }
    MWA_FreeEvent(event);

    return (handled);
}

//=============================================================================
bool_t MWA_SetProperty(uint32_t eoj,
                       uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    ELL_Object_t *obj;
    bool_t ret;

    MWA_LockENL();

    obj = ELL_FindObject(eoj);
    if (obj == NULL) {
        MWA_UnlockENL();
        return (FALSE);
    }
    ret = ELL_SetProperty(obj, epc, pdc, edt);

    MWA_UnlockENL();

    return (ret);
}

//=============================================================================
uint8_t MWA_GetProperty(uint32_t eoj, uint8_t epc, uint8_t *edt, int max)
{
    ELL_Object_t *obj;
    uint8_t pdc;

    MWA_LockENL();

    obj = ELL_FindObject(eoj);
    if (obj == NULL) {
        MWA_UnlockENL();
        return (0);
    }

    pdc = ELL_GetProperty(obj, epc, edt, max);

    MWA_UnlockENL();

    return (pdc);
}

//=============================================================================
bool_t MWA_AnnoProperty(uint32_t eoj,
                        uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    ELL_Object_t *obj;
    bool_t ret = FALSE;

    MWA_LockENL();

    obj = ELL_FindObject(eoj);
    if (obj == NULL) {
        MWA_UnlockENL();
        return (FALSE);
    }
    if (ELL_SetProperty(obj, epc, pdc, edt)) {
        ELL_NeedAnnounce(obj, epc);
        ret = TRUE;
    }

    MWA_UnlockENL();

    return (ret);
}

//=============================================================================
// Check EPC to Announce
//=============================================================================
void MWA_CheckAnnounce(void)
{
    int cnt, num_obj;
    ELL_Object_t *node, *obj;
    uint8_t pdc;
    uint8_t *edt;
    uint32_t eoj;

    int num_anno, send_size;
    uint8_t anno_list[16];

    if (gpUDP == NULL) return;
    if (gENLState == MWA_ENL_STATE_NONE) return;

    MWA_LockENL();

    node = ELL_FindObject(0x0ef001);
    if (node == NULL) {
        MWA_UnlockENL();
        return;
    }

    pdc = ELL_GetProperty(node, 0xd6, gEDTWork, ELL_EDT_WORK_SIZE);
    if (pdc > 0) {
        edt = gEDTWork;
        num_obj = *edt;
        edt ++;
        for (cnt = 0; cnt < num_obj; cnt ++) {
            eoj = ELL_EOJ(edt);
            edt += 3;

            obj = ELL_FindObject(eoj);
            if (obj == NULL) {
                continue;
            }

            num_anno = ELL_GetAnnounceEPC(obj, anno_list, 16);
            if (num_anno > 0) {
                send_size = ELL_MakeINFPacket(0, eoj, 0x0ef001, anno_list,
                                              gSendBuffer,
                                              sizeof(gSendBuffer));

                MWA_SendPacket(gpUDP, NULL, gSendBuffer, send_size);
            }
        }
    }

    MWA_UnlockENL();
}

//=============================================================================
void MWA_RestartENL(void)
{
    int size;
    const uint8_t first_epc[] = { 0xd5, 0x00 };

	if (gpUDP == NULL) return;

    MWA_LockENL();

	UDP_Reset(gpUDP);

    size = ELL_MakeINFPacket(0, 0x0ef001, 0x0ef001,
                             first_epc, gSendBuffer, sizeof(gSendBuffer));

    MWA_SendPacket(gpUDP, NULL, gSendBuffer, size);

    MWA_UnlockENL();
}

/******************************** END-OF-FILE ********************************/
