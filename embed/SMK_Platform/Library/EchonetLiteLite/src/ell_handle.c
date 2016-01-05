/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "echonetlitelite.h"
#include "connection.h"

//=============================================================================
static uint8_t gSendBuffer[ELL_SEND_BUF_SIZE];
static uint8_t gEDTWork[ELL_EDT_WORK_SIZE];

/*** Need To Initailize in RESET Function ***/
static ELL_SendPacketCB *gSendPacket = NULL;
static ELL_RecvPropertyCB *gRecvProperty = NULL;
static ELL_NotifyAnnoEPCCB *gNotifyAnnoEPC = NULL;

#define sendPacket(handle, to, buf, len) do {       \
        if (gSendPacket != NULL) {                  \
            (*gSendPacket)(handle, to, buf, len);   \
        }                                           \
    } while (0)
#define multicastPacket(handle, buf, len) do {        \
        if (gSendPacket != NULL) {                    \
            (*gSendPacket)(handle, NULL, buf, len);   \
        }                                             \
    } while (0)

/*** Need To Initailize in RESET Function ***/
static ELL_LockCB *gLockFunc = NULL;
static ELL_UnlockCB *gUnlockFunc = NULL;
static void *gLockData = NULL;

#define lockFunc() do {                         \
        if (gLockFunc != NULL) {                \
            (*gLockFunc)(gLockData);            \
        }                                       \
    } while (0)
#define unlockFunc() do {                         \
        if (gUnlockFunc != NULL) {                \
            (*gUnlockFunc)(gLockData);            \
        }                                         \
    } while (0)


//=============================================================================
// Set SendPacket Callback
//=============================================================================
void ELL_SetSendPacketCallback(ELL_SendPacketCB *send_packet)
{
    gSendPacket = send_packet;
}

//=============================================================================
// Set RecvProperty Callback
//=============================================================================
void ELL_SetRecvPropertyCallback(ELL_RecvPropertyCB *recv_prop)
{
    gRecvProperty = recv_prop;
}

//=============================================================================
// Set Lock/Unlock Function for Mutex
//=============================================================================
bool_t ELL_SetLockFunc(ELL_LockCB *lock, ELL_UnlockCB *unlock, void *data)
{
    if ((lock == NULL && unlock != NULL)
        || (lock != NULL && unlock == NULL)) {
        return (FALSE);
    }

    gLockFunc = lock;
    gUnlockFunc = unlock;
    gLockData = data;

    return (TRUE);
}

//=============================================================================
// Set Notify Announce EPC Callback
//=============================================================================
void ELL_SetNotifyAnnoEPCCallback(ELL_NotifyAnnoEPCCB *callback)
{
	gNotifyAnnoEPC = callback;
}

//=============================================================================
// Announce 0xD5 Property of Node Profile
//=============================================================================
void ELL_StartUp(void *handle)
{
    int size;
    const uint8_t first_epc[] = { 0xd5, 0x00 };

    lockFunc();

    size = ELL_MakeINFPacket(0, 0x0ef001, 0x0ef001,
                             first_epc, gSendBuffer, sizeof(gSendBuffer));

    multicastPacket(handle, gSendBuffer, size);

    unlockFunc();
}

//=============================================================================
// Handle Request Packet
// (SetI, SetC, Get, INF_REQ, SetGet)
//=============================================================================
static void ELL_HandleRequestPacket(ELL_Iterator_t *it, ELL_Header_t *header,
                                    void *handle, void *addr)
{
    ELL_Property_t prop;
    ELL_Object_t *cur_obj;
    ELL_Access_t access;
    uint8_t pdc;
    uint8_t flag;

    uint8_t res_esv;

    ELL_EDATA_t send_edata;
    int send_size;

    int set_get = 0;

    bool_t set_ok;

    // ------------------------------------------- Prepare Send Packet Work ---
    cur_obj = ELL_FindObject(header->deoj);
    if (cur_obj == NULL) {
#ifdef DEBUG
        ELL_Printf("no object %06X\n\n", header->deoj);
#endif
        return;
    }
    res_esv = ELL_GetESVRes(header->esv);

    lockFunc();

    ELL_PrepareEDATA(gSendBuffer, sizeof(gSendBuffer), &send_edata);

    // ------------------------------------------------ Process Each Requet ---
    while (ELL_HasNextProperty(it)) {
        access = ELL_GetNextProperty(it, &prop);
#ifdef DEBUG
        ELLDbg_PrintProperty(&prop);
#endif

        // ---------------- Proccess Each Property and Make Response Packet ---
        if (header->esv == ESV_SetGet
            && set_get == 0 && access == ELL_Get) {
            ELL_SetNextOPC(&send_edata);
            set_get = 1;
        }

        if (access == ELL_Get) {
            // --------------------------------------- Get Property Request ---
            flag = ELL_GetPropertyFlag(cur_obj, prop.epc);
            if ((flag & EPC_FLAG_RULE_GET) == 0 || prop.pdc != 0) {
                // ------------------------------------- No Get Access Rule ---
                pdc = 0;
            } else if ((flag & EPC_FLAG_RULE_GETUP) != 0
                       && cur_obj->get_cb != NULL) {
                // ------------------------------- Has Callback for Getting ---
                pdc = (*(ELL_GetCallback_t *)cur_obj->get_cb)(cur_obj,
															  prop.epc,
															  gEDTWork,
															  ELL_EDT_WORK_SIZE);
            } else {
                // ---------------------------------- Get Property Directly ---
                pdc = ELL_GetProperty(cur_obj, prop.epc,
                                      gEDTWork, ELL_EDT_WORK_SIZE);
            }

            ELL_AddEPC(&send_edata, prop.epc, pdc, gEDTWork);
            if (pdc == 0) {
                res_esv = ELL_GetESVSNA(header->esv);
            }
        }
        else if (access == ELL_Anno) {
            // ---------------------------------- Announce Property Request ---
            flag = ELL_GetPropertyFlag(cur_obj, prop.epc);
            if ((flag & (EPC_FLAG_RULE_ANNO | EPC_FLAG_RULE_GET)) == 0
                || prop.pdc != 0) {
                // ------------------------------------ No Anno Access Rule ---
                pdc = 0;
            } else if ((flag & EPC_FLAG_RULE_GETUP) != 0
                       && cur_obj->get_cb != NULL) {
                // ------------------------------- Has Callback for Getting ---
                pdc = (*(ELL_GetCallback_t *)cur_obj->get_cb)(cur_obj,
															  prop.epc,
															  gEDTWork,
															  ELL_EDT_WORK_SIZE);
            } else {
                // ---------------------------------- Get Property Directly ---
                pdc = ELL_GetProperty(cur_obj, prop.epc,
                                      gEDTWork, ELL_EDT_WORK_SIZE);
            }

            ELL_AddEPC(&send_edata, prop.epc, pdc, gEDTWork);
            if (pdc == 0) {
                res_esv = ELL_GetESVSNA(header->esv);
            }
        }
        else if (access == ELL_Set) {
            // --------------------------------------- Set Property Request ---
            pdc = prop.pdc;
            set_ok = FALSE;
            flag = ELL_GetPropertyFlag(cur_obj, prop.epc);
            if ((flag & EPC_FLAG_RULE_SET) == 0) {
                // ------------------------------------- No Set Access Rule ---
#ifdef DEBUG
                ELL_Printf("No Set Rule [EPC:%02X]\n\n", prop.epc);
#endif
            }
            else if (prop.pdc == 0) {
                // ---------------------------------------- PDC = 0, no EDT ---
#ifdef DEBUG
                ELL_Printf("PDC is 0\n\n");
#endif
            }
            else if ((flag & EPC_FLAG_RULE_SETUP) != 0
                     && cur_obj->set_cb != NULL) {
                // ------------------------------- Has Callback for Setting ---
                if ((*(ELL_SetCallback_t *)cur_obj->set_cb)(cur_obj,
															prop.epc, prop.pdc,
															prop.edt)) {
                    pdc = 0;
                    set_ok = TRUE;
                }
            }
            else if (!ELL_CheckProperty(cur_obj,
                                        prop.epc, prop.pdc, prop.edt)) {
                // ---------------------------- Failed to Check Value Range ---
#ifdef DEBUG
                ELL_Printf("Check Property NG\n\n");
#endif
            }
            else {
                // ---------------------------------- Set Property Directly ---
                if (ELL_SetProperty(cur_obj,
                                    prop.epc, prop.pdc, prop.edt)) {
                    pdc = 0;
                    set_ok = TRUE;
                }
            }

            ELL_AddEPC(&send_edata, prop.epc, pdc, prop.edt);
            if (!set_ok) {
                res_esv = ELL_GetESVSNA(header->esv);
            }
        }
    }

    // ----------------------------------------------- Send Response Packet ---
    if (send_edata.packet[0] > 0 && res_esv != ESV_None) {
        if (res_esv == ESV_INF) {
            send_size = ELL_SetHeaderToEDATA(&send_edata, header->tid,
                                             header->deoj,
                                             header->seoj, res_esv);

            multicastPacket(handle, gSendBuffer, send_size);
        } else {
            send_size = ELL_SetHeaderToEDATA(&send_edata, header->tid,
                                             header->deoj,
                                             header->seoj, res_esv);

            sendPacket(handle, addr, gSendBuffer, send_size);
        }
    }

    unlockFunc();
}

//=============================================================================
// Handle Response Packet
// (SetC_Res, Get_Res, INFC_Res, SetGet_Res)
// (SetI_SNA, SetC_SNA, Get_SNA, INF_SNA, SetGet_SNA)
//=============================================================================
static void ELL_HandleResponsePacket(ELL_Iterator_t *it, ELL_Header_t *header,
                                     void *handle, void *addr)
{
    ELL_Property_t prop;

    while (ELL_HasNextProperty(it)) {
        ELL_GetNextProperty(it, &prop);
#ifdef DEBUG
        ELLDbg_PrintProperty(&prop);
#endif

        if (gRecvProperty != NULL) {
            (*gRecvProperty)(header, addr, &prop);
        }
    }
}

//=============================================================================
// Handle Announce Packet
// (INF, INFC)
//=============================================================================
static void ELL_HandleAnnouncePacket(ELL_Iterator_t *it, ELL_Header_t *header,
                                     void *handle, void *addr)
{
    ELL_Property_t prop;

    ELL_EDATA_t send_edata;
    int send_size;

    // ------------------------------------- Ignore if DEOJ is not existent ---
    if (ELL_FindObject(header->deoj) == NULL) return;

    lockFunc();

    switch (header->esv) {
    case ESV_INF:
        // ---------------------------------------------- Handle INF Packet ---
        while (ELL_HasNextProperty(it)) {
            ELL_GetNextProperty(it, &prop);
#ifdef DEBUG
            ELLDbg_PrintProperty(&prop);
#endif

            if (gRecvProperty != NULL) {
                (*gRecvProperty)(header, addr, &prop);
            }
        }
        break;
    case ESV_INFC:
        // --------------------------------------------- Handle INFC Packet ---
        ELL_PrepareEDATA(gSendBuffer, sizeof(gSendBuffer), &send_edata);
        while (ELL_HasNextProperty(it)) {
            ELL_GetNextProperty(it, &prop);
#ifdef DEBUG
            ELLDbg_PrintProperty(&prop);
#endif

            ELL_AddEPC(&send_edata, prop.epc, 0, NULL);
        }
        send_size = ELL_SetHeaderToEDATA(&send_edata, header->tid,
                                         header->deoj, header->seoj,
                                         ELL_GetESVRes(header->esv));
        sendPacket(handle, addr, gSendBuffer, send_size);
        break;
    default:
        break;
    }

    unlockFunc();
}

//=============================================================================
ELL_ESVType_t ELL_HandleRecvPacket(uint8_t *recv_buf, int recv_size,
                                   void *handle, void *addr)
{
    ELL_Header_t header;
    ELL_Iterator_t it;
    ELL_ESVType_t esv_type;

    // ------------------------------------------------------- Parse Packet ---
    if (!ELL_SetPacketIterator(recv_buf, recv_size, &it)) {
        return (ESV_TYPE_NONE);
    }
    if (!ELL_GetHeader(recv_buf, recv_size, &header)) {
        return (ESV_TYPE_NONE);
    }

    esv_type = (ELL_ESVType_t)ELL_ESVType(ELL_ESV(recv_buf));
    switch (esv_type) {
    case ESV_TYPE_REQUEST:
        // ------------------------------------------ Handle Request Packet ---
        if (header.multi_deoj) {
            do {
                ELL_HandleRequestPacket(&it, &header, handle, addr);
                ELL_SetPacketIterator(recv_buf, recv_size, &it);
            } while ((header.deoj = ELL_GetNextInstance(header.deoj)) != 0);
        } else {
            ELL_HandleRequestPacket(&it, &header, handle, addr);
        }
        break;
    case ESV_TYPE_RESPONSE:
        // ----------------------------------------- Handle Response Packet ---
        ELL_HandleResponsePacket(&it, &header, handle, addr);
        break;
    case ESV_TYPE_ANNOUNCE:
        // ----------------------------------------- Handle Announce Packet ---
        ELL_HandleAnnouncePacket(&it, &header, handle, addr);
        break;
    default:
        break;
    }

    return (esv_type);
}

//=============================================================================
void ELL_CheckAnnounce(void *handle)
{
    int cnt, num_obj;
    ELL_Object_t *node, *obj;
    uint8_t pdc;
    uint8_t *edt;
    uint32_t eoj;

    int num_anno, send_size;
    uint8_t anno_list[16];

    lockFunc();

    node = ELL_FindObject(0x0ef001);
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

                multicastPacket(handle, gSendBuffer, send_size);

				if (gNotifyAnnoEPC != NULL) {
					(*gNotifyAnnoEPC)(eoj, anno_list, num_anno);
				}
            }
        }
    }

    unlockFunc();
}

/******************************** END-OF-FILE ********************************/
