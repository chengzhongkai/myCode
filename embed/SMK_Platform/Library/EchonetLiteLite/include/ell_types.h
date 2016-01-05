/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _ELL_TYPES_H
#define _ELL_TYPES_H

//=============================================================================
#ifdef __FREESCALE_MQX__
//typedef unsigned char uint8_t;
//typedef unsigned short uint16_t;
//typedef unsigned int uint32_t;
#else
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
#endif

typedef uint8_t bool_t;

//=============================================================================
#ifndef FALSE
#define FALSE (0)
#define TRUE (1)
#endif

//=============================================================================
#ifndef NULL
#define NULL ((void *)0)
#endif

//=============================================================================
typedef enum {
    ELL_STATUS_OK = 0,

    ELL_STATUS_ERROR = -1,
    ELL_STATUS_POINTER_IS_NULL = -2,
    ELL_STATUS_ILLEGAL_PARAMETER = -3,
} ELL_Status_t;

//=============================================================================
typedef enum {
    ELL_None = 0,

    ELL_Get,
    ELL_Set,
    ELL_Anno,

    ELL_Get_Res,
    ELL_Set_Res,
    ELL_Anno_Res
} ELL_Access_t;

//=============================================================================
typedef enum {
    ESV_None = 0x00,

    ESV_SetI = 0x60,
    ESV_SetC = 0x61,
    ESV_Get = 0x62,
    ESV_INF_REQ = 0x63,
    ESV_SetGet = 0x6e,

    ESV_SetC_Res = 0x71,
    ESV_Get_Res = 0x72,
    ESV_INF = 0x73,
    ESV_INFC = 0x74,
    ESV_INFC_Res = 0x7a,
    ESV_SetGet_Res = 0x7e,

    ESV_SetI_SNA = 0x50,
    ESV_SetC_SNA = 0x51,
    ESV_Get_SNA = 0x52,
    ESV_INF_SNA = 0x53,
    ESV_SetGet_SNA = 0x5e
} ELL_ESV_t;

//=============================================================================
typedef enum {
    ESV_TYPE_NONE = 0,
    ESV_TYPE_REQUEST,
    ESV_TYPE_RESPONSE,
    ESV_TYPE_ANNOUNCE
} ELL_ESVType_t;

//=============================================================================
typedef struct {
    uint16_t tid;
    uint32_t seoj;
    uint32_t deoj;
    bool_t multi_deoj;
    uint8_t esv;
} ELL_Header_t;

//=============================================================================
typedef struct {
    const uint8_t *packet;
    int len;
    uint8_t cur;
    uint8_t max;
    const uint8_t *edata;
    ELL_Access_t access;
} ELL_Iterator_t;

//=============================================================================
typedef struct {
    uint8_t epc;
    uint8_t pdc;
    const uint8_t *edt;
} ELL_Property_t;

//=============================================================================
#define EPC_FLAG_RULE_GET   0x01
#define EPC_FLAG_RULE_SET   0x02
#define EPC_FLAG_RULE_ANNO  0x04
#define EPC_FLAG_RULE_GETUP 0x08
#define EPC_FLAG_RULE_SETUP 0x10
#define EPC_FLAG_ADAPTER    0x40
#define EPC_FLAG_NEED_ANNO  0x80

//=============================================================================
typedef struct {
    uint8_t epc;
    uint8_t pdc;
    uint8_t flag;
    uint8_t range;
    uint8_t *edt;
    uint8_t *rdef;
} ELL_EPCDefine_t;

//=============================================================================
typedef struct {
    uint32_t eoj;
    uint8_t num_epc;
    ELL_EPCDefine_t *epc_list;
    void *set_cb; // ELL_SetCallback
    void *get_cb; // ELL_GetCallback
} ELL_Object_t;

//=============================================================================
typedef bool_t ELL_SetCallback(ELL_Object_t *obj,
                               uint8_t epc, uint8_t pdc, const uint8_t *edt);
typedef uint8_t ELL_GetCallback(ELL_Object_t *obj,
                                uint8_t epc, uint8_t *edt, int max);

//=============================================================================
typedef struct {
    uint8_t *packet;
    int max;
    int ptr;
    int opc_idx;
} ELL_EDATA_t;

//=============================================================================
typedef enum {
    ELL_PROP_VARIABLE = 0,
    ELL_PROP_LIST,
    ELL_PROP_NG_LIST,
    ELL_PROP_MIN_MAX,
    ELL_PROP_BINARY,
    ELL_PROP_BITMAP
} ELL_PropertyType_t;

#endif // !_ELL_TYPES_H

/******************************** END-OF-FILE ********************************/
