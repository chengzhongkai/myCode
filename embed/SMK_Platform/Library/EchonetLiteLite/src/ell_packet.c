/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "echonetlitelite.h"

//=============================================================================
const char *ELL_ESVString(uint8_t esv)
{
    switch (esv) {
    case ESV_SetI:
        return ("SetI");
    case ESV_SetC:
        return ("SetC");
    case ESV_Get:
        return ("Get");
    case ESV_INF_REQ:
        return ("INF_REQ");
    case ESV_SetGet:
        return ("SetGet");

    case ESV_SetC_Res:
        return ("SetC_Res");
    case ESV_Get_Res:
        return ("Get_Res");
    case ESV_INF:
        return ("INF");
    case ESV_INFC:
        return ("INFC");
    case ESV_INFC_Res:
        return ("INFC_Res");
    case ESV_SetGet_Res:
        return ("SetGet_Res");

    case ESV_SetI_SNA:
        return ("SetI_SNA");
    case ESV_SetC_SNA:
        return ("SetC_SNA");
    case ESV_Get_SNA:
        return ("Get_SNA");
    case ESV_INF_SNA:
        return ("INF_SNA");
    case ESV_SetGet_SNA:
        return ("SetGet_SNA");

    default:
        return ("None");
    }
}

//=============================================================================
bool_t ELL_IsESV(uint8_t esv)
{
    switch (esv) {
    case ESV_SetI:
    case ESV_SetC:
    case ESV_Get:
    case ESV_INF_REQ:
    case ESV_SetGet:

    case ESV_SetC_Res:
    case ESV_Get_Res:
    case ESV_INF:
    case ESV_INFC:
    case ESV_INFC_Res:
    case ESV_SetGet_Res:

    case ESV_SetI_SNA:
    case ESV_SetC_SNA:
    case ESV_Get_SNA:
    case ESV_INF_SNA:
    case ESV_SetGet_SNA:
        return (TRUE);

    default:
        return (FALSE);
    }
}

//=============================================================================
ELL_ESVType_t ELL_ESVType(uint8_t esv)
{
    switch (esv) {
    case ESV_SetI:
    case ESV_SetC:
    case ESV_Get:
    case ESV_INF_REQ:
    case ESV_SetGet:
        return (ESV_TYPE_REQUEST);

    case ESV_SetC_Res:
    case ESV_Get_Res:
        return (ESV_TYPE_RESPONSE);
    case ESV_INF:
        return (ESV_TYPE_ANNOUNCE);
    case ESV_INFC:
        return (ESV_TYPE_ANNOUNCE);
    case ESV_INFC_Res:
    case ESV_SetGet_Res:
        return (ESV_TYPE_RESPONSE);

    case ESV_SetI_SNA:
    case ESV_SetC_SNA:
    case ESV_Get_SNA:
    case ESV_INF_SNA:
    case ESV_SetGet_SNA:
        return (ESV_TYPE_RESPONSE);

    default:
        return (ESV_TYPE_NONE);
    }
}

//=============================================================================
uint8_t ELL_GetESVRes(uint8_t esv)
{
    switch (esv) {
    case ESV_SetI:
        return (ESV_None);
    case ESV_SetC:
        return (ESV_SetC_Res);
    case ESV_Get:
        return (ESV_Get_Res);
    case ESV_INF_REQ:
        return (ESV_INF);
    case ESV_SetGet:
        return (ESV_SetGet_Res);

    case ESV_INFC:
        return (ESV_INFC_Res);

    default:
        return (ESV_None);
    }
}

//=============================================================================
uint8_t ELL_GetESVSNA(uint8_t esv)
{
    switch (esv) {
    case ESV_SetI:
        return (ESV_SetI_SNA);
    case ESV_SetC:
    case ESV_SetC_Res:
        return (ESV_SetC_SNA);
    case ESV_Get:
    case ESV_Get_Res:
        return (ESV_Get_SNA);
    case ESV_INF_REQ:
        return (ESV_INF_SNA);
    case ESV_SetGet:
    case ESV_SetGet_Res:
        return (ESV_SetGet_SNA);

    default:
        return (ESV_SetI_SNA);
    }
}

//=============================================================================
uint8_t *ELL_EDATA1(uint8_t *packet, int idx)
{
    uint8_t *edata;
    uint8_t pdc;

    ELL_Assert(packet != NULL);

    edata = &packet[12]; // first EDATA(set of EPC, PDC and EDT)
    for ( ; idx > 0; idx --) {
        pdc = edata[1];
        edata += (pdc + 2);
    }

    return (edata);
}

//=============================================================================
uint8_t ELL_OPC2(uint8_t *packet)
{
    uint8_t opc;
    uint8_t *edata;

    ELL_Assert(packet != NULL);

    opc = ELL_OPC(packet);
    edata = ELL_EDATA1(packet, opc);

    return (*edata);
}

//=============================================================================
bool_t ELL_GetHeader(const uint8_t *packet, int len, ELL_Header_t *header)
{
    ELL_Assert(packet != NULL && header != NULL);

    if (len < 14) return (FALSE);
    if (packet[0] != 0x10 || packet[1] != 0x81) return (FALSE);
    if (!ELL_IsESV(ELL_ESV(packet)) || ELL_OPC(packet) == 0) return (FALSE);

    header->tid = ELL_TID(packet);
    header->seoj = ELL_SEOJ(packet);
    header->deoj = ELL_DEOJ(packet);
    header->esv = ELL_ESV(packet);

    if ((header->deoj & 0xff) == 0) {
        header->deoj = ELL_GetNextInstance(header->deoj);
        header->multi_deoj = TRUE;
    } else {
        header->multi_deoj = FALSE;
    }

    return (TRUE);
}

//=============================================================================
bool_t ELL_SetPacketIterator(const uint8_t *packet, int len,
                             ELL_Iterator_t *ell_iterator)
{
    int opc, pdc, idx;

    ELL_Assert(packet != NULL && len != 0 && ell_iterator != NULL);

    // ------------------------------------------------ Check Packet Header ---
    // 10 81 [TID 2] [SEOJ 3] [DEOJ 3] [ESV 1] [OPC 1] [EPC 1] [PDC 1] ...
    //(more than 14 bytes)
    if (len < 14) return (FALSE);
    if (packet[0] != 0x10 || packet[1] != 0x81) return (FALSE);
    if (!ELL_IsESV(ELL_ESV(packet)) || ELL_OPC(packet) == 0) return (FALSE);

    // ------------------------------------------------- Check EDT Validity ---
    opc = ELL_OPC(packet);
    idx = 12;
    for ( ; opc > 0; opc --) {
        if (idx >= len) return (FALSE);
        // skip epc
        idx ++;
        if (idx >= len) return (FALSE);
        pdc = packet[idx];
        idx ++;
        if (pdc > 0) {
            // check edt
            idx += pdc;
        }
    }
    switch (ELL_ESV(packet)) {
    case ESV_SetGet:
    case ESV_SetGet_Res:
    case ESV_SetGet_SNA:
        opc = packet[idx];
        idx ++;
        for ( ; opc > 0; opc --) {
            if (idx >= len) return (FALSE);
            // skip epc
            idx ++;
            if (idx >= len) return (FALSE);
            pdc = packet[idx];
            idx ++;
            if (pdc > 0) {
                // check edt
                idx += pdc;
            }
        }
        break;
    default:
        break;
    }
    if (idx != len) return (FALSE);

    // ------------------------------------------------------- Set Iterator ---
    ell_iterator->packet = packet;
    ell_iterator->len = len;
    ell_iterator->cur = 0;
    ell_iterator->max = ELL_OPC(packet);
    ell_iterator->edata = &packet[12];

    switch (ELL_ESV(packet)) {
    case ESV_SetI:
    case ESV_SetC:
        ell_iterator->access = ELL_Set;
        break;
    case ESV_SetC_Res:
    case ESV_SetI_SNA:
    case ESV_SetC_SNA:
        ell_iterator->access = ELL_Set_Res;
        break;

    case ESV_Get:
        ell_iterator->access = ELL_Get;
        break;
    case ESV_Get_Res:
    case ESV_Get_SNA:
        ell_iterator->access = ELL_Get_Res;
        break;

    case ESV_INF_REQ:
        ell_iterator->access = ELL_Anno;
        break;
    case ESV_INF_SNA:
        ell_iterator->access = ELL_Anno;
        break;

    case ESV_INF:
    case ESV_INFC:
        ell_iterator->access = ELL_Anno;
        break;
    case ESV_INFC_Res:
        ell_iterator->access = ELL_Anno_Res;
        break;

    case ESV_SetGet:
        ell_iterator->access = ELL_Set;
        break;
    case ESV_SetGet_Res:
    case ESV_SetGet_SNA:
        ell_iterator->access = ELL_Set;
        break;
    default:
        ell_iterator->access = ELL_None;
        break;
    }

    return (TRUE);
}

//=============================================================================
bool_t ELL_HasNextProperty(ELL_Iterator_t *ell_iterator)
{
    ELL_Assert (ell_iterator != NULL);

    if (ell_iterator->edata != NULL) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

//=============================================================================
ELL_Access_t ELL_GetNextProperty(ELL_Iterator_t *ell_iterator,
                                 ELL_Property_t *ell_property)
{
    const uint8_t *edata;
    ELL_Access_t ret;

    ELL_Assert(ell_iterator != NULL && ell_property != NULL);

    if (ell_iterator->edata == NULL) return (ELL_None);

    edata = ell_iterator->edata;
    ell_iterator->edata += (ell_iterator->edata[1] + 2);

    ret = ell_iterator->access;

    ell_property->epc = edata[0];
    ell_property->pdc = edata[1];
    if (ell_property->pdc == 0) {
        ell_property->edt = NULL;
    } else {
        ell_property->edt = &edata[2];
    }

    ell_iterator->cur ++;
    switch (ELL_ESV(ell_iterator->packet)) {
    case ESV_SetGet:
    case ESV_SetGet_Res:
    case ESV_SetGet_SNA:
        if (ell_iterator->cur >= ell_iterator->max) {
            if (ell_iterator->access == ELL_Set) {
                ell_iterator->max = *ell_iterator->edata;
                ell_iterator->edata ++;
                ell_iterator->cur = 0;
                ell_iterator->access = ELL_Get;
            } else {
                ell_iterator->edata = NULL; // FINISHED
            }
        }
        break;
    default:
        if (ell_iterator->cur >= ell_iterator->max) {
            ell_iterator->edata = NULL; // FINISHED
        }
        break;
    }

    return (ret);
}

//=============================================================================
static uint8_t ELL_numEPC(const uint8_t *epc_list)
{
    int cnt;

    ELL_Assert(epc_list != NULL);

    for (cnt = 0; cnt < 128; cnt ++) {
        if (*epc_list < 128) {
            return (cnt);
        }
        epc_list ++;
    }
    return (0);
}

//=============================================================================
static uint8_t ELL_numEPC2(ELL_Property_t *prop_list)
{
    int cnt;

    ELL_Assert(prop_list != NULL);

    for (cnt = 0; cnt < 128; cnt ++) {
        if (prop_list->epc < 128) {
            return (cnt);
        }
        prop_list ++;
    }
    return (0);
}

//=============================================================================
static void ELL_setHeader(uint16_t tid, uint32_t seoj, uint32_t deoj,
                          uint8_t esv, uint8_t *packet)
{
    packet[0] = 0x10;
    packet[1] = 0x81;
    packet[2] = (uint8_t)(tid >> 8);
    packet[3] = (uint8_t)(tid & 0xff);
    packet[4] = (uint8_t)((seoj >> 16) & 0xff);
    packet[5] = (uint8_t)((seoj >> 8) & 0xff);
    packet[6] = (uint8_t)(seoj & 0xff);
    packet[7] = (uint8_t)((deoj >> 16) & 0xff);
    packet[8] = (uint8_t)((deoj >> 8) & 0xff);
    packet[9] = (uint8_t)(deoj & 0xff);
    packet[10] = esv;
    // packet[11] = 0;
}

//=============================================================================
int ELL_MakeINFPacket(uint16_t tid, uint32_t seoj, uint32_t deoj,
                      const uint8_t *epc_list, uint8_t *packet, int len)
{
    uint8_t opc;
    int cnt, ptr;

    uint8_t epc, pdc;
    ELL_Object_t *obj;

    if (epc_list == NULL || packet == NULL) return (0);
    if (len < 14) return (0);

    // ------------------------------------------- Set TID, SEOJ, DEOJ, ESV ---
    ELL_setHeader(tid, seoj, deoj, ESV_INF, packet);

    // --------------------------------------- Set OPC & EPC, PDC, EDT List ---
    opc = ELL_numEPC(epc_list);
    if (opc == 0) return (0);

    packet[ELL_IDX_OPC] = opc;

    obj = ELL_FindObject(seoj);
    if (obj == NULL) return (0);

    ptr = ELL_IDX_FIRST_EPC;
    for (cnt = 0; cnt < opc; cnt ++) {
        epc = *epc_list ++;
        packet[ptr ++] = epc;
        pdc = ELL_GetProperty(obj, epc, &packet[ptr + 1], len - ptr);
        if (pdc == 0) return (0);
        packet[ptr] = pdc;
        ptr += (pdc + 1);
    }

    return (ptr);
}

//=============================================================================
int ELL_MakeGetPacket(uint16_t tid, uint32_t seoj, uint32_t deoj,
                      const uint8_t *epc_list, uint8_t *packet, int len)
{
    uint8_t opc;
    int cnt, ptr;

    uint8_t epc;

    if (epc_list == NULL || packet == NULL) return (0);
    if (len < 14) return (0);

    // ------------------------------------------- Set TID, SEOJ, DEOJ, ESV ---
    ELL_setHeader(tid, seoj, deoj, ESV_Get, packet);

    // --------------------------------------- Set OPC & EPC, PDC, EDT List ---
    opc = ELL_numEPC(epc_list);
    if (opc == 0) return (0);

    packet[ELL_IDX_OPC] = opc;

    ptr = ELL_IDX_FIRST_EPC;
    for (cnt = 0; cnt < opc; cnt ++) {
        epc = *epc_list ++;
        packet[ptr ++] = epc;
        packet[ptr ++] = 0;
    }

    return (ptr);
}

//=============================================================================
int ELL_MakePacket(uint16_t tid, uint32_t seoj, uint32_t deoj, uint8_t esv,
                   ELL_Property_t *prop_list, uint8_t *packet, int len)
{
    uint8_t opc;
    int cnt, ptr;

    if (prop_list == NULL || packet == NULL) return (0);
    if (len < 14) return (0);

    // ------------------------------------------- Set TID, SEOJ, DEOJ, ESV ---
    ELL_setHeader(tid, seoj, deoj, esv, packet);

    // --------------------------------------- Set OPC & EPC, PDC, EDT List ---
    opc = ELL_numEPC2(prop_list);
    if (opc == 0) return (0);

    packet[ELL_IDX_OPC] = opc;

    ptr = ELL_IDX_FIRST_EPC;
    for (cnt = 0; cnt < opc; cnt ++) {
        packet[ptr ++] = prop_list->epc;
        packet[ptr ++] = prop_list->pdc;
        if (prop_list->pdc > 0) {
            MEM_Copy(&packet[ptr], prop_list->edt, prop_list->pdc);
            ptr += prop_list->pdc;
        }
        prop_list ++;
    }

    return (ptr);
}

//=============================================================================
bool_t ELL_PrepareEDATA(uint8_t *packet, int len, ELL_EDATA_t *edata)
{
    if (packet == NULL || edata == NULL) return (FALSE);
    if (len < 14) return (FALSE);

    edata->packet = packet;
    edata->max = len;
    edata->packet[ELL_IDX_OPC] = 0;
    edata->ptr = ELL_IDX_FIRST_EPC;
    edata->opc_idx = ELL_IDX_OPC;

    return (TRUE);
}

//=============================================================================
int ELL_SetHeaderToEDATA(ELL_EDATA_t *edata, uint16_t tid,
                         uint32_t seoj, uint32_t deoj, uint8_t esv)
{
    if (edata == NULL) return (0);

    ELL_setHeader(tid, seoj, deoj, esv, edata->packet);

    return (edata->ptr);
}

//=============================================================================
bool_t ELL_AddEPC(ELL_EDATA_t *edata,
                  uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    int cnt;

    if (edata == NULL) return (FALSE);

    if (edata->packet[edata->opc_idx] == 0xff) return (FALSE);
    if (pdc != 0 && edt == NULL) return (FALSE);

    if (edata->ptr >= edata->max) {
        return (FALSE);
    }
    edata->packet[edata->ptr ++] = epc;

    if (edata->ptr >= edata->max) {
        return (FALSE);
    }
    edata->packet[edata->ptr ++] = pdc;

    for (cnt = 0; cnt < pdc; cnt ++) {
        if (edata->ptr >= edata->max) {
            return (FALSE);
        }
        edata->packet[edata->ptr ++] = *edt ++;
    }
    edata->packet[edata->opc_idx] ++;

    return (TRUE);
}

//=============================================================================
bool_t ELL_SetNextOPC(ELL_EDATA_t *edata)
{
    if (edata == NULL) return (FALSE);

    edata->opc_idx = edata->ptr;
    edata->packet[edata->opc_idx] = 0;
    edata->ptr ++;

    return (TRUE);
}

/******************************** END-OF-FILE ********************************/
