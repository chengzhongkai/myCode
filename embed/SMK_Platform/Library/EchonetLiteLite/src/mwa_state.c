/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

// Customize for Sankyo-Tateyama
#define FOR_SANKYO_TATEYAMA

#ifdef FOR_SANKYO_TATEYAMA
#include "ipcfg.h"
#endif

//=============================================================================
#ifdef __FREESCALE_MQX__
extern void LED_SetMode(uint8_t led, uint8_t mode);
#else
#define LED_SetMode(a, b)
#endif

extern MWA_EventQueue_t gMWA_FrameCommEventQ;

MWA_EventQueue_t gMWA_StateMachineEventQ;

MWA_TimerHandle_t gMWA_SMTimer;

//=============================================================================
// Adapter State
//=============================================================================
typedef enum {
    MWA_ADP_STATE_NONE = 0,

    MWA_ADP_STATE_UNRECOGNIZED_C0,
    MWA_ADP_STATE_UNRECOGNIZED_C1,

    MWA_ADP_STATE_UNIDENTIFIED_C0,
    MWA_ADP_STATE_UNIDENTIFIED_C1,

    MWA_ADP_STATE_WAIT,

    MWA_ADP_STATE_INIT_ADAPTER_C0,
    MWA_ADP_STATE_INIT_ADAPTER_C1,
    MWA_ADP_STATE_INIT_ADAPTER_C2,

    MWA_ADP_STATE_CONSTRUCT_OBJ_C0,
    MWA_ADP_STATE_CONSTRUCT_OBJ_C1,
    MWA_ADP_STATE_CONSTRUCT_OBJ_C2,
    MWA_ADP_STATE_CONSTRUCT_OBJ_C3,

    MWA_ADP_STATE_START_ECHONET,

    MWA_ADP_STATE_NORMAL_OPERATION_C0,
    MWA_ADP_STATE_NORMAL_OPERATION_C1,

    MWA_ADP_STATE_ERR_CONNECTION,
    MWA_ADP_STATE_ERR_STOP,

    MWA_ADP_STATE_MAX
} MWA_AdpState_t;

static MWA_AdpState_t gAdpState = MWA_ADP_STATE_NONE;

//=============================================================================
// Ready Device Information
//=============================================================================
typedef struct {
    uint8_t type;
    uint8_t speed;

    uint8_t recognized_result;

    uint16_t init_adapter;
    uint8_t init_adapter_fn;
    uint16_t confirm_adapter;

    uint16_t construct_obj;
    uint16_t confirm_obj;

    uint16_t start_adapter;
    uint16_t start_adapter_res;
} MWA_ReadyDevice_t;

static MWA_ReadyDevice_t gReadyDevice;

//=============================================================================
// Definitions for Recognizing and Identifying I/F
//=============================================================================
static const uint8_t gUARTBaudrateList[3] = {
    MWA_BAUDRATE_2400,
    MWA_BAUDRATE_9600,
    MWA_BAUDRATE_MAX
};
static int gUARTBaudrateTryIdx;
static const int gUARTBaudrateTable[] = {
    2400, 4800, 9600, 19200, 38400, 57600, 115200
};

#define MWA_RECOGNIZE_IF_RETRY_MAX (60)
static int gRecognizeIFRetryCnt;

static int gStateRetryCnt;

//=============================================================================
// Definitions for Object Construction
//=============================================================================
typedef struct {
    uint8_t epc;
    uint8_t flag;
    uint8_t size;
} MWA_Property_t;

typedef struct {
    uint8_t idx;
    uint32_t eoj;
    uint8_t version[4];
    uint8_t maker_code[3];
    uint8_t location[3];
    uint8_t product_code[12];
    uint8_t serial_number[12];
    uint8_t product_date[4];

    uint8_t num_props;
    MWA_Property_t *props;
} MWA_ObjectInfo_t;

#define MWA_NUM_OBJECT_INFO (3)
static MWA_ObjectInfo_t gAdapterObjectInfo[MWA_NUM_OBJECT_INFO];
static int gNumAdapterObjectInfo;
static bool_t gAdpObjLoaded[MWA_NUM_OBJECT_INFO];

//=============================================================================
// Definitions for Start ECHONET (Initialize Properties)
//=============================================================================
typedef struct {
    uint32_t eoj;
    uint8_t epc;
    uint8_t size;
} MWA_InitProperty_t;

static int gInitPropertyNum;
static int gInitPropertyPtr;
static MWA_InitProperty_t *gInitPropertyList = NULL;

//=============================================================================
static const uint8_t *gMWANodeProfile = NULL;
static int gMWANodeProfileLen = 0;

//=============================================================================
// Work for Request from Ready Device
//=============================================================================
#define MWA_EDT_WORK_SIZE (256)
static uint8_t gMWAEDTWork[MWA_EDT_WORK_SIZE];


//=============================================================================
// Static Functions
//=============================================================================
static void MWA_EnterAdapterState(MWA_AdpState_t next_state);


//*****************************************************************************
// 
//*****************************************************************************

//=============================================================================
static void MWA_ExpandPropMap(uint8_t *master_map,
                              uint8_t prop_map[17], uint8_t flag)
{
    uint8_t map;
    uint8_t mask;
    int cnt, cnt2;

    for (cnt = 0; cnt < 16; cnt ++) {
        map = prop_map[cnt + 1];
        mask = 0x01;
        for (cnt2 = 0; cnt2 < 8; cnt2 ++) {
            if ((map & mask) != 0) {
                master_map[cnt2 * 0x10 + cnt] |= flag;
            }
            mask <<= 1;
        }
    }
}

//=============================================================================
static bool_t MWA_GetObjectInfo(MWA_Frame_t *frame, MWA_ObjectInfo_t *obj_info)
{
    uint8_t eoj[3];
    uint16_t len;
    uint16_t param_bmp;
    uint8_t prop_map[17];
    uint8_t *prop_work = NULL;
    bool_t ret = FALSE;
    int cnt, prop_idx;
    int prop_size_map_len;
    // int total_props;

    do {
        if (!MWA_GetByte(frame, &obj_info->idx)) break;

        ELL_Printf("Object Index: 0x%02X\n", obj_info->idx);

        if (!MWA_GetByte(frame, &eoj[0])) break;
        if (!MWA_GetByte(frame, &eoj[1])) break;
        if (!MWA_GetByte(frame, &eoj[2])) break;

        obj_info->eoj = ((uint32_t)eoj[0] << 16) | ((uint32_t)eoj[1] << 8)
            | (uint32_t)eoj[2];

        ELL_Printf("EOJ: 0x%06X\n", obj_info->eoj);

        if (!MWA_GetShort(frame, &len)) break;

        ELL_Printf("Length: %d\n", len);
        prop_size_map_len = len - 193;

        if (!MWA_GetShort(frame, &param_bmp)) break;

        ELL_Printf("Param Bitmap: 0x%02X\n", param_bmp);

        prop_work = MEM_Alloc(128);
        if (prop_work == NULL) break;

        MEM_Set(prop_work, 0, 128);

        MWA_GetByteArray(frame, prop_map, 17); // SetM Property Map
        MWA_GetByteArray(frame, prop_map, 17); // Set Property Map
        MWA_ExpandPropMap(prop_work, prop_map, EPC_FLAG_RULE_SET);
        MWA_GetByteArray(frame, prop_map, 17); // GetM Property Map
        MWA_GetByteArray(frame, prop_map, 17); // Get Property Map
        MWA_ExpandPropMap(prop_work, prop_map, EPC_FLAG_RULE_GET);
        MWA_GetByteArray(frame, prop_map, 17); // Anno Property Map
        MWA_ExpandPropMap(prop_work, prop_map, EPC_FLAG_RULE_ANNO);
        MWA_GetByteArray(frame, prop_map, 17); // IASetup Property Map
        MWA_ExpandPropMap(prop_work, prop_map, EPC_FLAG_RULE_SETUP);
        MWA_GetByteArray(frame, prop_map, 17); // IAGetup Property Map
        MWA_ExpandPropMap(prop_work, prop_map, EPC_FLAG_RULE_GETUP);
        MWA_GetByteArray(frame, prop_map, 17); // IASetMup Property Map
        MWA_GetByteArray(frame, prop_map, 17); // IAGetMup Property Map

#if 0
        total_props = 0;
        ELL_Printf("Property Map:\n");
        for (cnt = 0; cnt < 128; cnt ++) {
            if (prop_work[cnt] != 0) {
                ELL_Printf("  0x%02X: 0x%02X\n", cnt + 128, prop_work[cnt]);
                total_props ++;
            }
        }
        ELL_Printf("...%d property(s)\n", total_props);
#endif

        MWA_GetByteArray(frame, obj_info->version, 4);
        MWA_GetByteArray(frame, obj_info->maker_code, 3);
        MWA_GetByteArray(frame, obj_info->location, 3);
        MWA_GetByteArray(frame, obj_info->product_code, 12);
        MWA_GetByteArray(frame, obj_info->serial_number, 12);
        MWA_GetByteArray(frame, obj_info->product_date, 4);

        // ----------------------------------------------- Count Properties ---
        obj_info->num_props = 0;
        for (cnt = 0; cnt < 128; cnt ++) {
            if (prop_work[cnt] != 0) {
                obj_info->num_props ++;
            }
        }

        // ------------------------------------------- Allocate Work Memory ---
        obj_info->props = MEM_Alloc(sizeof(MWA_Property_t)
                                    * obj_info->num_props);
        if (obj_info->props == NULL) {
            break;
        }

        // ---------------------------------------------- Set Property Info ---
        prop_idx = 0;
        for (cnt = 0; cnt < 128; cnt ++) {
            if (prop_work[cnt] != 0) {
                obj_info->props[prop_idx].epc = 0x80 + cnt;

                ELL_Printf("EPC 0x%02X: ", obj_info->props[prop_idx].epc);
                if ((prop_work[cnt] & EPC_FLAG_RULE_SET) != 0) {
                    if ((prop_work[cnt] & EPC_FLAG_RULE_SETUP) != 0) {
                        ELL_Printf("IASetup, ");
                    } else {
                        ELL_Printf("IASet, ");
                    }
                }
                if ((prop_work[cnt] & EPC_FLAG_RULE_GET) != 0) {
                    if ((prop_work[cnt] & EPC_FLAG_RULE_GETUP) != 0) {
                        ELL_Printf("IAGetup, ");
                    } else {
                        ELL_Printf("IAGet, ");
                    }
                }
                if ((prop_work[cnt] & EPC_FLAG_RULE_ANNO) != 0) {
                    ELL_Printf("Anno, ");
                }

                if ((prop_work[cnt] & EPC_FLAG_RULE_SETUP) != 0
                    && (prop_work[cnt] & EPC_FLAG_RULE_SET) == 0) {
                    ELL_Printf("*** Setup but not Set ***, ");
                    prop_work[cnt] |= EPC_FLAG_RULE_SET;
                }
                if ((prop_work[cnt] & EPC_FLAG_RULE_GETUP) != 0
                    && (prop_work[cnt] & EPC_FLAG_RULE_GET) == 0) {
                    ELL_Printf("*** Getup but not Get ***, ");
                    prop_work[cnt] |= EPC_FLAG_RULE_GET;
                }

                obj_info->props[prop_idx].flag = prop_work[cnt];

#if 0
                if (!MWA_GetByte(frame, &obj_info->props[prop_idx].size)) {
                    break;
                }
#else
                // This Property Size Map is Doubtful...
                // obj_info->props[prop_idx].size = 20;
                obj_info->props[prop_idx].size = 0;
#endif

                ELL_Printf("\n");
                // ELL_Printf("Size=%d\n", obj_info->props[prop_idx].size);
                prop_idx ++;
            }
        }
        if (cnt != 128) break;

        /*** Reuse Work ***/
        if (prop_size_map_len > 0 && prop_size_map_len < 128) {
            MWA_GetByteArray(frame, prop_work, prop_size_map_len);

            if (obj_info->num_props == prop_size_map_len) {
                for (cnt = 0; cnt < obj_info->num_props; cnt ++) {
                    obj_info->props[cnt].size = prop_work[cnt];
                }
            } else {
                ELL_Printf("** Property Size Map Mismatch (props=%d, map=%d)\n",
                           obj_info->num_props, prop_size_map_len);
            }
        } else {
            ELL_Printf("** Property Size Map Error (%d)\n", prop_size_map_len);
            break;
        }

        ret = TRUE;
    } while (0);

    if (prop_work != NULL) {
        MEM_Free(prop_work);
    }

    return (ret);
}

//=============================================================================
// Need to be Freed
static uint8_t *MWA_CreateNodeProfileEPCDefine(void)
{
    uint8_t *instance_list;
    int instance_list_size;
    uint8_t *class_list;
    int class_list_size;
    int cnt, cnt2;
    MWA_ObjectInfo_t *obj_info;
    uint8_t class_id0, class_id1;
    uint16_t num_classes;
    uint8_t *define;
    uint8_t *ptr;

    // ----------------------------------------------- Create Instance List ---
    instance_list_size = gNumAdapterObjectInfo * 3 + 1;
    instance_list = MEM_Alloc(instance_list_size);
    if (instance_list == NULL) {
        return (NULL);
    }
    for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
        obj_info = &gAdapterObjectInfo[cnt];
        instance_list[cnt * 3 + 1] = (obj_info->eoj >> 16) & 0xff;
        instance_list[cnt * 3 + 2] = (obj_info->eoj >> 8) & 0xff;
        instance_list[cnt * 3 + 3] = (obj_info->eoj) & 0xff;
    }
    instance_list[0] = gNumAdapterObjectInfo;

    // -------------------------------------------------- Create Class List ---
    class_list = MEM_Alloc(gNumAdapterObjectInfo * 2 + 1);
    if (class_list == NULL) {
        MEM_Free(instance_list);
        return (NULL);
    }
    num_classes = 0;
    for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
        obj_info = &gAdapterObjectInfo[cnt];
        class_id0 = (obj_info->eoj >> 16) & 0xff;
        class_id1 = (obj_info->eoj >> 8) & 0xff;
        for (cnt2 = 0; cnt2 < num_classes; cnt2 ++) {
            if (class_list[cnt2 * 2 + 1] == class_id0
                && class_list[cnt2 * 2 + 2] == class_id1) {
                break;
            }
        }
        if (cnt2 == num_classes) {
            class_list[num_classes * 2 + 1] = class_id0;
            class_list[num_classes * 2 + 2] = class_id1;
            num_classes ++;
        }
    }
    class_list[0] = num_classes;
    class_list_size = num_classes * 2 + 1;

    define = MEM_Alloc(gMWANodeProfileLen + (3 * 5)
                       + 3 + 2 // 0xD3, 0xD4
                       + instance_list_size * 2 // 0xD5, 0xD6
                       + class_list_size); // 0xD7
    if (define != NULL) {
        MEM_Copy(define, gMWANodeProfile, gMWANodeProfileLen);
        ptr = define + (gMWANodeProfileLen - 1);

        *ptr ++ = 0xd3;
        *ptr ++ = 3;
        *ptr ++ = (gNumAdapterObjectInfo >> 16) & 0xff;
        *ptr ++ = (gNumAdapterObjectInfo >> 8) & 0xff;
        *ptr ++ = (gNumAdapterObjectInfo) & 0xff;
        *ptr ++ = EPC_FLAG_RULE_GET;

        *ptr ++ = 0xd4;
        *ptr ++ = 2;
        *ptr ++ = ((num_classes + 1) >> 8) & 0xff;
        *ptr ++ = ((num_classes + 1)) & 0xff;
        *ptr ++ = EPC_FLAG_RULE_GET;

        *ptr ++ = 0xd5;
        *ptr ++ = instance_list_size;
        MEM_Copy(ptr, instance_list, instance_list_size);
        ptr += instance_list_size;
        *ptr ++ = EPC_FLAG_RULE_ANNO;

        *ptr ++ = 0xd6;
        *ptr ++ = instance_list_size;
        MEM_Copy(ptr, instance_list, instance_list_size);
        ptr += instance_list_size;
        *ptr ++ = EPC_FLAG_RULE_GET;

        *ptr ++ = 0xd7;
        *ptr ++ = class_list_size;
        MEM_Copy(ptr, class_list, class_list_size);
        ptr += class_list_size;
        *ptr ++ = EPC_FLAG_RULE_GET;

        *ptr = 0;
    }

    MEM_Free(instance_list);
    MEM_Free(class_list);

    return (define);
}

//=============================================================================
// Timer Callback for State Machine
//=============================================================================
static void MWA_SMTimerCallback(void *data, uint32_t tag, bool_t cancel)
{
    MWA_Event_t *event;
    MWA_TimeroutPayload_t *payload;

    if (!cancel) {
        // --------------------------------------------- Send Timeout Event ---
        event = MWA_AllocEvent(MWA_EVENT_TIMEOUT,
                               sizeof(MWA_TimeroutPayload_t));
        if (event == NULL) return;

        payload = (MWA_TimeroutPayload_t *)(event + 1);
        payload->tag = tag;

        if (!MWA_AddEventToQueue(&gMWA_StateMachineEventQ, event)) {
            MWA_FreeEvent(event);
        }
    }
}

//=============================================================================
// Reset Adapter
//=============================================================================
static void MWA_ResetAdapter(void)
{
    MEM_Set(&gReadyDevice, 0xff, sizeof(gReadyDevice));

    MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
}

//=============================================================================
// Initialize Adapter
//=============================================================================
bool_t MWA_InitAdapter(const uint8_t *node_profile_def, int def_len,
					   UDP_Handle_t *udp, MWA_UARTHandle_t *uart)
{
    int cnt;

	if (node_profile_def == NULL || def_len <= 0) return (FALSE);
	if (node_profile_def[def_len - 1] != 0) return (FALSE);

	gMWANodeProfile = node_profile_def;
	gMWANodeProfileLen = def_len;

    gNumAdapterObjectInfo = 0;
    for (cnt = 0; cnt < MWA_NUM_OBJECT_INFO; cnt ++) {
        MEM_Set(&gAdapterObjectInfo[cnt], 0, sizeof(MWA_ObjectInfo_t));
        gAdpObjLoaded[cnt] = FALSE;
    }

    MWA_InitEventQueue(&gMWA_StateMachineEventQ);

    MWA_InitTimer(&gMWA_SMTimer, MWA_SMTimerCallback, NULL);

    MWA_InitFrameComm(uart);

    MWA_InitEchonet(udp);

    MWA_ResetAdapter();

    return (TRUE);
}

//=============================================================================
static bool_t MWA_SetSMTimeoutTimer(uint32_t tag, int timeout)
{
    // ------------------------------------------------ Start Timeout Timer ---
    if (timeout > 0) {
        MWA_StartTimer(&gMWA_SMTimer, timeout, tag);
    }

    return (TRUE);
}

//=============================================================================
// Send Event to Frame Communication
//=============================================================================
static bool_t MWA_RequestSendFrame(uint16_t frame_type, uint8_t cmd_no,
                                   uint8_t frame_no,
                                   uint8_t *data, int len)
{
    MWA_Event_t *event;
    MWA_SendFramePayload_t *payload;

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_SEND_FRAME,
                           sizeof(MWA_SendFramePayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_SendFramePayload_t *)(event + 1);

    // --------------------------------------------------------- Make Frame ---
    if (!MWA_PrepareFrame(&payload->send_frame,
                          frame_type, cmd_no, frame_no, len)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }
    if (data != NULL && len > 0) {
        MWA_SetFrameData(&payload->send_frame, data, len);
    }
    MWA_SetFrameFCC(&payload->send_frame);

    // ---------------------------------- Send Event to Frame Communication ---
    if (!MWA_AddEventToQueue(&gMWA_FrameCommEventQ, event)) {
        MWA_FreeFrame(&payload->send_frame);
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Send Recognize I/F Request (FT=0xFFFF, CN=0x00)
//=============================================================================
static bool_t MWA_SendRecognizeIF(void)
{
    if (!MWA_RequestSendFrame(0xffff, 0x00, 0, NULL, 0)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0xffff0000, 1000); // more than 300ms

    return (TRUE);
}

//=============================================================================
// Send Confirm I/F Request (FT=0xFFFF, CN=0x01)
//=============================================================================
static bool_t MWA_SendConfirmIF(uint8_t result)
{
    if (!MWA_RequestSendFrame(0xffff, 0x01, 0, &result, 1)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0xffff0001, 1000); // more than 300ms

    return (TRUE);
}

//=============================================================================
// Send Identify I/F Request (FT=0x0000, CN=0x00)
//=============================================================================
static bool_t MWA_SendIdentifyIF(uint8_t mwa_type, uint8_t speed)
{
    uint8_t payload[2];

    payload[0] = mwa_type;
    payload[1] = speed;

    if (!MWA_RequestSendFrame(0x0000, 0x00, 0, payload, 2)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0x00000000, 5000);

    return (TRUE);
}

//=============================================================================
// Send Init Adapter Response (FT=0x0001, CN=0x81)
//=============================================================================
static bool_t MWA_SendInitAdapterRes(uint16_t res, uint8_t frame_no)
{
    uint8_t payload[11];

    payload[0] = (uint8_t)((res >> 8) & 0xff);
    payload[1] = (uint8_t)(res & 0xff);
    payload[2] = 0xfe;
    MEM_Set(&payload[3], 0x00, 8);

    if (!MWA_RequestSendFrame(0x0001, 0x81, frame_no, payload, 11)) {
        /*** Failed ***/
    }

    return (TRUE);
}

//=============================================================================
// Send Init Adapter Done (FT=0x0001, CN=0x02)
//=============================================================================
static bool_t MWA_SendInitAdapterDone(uint16_t status)
{
    uint8_t payload[2];

    payload[0] = (uint8_t)((status >> 8) & 0xff);
    payload[1] = (uint8_t)(status & 0xff);

    if (!MWA_RequestSendFrame(0x0001, 0x02, 0, payload, 2)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0x00010002, 3000);

    return (TRUE);
}

//=============================================================================
// Send Query Object Request (FT=0x0002, CN=0x00)
//=============================================================================
static bool_t MWA_SendQueryObject(void)
{
    if (!MWA_RequestSendFrame(0x0002, 0x00, 0, NULL, 0)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0x00020000, 3000);

    return (TRUE);
}

//=============================================================================
// Send Construct Object Done (FT=0x0002, CN=0x01)
//=============================================================================
static bool_t MWA_SendConstructObjectDone(uint16_t status)
{
    uint8_t payload[2];

    payload[0] = (uint8_t)((status >> 8) & 0xff);
    payload[1] = (uint8_t)(status & 0xff);

    if (!MWA_RequestSendFrame(0x0002, 0x01, 0, payload, 2)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0x00020001, 3000);

    return (TRUE);
}

//=============================================================================
// Send Construct Object Done (FT=0x0002, CN=0x02)
//=============================================================================
static bool_t MWA_SendStartAdapterDone(uint16_t status)
{
    uint8_t payload[2];

    payload[0] = (uint8_t)((status >> 8) & 0xff);
    payload[1] = (uint8_t)(status & 0xff);

    if (!MWA_RequestSendFrame(0x0002, 0x02, 0, payload, 2)) {
        /*** Failed ***/
    }

    MWA_SetSMTimeoutTimer(0x00020002, 3000);

    return (TRUE);
}

//=============================================================================
// Send Access Object Request (FT=0x0003, CN=0x10)
//=============================================================================
static bool_t MWA_SendAccessObject(uint32_t eoj, uint8_t epc,
                                   uint8_t pdc, const uint8_t *edt)
{
    MWA_Event_t *event;
    MWA_SendFramePayload_t *payload;
    uint16_t len;
    uint8_t *data;

    if (edt == NULL && pdc != 0) return (FALSE);

    ELL_Printf("Access Object: EOJ(0x%06X), EPC(0x%02X)\n", eoj, epc);

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_SEND_FRAME,
                           sizeof(MWA_SendFramePayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_SendFramePayload_t *)(event + 1);

    // --------------------------------------------------------- Make Frame ---
    len = (uint16_t)pdc + 1;
    if (!MWA_PrepareFrame(&payload->send_frame, 0x0003, 0x10, 0, len + 5)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }

    data = (uint8_t *)payload->send_frame.data;
    data[0] = (uint8_t)((eoj >> 16) & 0xff);
    data[1] = (uint8_t)((eoj >> 8) & 0xff);
    data[2] = (uint8_t)(eoj & 0xff);
    data[3] = (uint8_t)((len >> 8) & 0xff);
    data[4] = (uint8_t)(len & 0xff);
    data[5] = epc;
    if (edt != NULL) {
        MEM_Copy(&data[6], edt, pdc);
    }
    MWA_SetFrameFCC(&payload->send_frame);

    // ---------------------------------- Send Event to Frame Communication ---
    if (!MWA_AddEventToQueue(&gMWA_FrameCommEventQ, event)) {
        MWA_FreeFrame(&payload->send_frame);
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Send Notify Object Done (FT=0x0003, CN=0x91)
//=============================================================================
static bool_t MWA_SendNotifyObjectDone(uint16_t status, uint8_t frame_no,
                                       uint32_t eoj)
{
    uint8_t payload[5];

    payload[0] = (uint8_t)((status >> 8) & 0xff);
    payload[1] = (uint8_t)(status & 0xff);
    payload[2] = (uint8_t)((eoj >> 16) & 0xff);
    payload[3] = (uint8_t)((eoj >> 8) & 0xff);
    payload[4] = (uint8_t)(eoj & 0xff);

    return (MWA_RequestSendFrame(0x0003, 0x91, frame_no, payload, 5));
}

//=============================================================================
// Send Access Ready Request (FT=0x0003, CN=0x94)
//=============================================================================
static bool_t MWA_SendAccessReadyRep(uint16_t status, uint8_t frame_no,
                                     uint32_t eoj,
                                     uint8_t epc, uint8_t pdc,
                                     const uint8_t *edt)
{
    MWA_Event_t *event;
    MWA_SendFramePayload_t *payload;
    uint16_t len;
    uint8_t *data;

    if (edt == NULL && pdc != 0) return (FALSE);

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_SEND_FRAME,
                           sizeof(MWA_SendFramePayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_SendFramePayload_t *)(event + 1);

    // --------------------------------------------------------- Make Frame ---
    len = (uint16_t)pdc + 1;
    if (!MWA_PrepareFrame(&payload->send_frame,
                          0x0003, 0x94, frame_no, len + 7)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }

    data = (uint8_t *)payload->send_frame.data;
    data[0] = (uint8_t)((status >> 8) & 0xff);
    data[1] = (uint8_t)(status & 0xff);
    data[2] = (uint8_t)((eoj >> 16) & 0xff);
    data[3] = (uint8_t)((eoj >> 8) & 0xff);
    data[4] = (uint8_t)(eoj & 0xff);
    data[5] = (uint8_t)((len >> 8) & 0xff);
    data[6] = (uint8_t)(len & 0xff);
    data[7] = epc;
    if (edt != NULL) {
        MEM_Copy(&data[8], edt, pdc);
    }
    MWA_SetFrameFCC(&payload->send_frame);

    // ---------------------------------- Send Event to Frame Communication ---
    if (!MWA_AddEventToQueue(&gMWA_FrameCommEventQ, event)) {
        MWA_FreeFrame(&payload->send_frame);
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Call from ECHONET Lite Communication
//=============================================================================
bool_t MWA_RequestENLAccess(uint32_t eoj, uint8_t epc,
                            uint8_t pdc, const uint8_t *edt)
{
    MWA_Event_t *event;;
    MWA_ENLPropertyPayload_t *payload;

    if (edt == NULL && pdc != 0) return (FALSE);

    event = MWA_AllocEvent(MWA_EVENT_QUERY_PROP,
                           sizeof(MWA_ENLPropertyPayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_ENLPropertyPayload_t *)(event + 1);

    payload->eoj = eoj;
    payload->epc = epc;
    payload->pdc = pdc;
    payload->edt = (uint8_t *)edt;

    if (!MWA_AddEventToQueue(&gMWA_StateMachineEventQ, event)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
static bool_t MWA_CheckFrameCmd(MWA_Frame_t *frame)
{
    switch (frame->frame_type) {
    case 0xffff:
        if (frame->cmd_no == 0x80 || frame->cmd_no == 0x81) {
            return (TRUE);
        }
        break;
    case 0x0000:
        if (frame->cmd_no == 0x80) {
            return (TRUE);
        }
        break;
    case 0x0001:
        if (frame->cmd_no == 0x01 || frame->cmd_no == 0x82) {
            return (TRUE);
        }
        break;
    case 0x0002:
        if (frame->cmd_no == 0x80 || frame->cmd_no == 0x81
            || frame->cmd_no == 0x82) {
            return (TRUE);
        }
        break;
    case 0x0003:
        if (frame->cmd_no == 0x11 || frame->cmd_no == 0x14
            || frame->cmd_no == 0x90) {
            return (TRUE);
        }
        break;
    case 0x00ff:
        return (TRUE);
    default:
        return (FALSE);
    }

    return (FALSE);
}

//=============================================================================
// Send Event (Receive Frame) to State Machine
// Call from Frame Communication
//=============================================================================
bool_t MWA_SendFrameToStateMachine(const uint8_t *buf, int len)
{
    MWA_Event_t *event;
    MWA_RecvFramePayload_t *payload;

    if (buf == NULL || len == 0) return (FALSE);

    // ----------------------------------------------------- Allocate Event ---
    event = MWA_AllocEvent(MWA_EVENT_RECV_FRAME,
                           sizeof(MWA_RecvFramePayload_t));
    if (event == NULL) return (FALSE);

    payload = (MWA_RecvFramePayload_t *)(event + 1);

    // ------------------------------------------- Unpack Frame from Buffer ---
    if (!MWA_UnpackFrame(buf, len, &payload->recv_frame)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }

    // ------------------------------------ Check Frame Type and Command No ---
    if (!MWA_CheckFrameCmd(&payload->recv_frame)) {
        MWA_SendErrorFrame(0x01, payload->recv_frame.frame_no);
        MWA_FreeEvent(event);
        return (FALSE);
    }

    // ---------------------------------------- Send Event to State Machine ---
    if (!MWA_AddEventToQueue(&gMWA_StateMachineEventQ, event)) {
        MWA_FreeEvent(event);
        return (FALSE);
    }

    return (TRUE);
}

//*****************************************************************************
// State Machine Handlers
//*****************************************************************************

//=============================================================================
static bool_t MWA_HandleRecvFrame_WrongState(MWA_Frame_t *frame,
                                             uint16_t result)
{
    if (frame->frame_type == 0x0001 && frame->cmd_no == 0x01) {
        // ---------------------------------- Receive FT=0x0001 and CN=0x01 ---
        if (result == 0x0101) {
            MWA_SendInitAdapterRes(result, frame->frame_no);

            return (TRUE);
        }
    }
    else if (frame->frame_type == 0x0003 && frame->cmd_no == 0x11) {
        // ---------------------------------- Receive FT=0x0003 and CN=0x11 ---
        uint8_t eoj[3];
        MWA_GetByteArray(frame, eoj, 3);

        // ------------------------------------- Send FT=0x0003 and CN=0x91 ---
        MWA_SendNotifyObjectDone(result, frame->frame_no, ELL_EOJ(eoj));

        return (TRUE);
    }
    else if (frame->frame_type == 0x0003 && frame->cmd_no == 0x14) {
        // ---------------------------------- Receive FT=0x0003 and CN=0x14 ---
        uint8_t eoj[3];
        uint16_t len;
        uint8_t epc;
        const uint8_t *edt;
        MWA_GetByteArray(frame, eoj, 3);
        MWA_GetShort(frame, &len);
        MWA_GetByte(frame, &epc);
        edt = MWA_GetPointer(frame);

        // ------------------------------------- Send FT=0x0003 and CN=0x94 ---
        if (result == 0x0101 || result == 0x0103) {
            MWA_SendAccessReadyRep(result, frame->frame_no,
                                   ELL_EOJ(eoj), epc, len - 1, edt);

            return (TRUE);
        }
    }

    return (FALSE);
}

//=============================================================================
// Enter Unrecognized State C0 (Open UART and Send Recognize I/F Request)
//=============================================================================
static void MWA_EnterState_UnrecognizedC0(void)
{
    uint8_t baudrate;

    ELL_Printf("-- [ADP] enter UNRECOGNIZED C0 --\n");
    LED_SetMode(1, 2);
    LED_SetMode(0, 0);

    gRecognizeIFRetryCnt = 0;

    // ---------------------------------------------------------- Open UART ---
    gUARTBaudrateTryIdx = 0;
    baudrate = gUARTBaudrateList[gUARTBaudrateTryIdx];
    MWA_OpenFrame(gUARTBaudrateTable[baudrate]);

    // ----------------------------------------- Send FT=0xFFFF and CN=0x00 ---
    MWA_SendRecognizeIF();
}

//=============================================================================
// Handle Unrecognized State C0 (Wait Recognize I/F Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_UnrecognizedC0(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0xffff && frame->cmd_no == 0x80) {
        // ------------------------------------- Wait FT=0xFFFF and CN=0x80 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetByte(frame, &gReadyDevice.type);
        MWA_GetByte(frame, &gReadyDevice.speed);

        ELL_Printf("FT=0xFFFF, CN=0x80\n");
        ELL_Printf("  Type=0x%02X, Speed=0x%02X(c/w 0x%02X)\n",
                   gReadyDevice.type, gReadyDevice.speed,
                   gUARTBaudrateList[gUARTBaudrateTryIdx]);

        if ((gReadyDevice.type & 0x02) != 0) {
            if (gReadyDevice.speed < MWA_BAUDRATE_MAX
                && gReadyDevice.speed == gUARTBaudrateList[gUARTBaudrateTryIdx]) {
                // Applicable
                ELL_Printf("  Applicable\n");
                gReadyDevice.recognized_result = 0x00;
            } else {
#if 0
                // Applicable (Unmatched Speed)
                ELL_Printf("  Applicable, but unmatched speed\n");
                gReadyDevice.recognized_result = 0x02;
#else
                // Applicable (Change Speed)
                ELL_Printf("  Applicable, Change Speed\n");
                gReadyDevice.recognized_result = 0x00;
#endif
            }
        } else {
            // Not Applicable
            ELL_Printf("  Not Applicable\n");
            gReadyDevice.recognized_result = 0x01;
        }
        MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C1);
    }
    else {
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Handle Unrecognized State C0 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_UnrecognizedC0(MWA_Event_t *event)
{
    uint8_t baudrate;

    MWA_CloseFrame();

    // -------------------------------------------------------- Retry Check ---
    gRecognizeIFRetryCnt ++;
    if (gRecognizeIFRetryCnt >= MWA_RECOGNIZE_IF_RETRY_MAX) {
        // ------------- Reach to Retry Count, Enter Error Connection State ---
        MWA_EnterAdapterState(MWA_ADP_STATE_ERR_CONNECTION);
    } else {
        // ------------------------------------------------------ Open UART ---
        gUARTBaudrateTryIdx ++;
        baudrate = gUARTBaudrateList[gUARTBaudrateTryIdx];
        if (baudrate == MWA_BAUDRATE_MAX) {
            gUARTBaudrateTryIdx = 0;
            baudrate = gUARTBaudrateList[gUARTBaudrateTryIdx];
        }
        MWA_OpenFrame(gUARTBaudrateTable[baudrate]);

        // ------------------------------------- Send FT=0xFFFF and CN=0x00 ---
        MWA_SendRecognizeIF();
    }

    return (TRUE);
}

//=============================================================================
// Enter Unrecognized State C1 (Send Confirm I/F Request)
//=============================================================================
static void MWA_EnterState_UnrecognizedC1(void)
{
    ELL_Printf("-- [ADP] enter UNRECOGNIZED C1 --\n");

    // ----------------------------------------- Send FT=0xFFFF and CN=0x01 ---
    MWA_SendConfirmIF(gReadyDevice.recognized_result);
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Unrecognized State C1 (Wait Confirm I/F Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_UnrecognizedC1(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0xffff && frame->cmd_no == 0x81) {
        // ------------------------------------- Wait FT=0xFFFF and CN=0x81 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        switch (gReadyDevice.recognized_result) {
        case 0x00: // Applicable
            if (gReadyDevice.speed != gUARTBaudrateList[gUARTBaudrateTryIdx]) {
                MWA_CloseFrame();
                MWA_OpenFrame(gUARTBaudrateTable[gReadyDevice.speed]);
            }
            // -------------------------------- Enter Unidentified C0 State ---
            MWA_EnterAdapterState(MWA_ADP_STATE_UNIDENTIFIED_C0);
            break;

        case 0x02: // Applicable (Unmatched Speed)
            // -------------------------------- Enter Unrecognized C0 State ---
            MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
            break;

        case 0x01:
        default:
            // ------------------------------- Enter Error Connection State ---
            MWA_EnterAdapterState(MWA_ADP_STATE_ERR_CONNECTION);
            break;
        }
    }
    else {
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Handle Unrecognized State C1 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_UnrecognizedC1(MWA_Event_t *event)
{
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        // ------------------------------------ Enter Unrecognized C0 State ---
        MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
    } else {
        // ----------------------------- Send FT=0xFFFF and CN=0x01 (Retry) ---
        MWA_SendConfirmIF(gReadyDevice.recognized_result);
    }

    return (TRUE);
}

//=============================================================================
// Enter Unidentified State C0 (Start Timer)
//=============================================================================
static void MWA_EnterState_UnidentifiedC0(void)
{
    ELL_Printf("-- [ADP] enter UNIDENTIFIED C0 --\n");

    MWA_StartTimer(&gMWA_SMTimer, 500, 0);
}

//=============================================================================
// Handle Unidentified State C0 (Wait 500ms)
//=============================================================================
static bool_t MWA_HandleRecvFrame_UnidentifiedC0(MWA_Frame_t *frame)
{
    return (MWA_HandleRecvFrame_WrongState(frame, 0x0101));
}

//=============================================================================
// Handle Unidentified State C0 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_UnidentifiedC0(MWA_Event_t *event)
{
    MWA_EnterAdapterState(MWA_ADP_STATE_UNIDENTIFIED_C1);

    return (TRUE);
}

//=============================================================================
// Enter Unidentified State C1 (Send Identify I/F Request)
//=============================================================================
static void MWA_EnterState_UnidentifiedC1(void)
{
    ELL_Printf("-- [ADP] enter UNIDENTIFIED C1 --\n");

    // ----------------------------------------- Send FT=0x0000 and CN=0x00 ---
    MWA_SendIdentifyIF(0x02, gReadyDevice.speed);
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Unidentified State C1 (Wait Identify I/F Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_UnidentifiedC1(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0000 && frame->cmd_no == 0x80) {
        // ------------------------------------- Wait FT=0x0000 and CN=0x80 ---
        uint16_t result;

        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetShort(frame, &result);
        switch (result) {
        case 0x0000: // Success
            ELL_Printf("  Success\n");
            MWA_EnterAdapterState(MWA_ADP_STATE_WAIT);
            break;
        case 0x0011: // Unmatched Adapter Type
            ELL_Printf("  Adapter Type Mismatched\n");
            MWA_EnterAdapterState(MWA_ADP_STATE_WAIT);
            break;
        case 0x0012: // Unmatched Objects
            /*** Discard Object Infos ***/
            ELL_Printf("  Discard Object Informations\n");
            MWA_EnterAdapterState(MWA_ADP_STATE_WAIT);
            break;
        case 0x0021: // Discard I/F Info
            /*** Discard Object and I/F Infos ***/
            ELL_Printf("  Discard Object and I/F Informations\n");
            MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
            break;
        default: // Error
            ELL_Printf("  ERROR\n");
            MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
            break;
        }
    }
    else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0101));
    }

    return (TRUE);
}

//=============================================================================
// Handle Unidentified State C1 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_UnidentifiedC1(MWA_Event_t *event)
{
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        // ------------------------------------ Enter Unrecognized C0 State ---
        MWA_EnterAdapterState(MWA_ADP_STATE_UNRECOGNIZED_C0);
    } else {
        // ----------------------------- Send FT=0x0000 and CN=0x00 (Retry) ---
        MWA_SendIdentifyIF(0x02, gReadyDevice.speed);
    }

    return (TRUE);
}

//=============================================================================
// Enter Wait State (Do Nothing)
//=============================================================================
static void MWA_EnterState_Wait(void)
{
    ELL_Printf("-- [ADP] enter WAIT --\n");
}

//=============================================================================
// Handle Wait State (Wait Init Adapter Request)
//=============================================================================
static bool_t MWA_HandleRecvFrame_Wait(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0001 && frame->cmd_no == 0x01) {
        // ------------------------------------- Wait FT=0x0001 and CN=0x01 ---
        MWA_GetShort(frame, &gReadyDevice.init_adapter);
        gReadyDevice.init_adapter_fn = frame->frame_no;

        MWA_EnterAdapterState(MWA_ADP_STATE_INIT_ADAPTER_C0);
    }
    else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0103));
    }

    return (TRUE);
}

//=============================================================================
// Handle Wait State (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_Wait(MWA_Event_t *event)
{
    return (TRUE);
}

//=============================================================================
// Enter Init Adapter State C0 (Send Init Adapter Response)
//=============================================================================
static void MWA_EnterState_InitAdapterC0(void)
{
    uint16_t res;

    ELL_Printf("-- [ADP] enter INIT ADAPTER C0 --\n");

    switch (gReadyDevice.init_adapter) {
    case 0x0001:
    case 0x0003:
    case 0x0005:
        // Start with Having Object Information
        res = 0x0000;
        break;
    case 0x0002:
    case 0x0004:
    case 0x0006:
        // Start with Discarding Object Information
        res = 0x0000;
        break;
    default:
        res = 0xffff;
        break;
    }

    // ----------------------------------------- Send FT=0x0001 and CN=0x81 ---
    MWA_SendInitAdapterRes(res, gReadyDevice.init_adapter_fn);

    MWA_SetSMTimeoutTimer(0x00010081, 300);
}

//=============================================================================
// Handle Init Adapter State C0 (Do Nothing)
//=============================================================================
static bool_t MWA_HandleRecvFrame_InitAdapterC0(MWA_Frame_t *frame)
{
    return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
}

//=============================================================================
// Handle Init Adapter State C0 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_InitAdapterC0(MWA_Event_t *event)
{
    MWA_EnterAdapterState(MWA_ADP_STATE_INIT_ADAPTER_C1);

    return (TRUE);
}

//=============================================================================
// Enter Init Adapter State C1 (Initialize Adapter)
//=============================================================================
static void MWA_EnterState_InitAdapterC1(void)
{
    ELL_Printf("-- [ADP] enter INIT ADAPTER C1 --\n");

    /*** Do Something Initializing Adapter ***/

    MWA_StartTimer(&gMWA_SMTimer, 10, 0);
}

//=============================================================================
// Handle Init Adapter State C1 (Do Nothing)
//=============================================================================
static bool_t MWA_HandleRecvFrame_InitAdapterC1(MWA_Frame_t *frame)
{
    return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
}

//=============================================================================
// Handle Init Adapter State C1 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_InitAdapterC1(MWA_Event_t *event)
{
    MWA_EnterAdapterState(MWA_ADP_STATE_INIT_ADAPTER_C2);

    return (TRUE);
}

//=============================================================================
// Enter Init Adapter State C2 (Send Start Adapter Notification)
//=============================================================================
static void MWA_EnterState_InitAdapterC2(void)
{
    ELL_Printf("-- [ADP] enter INIT ADAPTER C2 --\n");

    // ----------------------------------------- Send FT=0x0001 and CN=0x02 ---
    MWA_SendInitAdapterDone(0x0000);
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Init Adapter State C2 (Wait Start Adapter Notification Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_InitAdapterC2(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0001 && frame->cmd_no == 0x82) {
        // ------------------------------------- Wait FT=0x0001 and CN=0x82 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetShort(frame, &gReadyDevice.confirm_adapter);

        if (gReadyDevice.confirm_adapter == 0x0000) {
            MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C0);
        } else {
            MWA_EnterAdapterState(MWA_ADP_STATE_WAIT);
        }
    } else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
    }

    return (TRUE);
}

//=============================================================================
// Handle Init Adapter State C2 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_InitAdapterC2(MWA_Event_t *event)
{
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        MWA_EnterAdapterState(MWA_ADP_STATE_WAIT);
    } else {
        // ----------------------------- Send FT=0x0001 and CN=0x02 (Retry) ---
        MWA_SendInitAdapterDone(0x0000);
    }

    return (TRUE);
}

//=============================================================================
// Enter Construct Object State C0 (Send Query Object Request)
//=============================================================================
static void MWA_EnterState_ConstructObjC0(void)
{
    int cnt;

    ELL_Printf("-- [ADP] enter CONSTRUCT OBJECT C0 --\n");

    // ---------------------------------------------------- Initialize Work ---
    gNumAdapterObjectInfo = 0;
    for (cnt = 0; cnt < MWA_NUM_OBJECT_INFO; cnt ++) {
        if (gAdapterObjectInfo[cnt].props != NULL) {
            MEM_Free(gAdapterObjectInfo[cnt].props);
        }
        gAdpObjLoaded[cnt] = FALSE;
    }

    // ----------------------------------------- Send FT=0x0002 and CN=0x00 ---
    MWA_SendQueryObject();
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Construct Object State C0 (Wait Query Object Request)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ConstructObjC0(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0002 && frame->cmd_no == 0x80) {
        // ------------------------------------- Wait FT=0x0002 and CN=0x80 ---
        uint16_t result;
        uint8_t num_obj;
        uint8_t cnt;
        MWA_ObjectInfo_t *obj_info;
        int obj_idx;

        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetShort(frame, &result);
        MWA_GetByte(frame, &num_obj);

        obj_info = MEM_Alloc(sizeof(MWA_ObjectInfo_t));
        ELL_Assert(obj_info != NULL);
        for (cnt = 0; cnt < num_obj; cnt ++) {
            MEM_Set(obj_info, 0, sizeof(MWA_ObjectInfo_t));
            if (MWA_GetObjectInfo(frame, obj_info)) {
                if (gNumAdapterObjectInfo == 0) {
                    gNumAdapterObjectInfo = (obj_info->idx >> 4) & 0x0f;

                    /*** First 3 objects only ***/
                    if (gNumAdapterObjectInfo >= MWA_NUM_OBJECT_INFO) {
                        gNumAdapterObjectInfo = MWA_NUM_OBJECT_INFO;
                    }
                }
                obj_idx = obj_info->idx & 0x0f;
                if (obj_idx > 0 && obj_idx <= MWA_NUM_OBJECT_INFO) {
                    obj_idx --;
                    if (gAdpObjLoaded[obj_idx]) {
                        /*** Already Loaded ***/
                        ELL_Printf("  Object Already Loaded\n");
                        gReadyDevice.construct_obj = 0x0011;
                        MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C1);
                        MEM_Free(obj_info);
                        return (TRUE);
                    } else {
                        MEM_Copy(&gAdapterObjectInfo[obj_idx], obj_info,
                                 sizeof(MWA_ObjectInfo_t));
                        gAdpObjLoaded[obj_idx] = TRUE;
                    }
                } else {
                    /*** Object Index Error ***/
                    ELL_Printf("  Object Index Error\n");
                    gReadyDevice.construct_obj = 0x0011;
                    MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C1);
                    MEM_Free(obj_info);
                    return (TRUE);
                }
            } else {
                /*** Payload Structure Error ***/
                ELL_Printf("  Payload Structure Error\n");
                gReadyDevice.construct_obj = 0x0011;
                MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C1);
                MEM_Free(obj_info);
                return (TRUE);
            }
        }
        MEM_Free(obj_info);
        obj_info = NULL;

        for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
            if (!gAdpObjLoaded[cnt]) break;
        }
        if (cnt == gNumAdapterObjectInfo) {
            gReadyDevice.construct_obj = 0x0000;
            MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C1);
        } else {
            // --------------------------------- Send FT=0x0002 and CN=0x00 ---
            MWA_SendQueryObject();
            gStateRetryCnt = 0;
        }
    } else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
    }

    return (TRUE);
}

//=============================================================================
// Handle Construct Object State C0 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ConstructObjC0(MWA_Event_t *event)
{
    ELL_Printf("Response Timerout...\n");

    /*** RETRY? ***/
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
    } else {
        // ----------------------------- Send FT=0x0002 and CN=0x00 (Retry) ---
        MWA_SendQueryObject();
    }

    return (TRUE);
}

//=============================================================================
// Enter Construct Object State C1 (Send Construct Object Done)
//=============================================================================
static void MWA_EnterState_ConstructObjC1(void)
{
    ELL_Printf("-- [ADP] enter CONSTRUCT OBJECT C1 --\n");

    // ----------------------------------------- Send FT=0x0002 and CN=0x01 ---
    MWA_SendConstructObjectDone(gReadyDevice.construct_obj);
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Construct Object State C1 (Wait Construct Object Done Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ConstructObjC1(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0002 && frame->cmd_no == 0x81) {
        // ------------------------------------- Wait FT=0x0002 and CN=0x81 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetShort(frame, &gReadyDevice.confirm_obj);

        if (gReadyDevice.confirm_obj == 0x0000) {
            if (gReadyDevice.construct_obj == 0x0000) {
                MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C2);
            } else {
                /*** How Much Retrying? ***/
                MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C0);
            }
        } else {
            /*** OK? ***/
            MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
        }
    } else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
    }

    return (TRUE);
}

//=============================================================================
// Handle Construct Object State C1 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ConstructObjC1(MWA_Event_t *event)
{
    /*** RETRY? ***/
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        /*** OK? ***/
        MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
    } else {
        // ----------------------------- Send FT=0x0002 and CN=0x01 (Retry) ---
        MWA_SendConstructObjectDone(gReadyDevice.construct_obj);
    }

    return (TRUE);
}

//=============================================================================
// Enter Construct Object State C2 (Init ECHONET Lite)
//=============================================================================
static void MWA_EnterState_ConstructObjC2(void)
{
    ELL_Object_t *obj;

    MWA_ObjectInfo_t *info;
    MWA_Property_t *prop;
    int cnt, cnt2;

    uint8_t *define;
    int def_size, ptr;

    ELL_Printf("-- [ADP] enter CONSTRUCT OBJECT C2 --\n");

    // ------------------------------------------- Reset ECHONET Lite Stack ---
    MWA_ResetEchonet();

    // --------------------------------------------------- Register Objects ---
    for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
        info = &gAdapterObjectInfo[cnt];

        // ------------------------------------------ Calc Define Work Size ---
        def_size = 1;
        for (cnt2 = 0; cnt2 < info->num_props; cnt2 ++) {
            prop = &info->props[cnt2];
            def_size += prop->size + 3;
        }

        define = MEM_Alloc(def_size);
        if (define == NULL) {
            break;
        }
        MEM_Set(define, 0, def_size);

        // ----------------------------------------------- Fill Define Work ---
        ptr = 0;
        for (cnt2 = 0; cnt2 < info->num_props; cnt2 ++) {
            prop = &info->props[cnt2];
            define[ptr ++] = prop->epc;
            define[ptr ++] = prop->size;
#if 0
            ptr += prop->size;
#else // Allowed Zero Size
            if (prop->size > 0) {
                ptr += prop->size;
            }
#endif
            define[ptr ++] = prop->flag | EPC_FLAG_ADAPTER;
        }
        define[ptr] = 0;

        MWADbg_DumpMemory(define, def_size);

        // ------------------------------------------------ Register Object ---
        obj = ELL_RegisterObject(info->eoj, define);
        MEM_Free(define);
        define = NULL;
        if (obj == NULL) {
            break;
        }
    }
    if (cnt == gNumAdapterObjectInfo) {
        // ------------------------------------------ Register Node Profile ---
        define = MWA_CreateNodeProfileEPCDefine();
        obj = ELL_RegisterObject(0x0ef001, define);
        if (obj != NULL) {
            gReadyDevice.start_adapter = 0x0000;
        } else {
            gReadyDevice.start_adapter = 0x0011;
        }
        MEM_Free(define);
    } else {
        gReadyDevice.start_adapter = 0x0011;
    }

    MWA_StartTimer(&gMWA_SMTimer, 10, 0);
}

//=============================================================================
// Handle Construct Object State C2 (Init ECHONET Lite)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ConstructObjC2(MWA_Frame_t *frame)
{
    return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
}

//=============================================================================
// Handle Construct Object State C2 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ConstructObjC2(MWA_Event_t *event)
{
    MWA_EnterAdapterState(MWA_ADP_STATE_CONSTRUCT_OBJ_C3);

    return (TRUE);
}

//=============================================================================
// Enter Construct Object State C3 (Send Start Adapter Done)
//=============================================================================
static void MWA_EnterState_ConstructObjC3(void)
{
    ELL_Printf("-- [ADP] enter CONSTRUCT OBJECT C3 --\n");

    // ----------------------------------------- Send FT=0x0002 and CN=0x02 ---
    MWA_SendStartAdapterDone(gReadyDevice.start_adapter);
    gStateRetryCnt = 0;
}

//=============================================================================
// Handle Construct Object State C3 (Wait Start Adapter Done Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ConstructObjC3(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0002 && frame->cmd_no == 0x82) {
        // ------------------------------------- Wait FT=0x0002 and CN=0x82 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetShort(frame, &gReadyDevice.start_adapter_res);

        if (gReadyDevice.start_adapter_res == 0x0000) {
            if (gReadyDevice.start_adapter == 0x0000) {
                MWA_EnterAdapterState(MWA_ADP_STATE_START_ECHONET);
            } else {
                MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
            }
        } else {
            MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
        }
    } else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0104));
    }

    return (TRUE);
}

//=============================================================================
// Handle Construct Object State C3 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ConstructObjC3(MWA_Event_t *event)
{
    /*** RETRY? ***/
    gStateRetryCnt ++;
    if (gStateRetryCnt > 1) {
        /*** OK? ***/
        MWA_EnterAdapterState(MWA_ADP_STATE_ERR_STOP);
    } else {
        // ----------------------------- Send FT=0x0002 and CN=0x01 (Retry) ---
        MWA_SendStartAdapterDone(gReadyDevice.start_adapter);
    }

    return (TRUE);
}

//=============================================================================
static bool_t MWA_IsEPCFromReady(uint8_t epc)
{
    if (epc < 0x80) return (FALSE);

    switch (epc) {
    case 0x82:
    case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
        return (FALSE);
    default:
        return (TRUE);
    }
}

//=============================================================================
// Enter Start Echonet State (Initialize ECHONET Lite Properties)
//=============================================================================
static void MWA_EnterState_StartEchonet(void)
{
    MWA_ObjectInfo_t *info;
    MWA_Property_t *prop;
    ELL_Object_t *obj;
    int cnt, cnt2;
    int ptr;

    ELL_Printf("-- [ADP] enter START ECHONET --\n");

    // ------------------------------- Set Default Version Number and so on ---
    for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
        info = &gAdapterObjectInfo[cnt];

        obj = ELL_FindObject(info->eoj);
        if (obj == NULL) continue;

        for (cnt2 = 0; cnt2 < info->num_props; cnt2 ++) {
            prop = &info->props[cnt2];
            switch (prop->epc) {
            case 0x82: // Version Number
                ELL_InitProperty(obj, prop->epc, 4, info->version);
                break;
            case 0x8a: // Maker Code
                ELL_InitProperty(obj, prop->epc, 3, info->maker_code);
                break;
            case 0x8b: // Jigyosho Code
                ELL_InitProperty(obj, prop->epc, 3, info->location);
                break;
            case 0x8c: // Product Code
                ELL_InitProperty(obj, prop->epc, 12, info->product_code);
                break;
            case 0x8d: // Serial Number
                ELL_InitProperty(obj, prop->epc, 12, info->serial_number);
                break;
            case 0x8e: // Product Date
                ELL_InitProperty(obj, prop->epc, 4, info->product_date);
                break;

#ifdef FOR_SANKYO_TATEYAMA
			case 0x83:
			{
				uint8_t id_number[17];
				const uint8_t cmp_zero[6] = {0};
				_enet_address mac;
				ELL_GetProperty(obj, 0x83, id_number, 17);
				if (MEM_Compare(&id_number[11], cmp_zero, 6) == 0) {
					// if last 6 bytes are all 0x00, fill MAC address there.
					ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
					MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
					ELL_InitProperty(obj, 0x83, 17, id_number);
				}
			}
				break;
#endif
            }
        }
    }

    // ----- Get Properties from Ready Device and Set to ECHONET Lite Stack ---
    gInitPropertyPtr = 0;
    gInitPropertyNum = 0;
    for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
        info = &gAdapterObjectInfo[cnt];

        for (cnt2 = 0; cnt2 < info->num_props; cnt2 ++) {
            prop = &info->props[cnt2];
            if ((prop->flag & EPC_FLAG_RULE_GETUP) == 0
                && MWA_IsEPCFromReady(prop->epc)) {
                gInitPropertyNum ++;
            }
        }
    }

    if (gInitPropertyNum > 0) {
        gInitPropertyList = MEM_Alloc(sizeof(MWA_InitProperty_t)
                                      * gInitPropertyNum);
        if (gInitPropertyList != NULL) {
            ptr = 0;
            for (cnt = 0; cnt < gNumAdapterObjectInfo; cnt ++) {
                info = &gAdapterObjectInfo[cnt];

                for (cnt2 = 0; cnt2 < info->num_props; cnt2 ++) {
                    prop = &info->props[cnt2];
                    if ((prop->flag & EPC_FLAG_RULE_GETUP) == 0
                        && MWA_IsEPCFromReady(prop->epc)) {
                        gInitPropertyList[ptr].eoj = info->eoj;
                        gInitPropertyList[ptr].epc = prop->epc;
                        gInitPropertyList[ptr].size = prop->size;
                        ptr ++;
                    }
                }
            }
        } else {
            /*** ERROR ***/
        }

        // ------------------------------------- Send FT=0x0003 and CN=0x10 ---
        MWA_SendAccessObject(gInitPropertyList[0].eoj,
                             gInitPropertyList[0].epc, 0, NULL);

        // -------------------------------------------- Start Timeout Timer ---
        MWA_StartTimer(&gMWA_SMTimer, 3000, 0x00030010L);
    } else {
        gInitPropertyList = NULL;
        MWA_StartTimer(&gMWA_SMTimer, 10, 0);
    }
}

//=============================================================================
static void MWA_GetNextEPC(void)
{
    gInitPropertyPtr ++;
    if (gInitPropertyPtr < gInitPropertyNum) {
        MWA_InitProperty_t *init_prop;
        init_prop = &gInitPropertyList[gInitPropertyPtr];

        // ------------------------------------- Send FT=0x0003 and CN=0x10 ---
        MWA_SendAccessObject(init_prop->eoj, init_prop->epc, 0, NULL);

        // -------------------------------------------- Start Timeout Timer ---
        MWA_StartTimer(&gMWA_SMTimer, 3000, 0x00030010L);
    } else {
        if (gInitPropertyList != NULL) {
            MEM_Free(gInitPropertyList);
            gInitPropertyList = NULL;
        }

        // Notify Start ECHONET Lite Stack
        MWA_StartEchonet();

        MWA_EnterAdapterState(MWA_ADP_STATE_NORMAL_OPERATION_C0);
    }
}

//=============================================================================
// Handle Start Echonet State (Initialize ECHONET Lite Properties)
//=============================================================================
static bool_t MWA_HandleRecvFrame_NormalOperationC0(MWA_Frame_t *frame);
static bool_t MWA_HandleRecvFrame_StartEchonet(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0003 && frame->cmd_no == 0x90) {
        // ------------------------------------- Wait FT=0x0003 and CN=0x90 ---
        uint8_t eoj[3];
        uint16_t result, len;
        uint8_t epc;
        const uint8_t *edt;

        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetByteArray(frame, eoj, 3);
        MWA_GetShort(frame, &result);
        MWA_GetShort(frame, &len);
        MWA_GetByte(frame, &epc);
        edt = MWA_GetPointer(frame);

        if (result == 0x0000 && len > 1 && edt != NULL) {
            ELL_Object_t *obj;
            obj = ELL_FindObject(ELL_EOJ(eoj));
            if (obj != NULL) {
                ELL_InitProperty(obj, epc, len - 1, edt);
            }
        }

        // --------------------------------------------------- Get Next EPC ---
        MWA_GetNextEPC();
    } else {
        // return (FALSE);
        return (MWA_HandleRecvFrame_NormalOperationC0(frame));
    }

    return (TRUE);
}

//=============================================================================
// Handle Start Echonet State (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_StartEchonet(MWA_Event_t *event)
{
    MWA_TimeroutPayload_t *payload;

    payload = (MWA_TimeroutPayload_t *)(event + 1);

    if (payload->tag == 0) {
        MWA_StartEchonet();

        MWA_EnterAdapterState(MWA_ADP_STATE_NORMAL_OPERATION_C0);
    } else {
        MWA_GetNextEPC();
    }

    return (TRUE);
}

//=============================================================================
// Enter Normal Operation State C0 (Do Nothing)
//=============================================================================
static void MWA_EnterState_NormalOperationC0(void)
{
    ELL_Printf("-- [ADP] enter NORMAL OPERATION C0 --\n");
    LED_SetMode(1, 1);
    LED_SetMode(0, 0);

#if 0
    exit(0);
#endif
}

//=============================================================================
// Handle Normal Operation State C0 (Wait Request from Ready Device)
//=============================================================================
static bool_t MWA_HandleRecvFrame_NormalOperationC0(MWA_Frame_t *frame)
{
    uint8_t eoj[3];
    uint16_t result, len;
    uint8_t epc;
    const uint8_t *edt;
    uint8_t pdc;

    if (frame->frame_type == 0x0003 && frame->cmd_no == 0x11) {
        // ------------------------------------- Wait FT=0x0003 and CN=0x11 ---
        MWA_GetByteArray(frame, eoj, 3);
        MWA_GetShort(frame, &len);
        MWA_GetByte(frame, &epc);
        edt = MWA_GetPointer(frame);

        // ---------------------- Announce Property to ECHONET Lite Network ---
        result = 0xffff;
        if (len > 1 && edt != NULL) {
            if (MWA_AnnoProperty(ELL_EOJ(eoj), epc, len - 1, edt)) {
                result = 0x0000;
            }
        }

        // ------------------------------------- Send FT=0x0003 and CN=0x91 ---
        MWA_SendNotifyObjectDone(result, frame->frame_no, ELL_EOJ(eoj));
    }
    else if (frame->frame_type == 0x0003 && frame->cmd_no == 0x14) {
        // ------------------------------------- Wait FT=0x0003 and CN=0x14 ---
        MWA_GetByteArray(frame, eoj, 3);
        MWA_GetShort(frame, &len);
        MWA_GetByte(frame, &epc);
        edt = MWA_GetPointer(frame);

        // ---------------------- Set or Get Property to ECHONET Lite Stack ---
        result = 0x0011;
        if (len == 1) {
            /*** Get Property ***/
            pdc = MWA_GetProperty(ELL_EOJ(eoj),
                                  epc, gMWAEDTWork, MWA_EDT_WORK_SIZE);
            if (pdc > 0) {
                edt = gMWAEDTWork;
                result = 0x0000;
            } else {
                edt = NULL;
            }
        } else if (len > 1) {
            /*** Set Property ***/
            pdc = len - 1;
            if (MWA_SetProperty(ELL_EOJ(eoj), epc, pdc, edt)) {
                edt = NULL;
                pdc = 0;
                result = 0x0000;
            }
        }

        // ------------------------------------- Send FT=0x0003 and CN=0x94 ---
        MWA_SendAccessReadyRep(result, frame->frame_no,
                               ELL_EOJ(eoj), epc, pdc, edt);
    } else {
        return (FALSE);
    }

    return (TRUE);
}

//=============================================================================
// Handle Normal Operation State C0 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_NormalOperationC0(MWA_Event_t *event)
{
    return (TRUE);
}

//=============================================================================
// Enter Normal Operation State C1 (Do Nothing)
//=============================================================================
static void MWA_EnterState_NormalOperationC1(void)
{
    ELL_Printf("-- [ADP] enter NORMAL OPERATION C1 --\n");
}

//=============================================================================
// Handle Normal Operation State C1 (Wait Access Response)
//=============================================================================
static bool_t MWA_HandleRecvFrame_NormalOperationC1(MWA_Frame_t *frame)
{
    uint8_t eoj[3];
    uint16_t result, len;
    uint8_t epc;
    uint8_t pdc;
    const uint8_t *edt;

    if (frame->frame_type == 0x0003 && frame->cmd_no == 0x90) {
        // ------------------------------------- Wait FT=0x0003 and CN=0x90 ---
        MWA_CancelTimer(&gMWA_SMTimer);

        MWA_GetByteArray(frame, eoj, 3);
        MWA_GetShort(frame, &result);
        MWA_GetShort(frame, &len);
        MWA_GetByte(frame, &epc);
        edt = MWA_GetPointer(frame);

        if (result == 0x0000 && len > 0 && len <= 256) {
            pdc = (uint8_t)(len - 1);
#ifdef FOR_SANKYO_TATEYAMA
			if (epc == 0x83 && pdc == 17) {
				const uint8_t cmp_zero[6] = {0};
				_enet_address mac;
				if (MEM_Compare(&edt[11], cmp_zero, 6) == 0) {
					// if last 6 bytes are all 0x00, fill MAC address there.
					ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
					MEM_Copy((uint8_t *)&edt[11], (uint8_t *)mac, 6);
				}
			}
#endif
            MWA_ResponseENLAccess(ELL_EOJ(eoj), epc, pdc, edt);
        } else {
            MWA_ResponseENLAccess(ELL_EOJ(eoj), 0, 0, NULL);
        }

        MWA_EnterAdapterState(MWA_ADP_STATE_NORMAL_OPERATION_C0);
    } else {
        /*** it might be come, FT=0x0003, CN=0x11 or FT=0x0003, CN=0x14 ***/
        return (MWA_HandleRecvFrame_NormalOperationC0(frame));
    }

    return (TRUE);
}

//=============================================================================
// Handle Normal Operation State C1 (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_NormalOperationC1(MWA_Event_t *event)
{
    MWA_ResponseENLAccess(0, 0, 0, NULL);

    MWA_EnterAdapterState(MWA_ADP_STATE_NORMAL_OPERATION_C0);

    return (TRUE);
}

//=============================================================================
// Enter Error Connection State (Do Nothing)
//=============================================================================
static void MWA_EnterState_ErrorConn(void)
{
    ELL_Printf("-- [ADP] enter ERROR CONNECTION --\n");
    LED_SetMode(1, 3);
    LED_SetMode(0, 0);
}

//=============================================================================
// Handle Error Connection State (Wait Init Adapter Request)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ErrorConn(MWA_Frame_t *frame)
{
    return (TRUE);
}

//=============================================================================
// Handle Error Connection State (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ErrorConn(MWA_Event_t *event)
{
    return (TRUE);
}

//=============================================================================
// Enter Error Stop State (Do Nothing)
//=============================================================================
static void MWA_EnterState_ErrorStop(void)
{
    ELL_Printf("-- [ADP] enter ERROR STOP --\n");
    LED_SetMode(1, 0);
    LED_SetMode(0, 1);
}

//=============================================================================
// Handle Error Stop State (Wait Init Adapter Request)
//=============================================================================
static bool_t MWA_HandleRecvFrame_ErrorStop(MWA_Frame_t *frame)
{
    if (frame->frame_type == 0x0001 && frame->cmd_no == 0x01) {
        // ------------------------------------- Wait FT=0x0001 and CN=0x01 ---
        MWA_GetShort(frame, &gReadyDevice.init_adapter);
        gReadyDevice.init_adapter_fn = frame->frame_no;

        MWA_EnterAdapterState(MWA_ADP_STATE_INIT_ADAPTER_C0);
    } else {
        return (MWA_HandleRecvFrame_WrongState(frame, 0x0105));
    }

    return (TRUE);
}

//=============================================================================
// Handle Error Stop State (Timeout)
//=============================================================================
static bool_t MWA_HandleRecvTimeout_ErrorStop(MWA_Event_t *event)
{
    return (TRUE);
}


//=============================================================================
// Event Handler Table
//=============================================================================
typedef struct {
    MWA_EnterStateCB_t *enter;
    MWA_RecvFrameCB_t *recv_frame;
    MWA_EventHandlerCB_t *timeout;
} MWA_AdapterHandlerTable_t;

static const MWA_AdapterHandlerTable_t gAdapterHandlerTable[] = {
    { NULL, NULL, NULL },
    { MWA_EnterState_UnrecognizedC0,
      MWA_HandleRecvFrame_UnrecognizedC0,
      MWA_HandleRecvTimeout_UnrecognizedC0 },
    { MWA_EnterState_UnrecognizedC1,
      MWA_HandleRecvFrame_UnrecognizedC1,
      MWA_HandleRecvTimeout_UnrecognizedC1 },
    { MWA_EnterState_UnidentifiedC0,
      MWA_HandleRecvFrame_UnidentifiedC0,
      MWA_HandleRecvTimeout_UnidentifiedC0 },
    { MWA_EnterState_UnidentifiedC1,
      MWA_HandleRecvFrame_UnidentifiedC1,
      MWA_HandleRecvTimeout_UnidentifiedC1 },
    { MWA_EnterState_Wait,
      MWA_HandleRecvFrame_Wait,
      MWA_HandleRecvTimeout_Wait },
    { MWA_EnterState_InitAdapterC0,
      MWA_HandleRecvFrame_InitAdapterC0,
      MWA_HandleRecvTimeout_InitAdapterC0 },
    { MWA_EnterState_InitAdapterC1,
      MWA_HandleRecvFrame_InitAdapterC1,
      MWA_HandleRecvTimeout_InitAdapterC1 },
    { MWA_EnterState_InitAdapterC2,
      MWA_HandleRecvFrame_InitAdapterC2,
      MWA_HandleRecvTimeout_InitAdapterC2 },
    { MWA_EnterState_ConstructObjC0,
      MWA_HandleRecvFrame_ConstructObjC0,
      MWA_HandleRecvTimeout_ConstructObjC0 },
    { MWA_EnterState_ConstructObjC1,
      MWA_HandleRecvFrame_ConstructObjC1,
      MWA_HandleRecvTimeout_ConstructObjC1 },
    { MWA_EnterState_ConstructObjC2,
      MWA_HandleRecvFrame_ConstructObjC2,
      MWA_HandleRecvTimeout_ConstructObjC2 },
    { MWA_EnterState_ConstructObjC3,
      MWA_HandleRecvFrame_ConstructObjC3,
      MWA_HandleRecvTimeout_ConstructObjC3 },
    { MWA_EnterState_StartEchonet,
      MWA_HandleRecvFrame_StartEchonet,
      MWA_HandleRecvTimeout_StartEchonet },
    { MWA_EnterState_NormalOperationC0,
      MWA_HandleRecvFrame_NormalOperationC0,
      MWA_HandleRecvTimeout_NormalOperationC0 },
    { MWA_EnterState_NormalOperationC1,
      MWA_HandleRecvFrame_NormalOperationC1,
      MWA_HandleRecvTimeout_NormalOperationC1 },
    { MWA_EnterState_ErrorConn,
      MWA_HandleRecvFrame_ErrorConn,
      MWA_HandleRecvTimeout_ErrorConn },
    { MWA_EnterState_ErrorStop,
      MWA_HandleRecvFrame_ErrorStop,
      MWA_HandleRecvTimeout_ErrorStop }
};

//=============================================================================
// Enter State Machine State
//=============================================================================
static void MWA_EnterAdapterState(MWA_AdpState_t next_state)
{
    MWA_EnterStateCB_t *handler;
    if (next_state < MWA_ADP_STATE_MAX) {
        handler = gAdapterHandlerTable[next_state].enter;

        if (handler != NULL) {
            (*handler)();
        }
    }

    gAdpState = next_state;
}

//=============================================================================
// Handle Adapter State Machine Event
//=============================================================================
bool_t MWA_HandleAdapter(MWA_Event_t *event)
{
    bool_t handled = FALSE;

    switch (event->id) {
    case MWA_EVENT_RECV_FRAME:
        // -------------------------------- Receive Frame from Ready Device ---
        {
            MWA_RecvFramePayload_t *payload;
            MWA_Frame_t *frame;

            payload = (MWA_RecvFramePayload_t *)(event + 1);
            frame = &payload->recv_frame;

            if (gAdpState < MWA_ADP_STATE_MAX) {
                MWA_RecvFrameCB_t *handler;
                handler = gAdapterHandlerTable[gAdpState].recv_frame;
                if (handler != NULL) {
                    handled = (*handler)(frame);
                }
            }

            if (frame->uart_buf != NULL) {
                MEM_Free(frame->uart_buf);
            }
        }
        break;

    case MWA_EVENT_TIMEOUT:
        // -------------------------------------------------------- Timeout ---
        if (gAdpState < MWA_ADP_STATE_MAX) {
            MWA_EventHandlerCB_t *handler;
            handler = gAdapterHandlerTable[gAdpState].timeout;
            if (handler != NULL) {
                handled = (*handler)(event);
            }
        }
        break;

    case MWA_EVENT_QUERY_PROP:
        // ---------------------------------------- Query Property from ENL ---
        if (gAdpState == MWA_ADP_STATE_NORMAL_OPERATION_C0) {
            MWA_ENLPropertyPayload_t *payload;
            payload = (MWA_ENLPropertyPayload_t *)(event + 1);

            /*** check success or failure ***/
            // --------------------------------- Send FT=0x0003 and CN=0x10 ---
            MWA_SendAccessObject(payload->eoj, payload->epc,
                                 payload->pdc, payload->edt);

            // ---------------------------------------- Start Timeout Timer ---
            MWA_StartTimer(&gMWA_SMTimer, 3000, 0x00030010L);

            MWA_EnterAdapterState(MWA_ADP_STATE_NORMAL_OPERATION_C1);
        }
        else {
            /*** Cannot Proccess ***/
            MWA_ResponseENLAccess(0, 0, 0, NULL);
        }
        break;

    default:
        break;
    }

    MWA_FreeEvent(event);

    return (handled);
}

//=============================================================================
MWA_Event_t *MWA_GetAdapterEvent(void)
{
    MWA_Event_t *event = NULL;

    event = MWA_GetEventFromQueue(&gMWA_StateMachineEventQ);

    return (event);
}

/******************************** END-OF-FILE ********************************/
