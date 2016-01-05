/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _ECHONET_LITE_LITE_H
#define _ECHONET_LITE_LITE_H

#include "ell_config.h"

#include "ell_depend.h"
#include "ell_macros.h"
#include "ell_types.h"
#include "connection.h"

//=============================================================================
const char *ELL_ESVString(uint8_t esv);
bool_t ELL_IsESV(uint8_t esv);
ELL_ESVType_t ELL_ESVType(uint8_t esv);
uint8_t ELL_GetESVRes(uint8_t esv);
uint8_t ELL_GetESVSNA(uint8_t esv);

//=============================================================================
bool_t ELL_GetHeader(const uint8_t *packet, int len, ELL_Header_t *header);
bool_t ELL_SetPacketIterator(const uint8_t *packet, int len,
                             ELL_Iterator_t *ell_iterator);
bool_t ELL_HasNextProperty(ELL_Iterator_t *ell_iterator);
ELL_Access_t ELL_GetNextProperty(ELL_Iterator_t *ell_iterator,
                                 ELL_Property_t *ell_property);

int ELL_MakeINFPacket(uint16_t tid, uint32_t seoj, uint32_t deoj,
                      const uint8_t *epc_list, uint8_t *packet, int len);
int ELL_MakeGetPacket(uint16_t tid, uint32_t seoj, uint32_t deoj,
                      const uint8_t *epc_list, uint8_t *packet, int len);
int ELL_MakePacket(uint16_t tid, uint32_t seoj, uint32_t deoj, uint8_t esv,
                   ELL_Property_t *prop_list, uint8_t *packet, int len);

bool_t ELL_PrepareEDATA(uint8_t *packet, int len, ELL_EDATA_t *edata);
int ELL_SetHeaderToEDATA(ELL_EDATA_t *edata, uint16_t tid,
                         uint32_t seoj, uint32_t deoj, uint8_t esv);
bool_t ELL_AddEPC(ELL_EDATA_t *edata,
                  uint8_t epc, uint8_t pdc, const uint8_t *edt);
bool_t ELL_SetNextOPC(ELL_EDATA_t *edata);

//=============================================================================
void ELL_InitObjects(void);
ELL_Object_t *ELL_RegisterObject(uint32_t eoj, const uint8_t *define);
bool_t ELL_AddRangeRule(ELL_Object_t *obj, const uint8_t *range);
bool_t ELL_AddCallback(ELL_Object_t *obj,
                       ELL_SetCallback_t *set_cb, ELL_GetCallback_t *get_cb);
ELL_Object_t *ELL_FindObject(uint32_t eoj);
uint32_t ELL_GetNextInstance(uint32_t eoj);
bool_t ELL_InitProperty(ELL_Object_t *obj,
                        uint8_t epc, uint8_t pdc, const uint8_t *edt);
bool_t ELL_SetProperty(ELL_Object_t *obj,
                       uint8_t epc, uint8_t pdc, const uint8_t *edt);
uint8_t ELL_GetProperty(ELL_Object_t *obj, uint8_t epc, uint8_t *edt, int max);
uint8_t ELL_GetPropertySize(ELL_Object_t *obj, uint8_t epc);
uint8_t ELL_GetPropertyFlag(ELL_Object_t *obj, uint8_t epc);
bool_t ELL_CheckProperty(ELL_Object_t *obj,
                         uint8_t epc, uint8_t pdc, const uint8_t *edt);
void ELL_ClearAnnounce(ELL_Object_t *obj);
void ELL_NeedAnnounce(ELL_Object_t *obj, uint8_t epc);
int ELL_GetAnnounceEPC(ELL_Object_t *obj, uint8_t *buf, int max);

uint8_t ELL_GetPropertyVirtual(ELL_Object_t *obj,
							   uint8_t epc, uint8_t *edt, int max);
uint8_t ELL_SetPropertyVirtual(ELL_Object_t *obj,
							   uint8_t epc, uint8_t pdc, const uint8_t *edt);

//=============================================================================
void ELL_StartUp(void *handle);
ELL_ESVType_t ELL_HandleRecvPacket(uint8_t *recv_buf, int recv_size,
                                   void *handle, void *addr);
void ELL_CheckAnnounce(void *handle);

typedef bool_t ELL_SendPacketCB(void *handle,
                                void *dest, const uint8_t *buf, int len);
void ELL_SetSendPacketCallback(ELL_SendPacketCB *send_packet);

typedef void ELL_RecvPropertyCB(const ELL_Header_t *header, const void *addr,
                                const ELL_Property_t *prop);
void ELL_SetRecvPropertyCallback(ELL_RecvPropertyCB *recv_prop);

typedef void ELL_LockCB(void *data);
typedef void ELL_UnlockCB(void *data);
bool_t ELL_SetLockFunc(ELL_LockCB *lock, ELL_UnlockCB *unlock, void *data);

typedef void ELL_NotifyAnnoEPCCB(uint32_t eoj, uint8_t *epc_list, int len);
void ELL_SetNotifyAnnoEPCCallback(ELL_NotifyAnnoEPCCB *callback);

//=============================================================================
void ELLDbg_PrintPacket(const char *prefix, IPv4Addr_t addr,
                        const uint8_t *buf, int len);
void ELLDbg_PrintProperty(const ELL_Property_t *prop);

#endif // !_ECHONET_LITE_LITE_H

/******************************** END-OF-FILE ********************************/
