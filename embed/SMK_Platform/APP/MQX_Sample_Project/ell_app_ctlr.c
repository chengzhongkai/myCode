/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "ell_task.h"

#include "Config.h"

#ifdef APP_ENL_CONTROLLER

//=============================================================================
typedef struct {
	MESSAGE_HEADER_STRUCT header;

	/*** Application Dependent ***/
	uint32_t eoj;
	uint8_t cmd_id;

	uint8_t *ir_data;
	int ir_data_len;
} ELL_AdpReqMsg_t;

enum {
	ELL_ADP_REQ_NONE = 0,
	ELL_ADP_REQ_SEND_IR
};

static _pool_id gELLAdpPoolID = 0;
static _queue_id gELLAdpReqQID = 0;

static ELL_AdpReqMsg_t *ELLAdp_AllocMsg(_queue_id source_qid);

//=============================================================================
// Node Profile Definition
//=============================================================================
const uint8_t gNodeProfileEPC[] = {
    0x80,  1, 0x30, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
    0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              0x0e, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

    0xd3,  3, 0x00, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Instances
    0xd4,  2, 0x00, 0x02, EPC_FLAG_RULE_GET, // N of Classes (includes Node Prof)
    0xd5,  4, 0x01, 0x05, 0xff, 0x01,
              EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  4, 0x01, 0x05, 0xff, 0x01,
              EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  3, 0x01, 0x05, 0xff, EPC_FLAG_RULE_GET, // Node Class List

    0x00 /*** Terminater ***/
};

//=============================================================================
// Controller Class Definition
//=============================================================================
const uint8_t gControllerEPC[] = {
    0x80,  1, 0x30, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
#if (ELL_USE_FIXED_PDC != 0)
    0x81,  1, 0x00,
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
              | EPC_FLAG_RULE_SETUP | EPC_FLAG_RULE_GETUP,
#else
    0x81, 17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
              | EPC_FLAG_RULE_SETUP,
#endif
    0x82,  4, 0x00, 0x00, 0x46, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              0x05, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x88,  1, 0x42, EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

	0xf0,  1, 0x00,
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET
              | EPC_FLAG_RULE_SETUP | EPC_FLAG_RULE_GETUP,

    0x00 /*** Terminater ***/
};

//=============================================================================
const uint8_t gControllerEPCRange[] = {
    // 0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
    // 0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x81, ELL_PROP_VARIABLE,
    0xf0, ELL_PROP_VARIABLE,
    0x00 /*** Terminator ***/
};


#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
static uint8_t gInstallationLocation[17];
#endif

#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
// Set Installation Location Property
//=============================================================================
static bool_t SW_SetInstallationLocation(ELL_Object_t *obj,
                                         uint8_t pdc, const uint8_t *edt)
{
    uint8_t *store;

    if (obj == NULL) return (FALSE);

	store = &gInstallationLocation[0];

    if (pdc == 1) {
        if (edt[0] >= 0x01 && edt[0] <= 0x07) return (FALSE);
        if (store[0] != edt[0]) {
            ELL_NeedAnnounce(obj, 0x81);
        }
        store[0] = edt[0];
        return (TRUE);
    } else if (pdc == 17 && edt[0] == 0x01) {
        if (MEM_Compare(store, edt, 17) != 0) {
            ELL_NeedAnnounce(obj, 0x81);
        }
        MEM_Copy(store, edt, pdc);
        return (TRUE);
    }

    return (FALSE);
}
#endif

#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
// Get Installation Location Property
//=============================================================================
static int SW_GetInstallationLocation(ELL_Object_t *obj, uint8_t *edt)
{
    uint8_t *store;

    if (obj == NULL || edt == NULL) return (0);

	store = &gInstallationLocation[0];

    if (store[0] == 0x01) {
        MEM_Copy(edt, store, 17);
        return (17);
    } else {
        edt[0] = store[0];
        return (1);
    }
}
#endif

//=============================================================================
static uint8_t *gSendIRData = NULL;
static int gSendIRDataLen = 0;

//=============================================================================
// Set Property Callback
//=============================================================================
static bool_t SW_SetProperty(ELL_Object_t *obj,
                             uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
	ELL_AdpReqMsg_t *msg;

    if (obj == NULL || edt == NULL || pdc == 0) return (FALSE);

    switch (epc) {
	case 0xf0:
		// ------------------------ Controller User Defined Property (0xF0) ---
        if (!ELL_CheckProperty(obj, epc, pdc, edt)) {
            return (FALSE);
        }
		if (obj->eoj == 0x05ff01) {
			msg = ELLAdp_AllocMsg(0);
			if (msg == NULL) {
				return (FALSE);
			}

			msg->eoj = 0x05ff01;
			msg->cmd_id = ELL_ADP_REQ_SEND_IR;

			msg->ir_data = _mem_alloc(pdc);
			if (msg->ir_data == NULL) {
				_msg_free(msg);
				return (FALSE);
			}
			MEM_Copy(msg->ir_data, edt, pdc);
			msg->ir_data_len = pdc;

			if (!_msgq_send(msg)) {
				_mem_free(msg->ir_data);
				_msg_free(msg);
				return (FALSE);
			}

			return (TRUE);
		}
        return (ELL_SetProperty(obj, epc, pdc, edt));

    case 0x81:
		// ---------------------- Set Installation Location Property (0x81) ---
#if (ELL_USE_FIXED_PDC != 0)
        return (SW_SetInstallationLocation(obj, pdc, edt));
#else
        if (pdc == 1) {
            if (edt[0] >= 0x01 && edt[0] <= 0x07) return (FALSE);
        } else if (pdc != 17 || edt[0] != 0x01) {
            return (FALSE);
        }
        return (ELL_SetProperty(obj, epc, pdc, edt));
#endif

    default:
        break;
    }

    return (FALSE);
}

//=============================================================================
// Get Property Callback
//=============================================================================
static uint8_t SW_GetProperty(ELL_Object_t *obj,
                              uint8_t epc, uint8_t *edt, int max)
{
    if (obj == NULL || edt == NULL) return (0);

    switch (epc) {
#if (ELL_USE_FIXED_PDC != 0)
    case 0x81:
        if (max < 17) return (0);
        return (SW_GetInstallationLocation(obj, edt));
        break;
#endif

	case 0xf0:
		if (gSendIRData != NULL
			&& gSendIRDataLen > 0 && gSendIRDataLen <= max) {
			MEM_Copy(edt, gSendIRData, gSendIRDataLen);
			return (gSendIRDataLen);
		} else if (max > 0) {
			edt[0] = 0;
			return (1);
		}
		break;

    default:
        break;
    }

    return (0);
}

//=============================================================================
// Init Application
//=============================================================================
bool_t ELL_InitApp(void)
{
	return (TRUE);
}

//=============================================================================
// Construct ECHONET Lite Objects
//=============================================================================
bool_t ELL_ConstructObjects(void)
{
    ELL_Object_t *obj;

    uint8_t installation_location[] = { 0x00 };
	uint8_t id_number[17];
	_enet_address mac;

	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);

    ELL_InitObjects();

    // ---------------------------------------- Construct Controller Object ---
    obj = ELL_RegisterObject(0x05ff01, gControllerEPC);
    if (obj == NULL) return (FALSE);
    ELL_AddRangeRule(obj, gControllerEPCRange);

    ELL_AddCallback(obj, SW_SetProperty, SW_GetProperty);

	// ------------------------------------- Installation Location Property ---
#if (ELL_USE_FIXED_PDC != 0)
    gInstallationLocation[0] = 0x00;
#else
    ELL_InitProperty(obj, 0x81, 1, installation_location);
#endif

	// --------------------------------------------------- Init ID Property ---
	ELL_GetProperty(obj, 0x83, id_number, 17);
	MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
	ELL_InitProperty(obj, 0x83, 17, id_number);


    // -------------------------------------- Construct Node Profile Object ---
    obj = ELL_RegisterObject(0x0ef001, gNodeProfileEPC);
    if (obj == NULL) return (FALSE);

	// --------------------------------------------------- Init ID Property ---
	ELL_GetProperty(obj, 0x83, id_number, 17);
	MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
	ELL_InitProperty(obj, 0x83, 17, id_number);

    return (TRUE);
}

//=============================================================================
// Allocate Request Message
//=============================================================================
static ELL_AdpReqMsg_t *ELLAdp_AllocMsg(_queue_id source_qid)
{
	ELL_AdpReqMsg_t *msg;

	if (gELLAdpPoolID == MSGPOOL_NULL_POOL_ID) {
		return (NULL);
	}

	// ------------------------------------------------------ Alloc Message ---
	msg = _msg_alloc(gELLAdpPoolID);
	if (msg == NULL) {
		return (NULL);
	}

	// ---------------------------------------------- Init Message Contents ---
	msg->header.SOURCE_QID = source_qid;
	msg->header.TARGET_QID = gELLAdpReqQID;
	msg->header.SIZE = sizeof(ELL_AdpReqMsg_t);

	msg->eoj = 0;
	msg->cmd_id = 0;
	msg->ir_data = NULL;
	msg->ir_data_len = 0;

	return (msg);
}

//=============================================================================
// ECHONET Lite Adapter Control Task
//=============================================================================
void ELL_AdpCtrl_Task(uint32_t param)
{
	ELL_AdpReqMsg_t *req_msg;

	// ------------------------------------------------ Create Message Pool ---
	gELLAdpPoolID = _msgpool_create(sizeof(ELL_AdpReqMsg_t), 20, 0, 0);
	if (gELLAdpPoolID == MSGPOOL_NULL_POOL_ID) {
		err_printf("[ELL ADP] Cannot Create Message Pool\n");
		_task_block();
	}

	// ----------------------------------------- Open Request Message Queue ---
	gELLAdpReqQID = _msgq_open(ELL_ADP_CTRL_REQ_QUEUE, 0);
	if (gELLAdpReqQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("[ELL ADP] Cannot Open Request Message Queue\n");
		_task_block();
	}

	// -------------------------------------------------- Notify Task Start ---
	ELL_StartTask(ELL_TASK_START_ADP_CTRL);

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		req_msg = _msgq_receive(gELLAdpReqQID, 0);
		if (req_msg == NULL) continue;

		switch (req_msg->cmd_id) {
		case ELL_ADP_REQ_SEND_IR:
			// ---------------------------------------------------- Send IR ---
			if (gSendIRData != NULL) {
				_mem_free(gSendIRData);
				gSendIRData = NULL;
				gSendIRDataLen = 0;
			}
			gSendIRData = req_msg->ir_data;
			gSendIRDataLen = req_msg->ir_data_len;
			break;
		default:
			break;
		}

		_msg_free(req_msg);
	}
}

//=============================================================================
// ECHONET Lite Adapter Sync Task
//=============================================================================
void ELL_AdpSync_Task(uint32_t param)
{
	// -------------------------------------------------- Notify Task Start ---
	ELL_StartTask(ELL_TASK_START_ADP_SYNC);

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		_time_delay(500);
	}
}

#endif /* APP_ENL_CONTROLLER */

/******************************** END-OF-FILE ********************************/
