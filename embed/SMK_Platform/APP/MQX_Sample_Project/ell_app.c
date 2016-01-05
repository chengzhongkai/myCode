/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "ell_task.h"

#include "HaControl.h"

#include "Config.h"

#ifdef APP_ENL_ADAPTER

#if !defined(APP_OKAYA_HAx1_AIRCON) \
	&& !defined(APP_OKAYA_HAx1_WATER_HEATER) \
	&& !defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	&& !defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
#error This application requires APP_OKAYA_* defines
#endif

#ifdef APP_OKAYA_HAx1_AIRCON // Aircon for OKAYA
#define APP_OKAYA_SW_EOJ 0x013001
#endif

#ifdef APP_OKAYA_HAx1_WATER_HEATER // Instantaneous Water Heater for OKAYA
#define APP_OKAYA_SW_EOJ 0x027201
#endif

#ifdef APP_OKAYA_HAx1_FLOOR_HEATER // Floor Heater for OKAYA
#define APP_OKAYA_SW_EOJ 0x027b01
#endif

#define EOJ_DEF ((APP_OKAYA_SW_EOJ & 0xff0000) >> 16), \
		((APP_OKAYA_SW_EOJ & 0x00ff00) >> 8), \
		(APP_OKAYA_SW_EOJ & 0x0000ff) 

//=============================================================================
typedef struct {
	MESSAGE_HEADER_STRUCT header;

	/*** Application Dependent ***/
	uint32_t eoj;
	uint8_t cmd_id;
	uint8_t ha_number;
	HaState_t ha_state;
} ELL_AdpReqMsg_t;

enum {
	ELL_ADP_REQ_CHANGE_HA_STATE = 0,
	ELL_ADP_REQ_CHECK_HA_STATE
};

static _pool_id gELLAdpPoolID = 0;
static _queue_id gELLAdpReqQID = 0;

static ELL_AdpReqMsg_t *ELLAdp_AllocMsg(_queue_id source_qid);

//*****************************************************************************
// for OKAYA KOHKI
//*****************************************************************************

//=============================================================================
#if defined(APP_OKAYA_HAx1_AIRCON) \
	|| defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	|| defined(APP_OKAYA_HAx1_WATER_HEATER)
const uint8_t gNodeProfileEPC[] = {
    0x80,  1, 0x30, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
    0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              EOJ_DEF, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,
    0xd3,  3, 0x00, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Instances
    0xd4,  2, 0x00, 0x02, EPC_FLAG_RULE_GET, // N of Classes (includes Node Prof)
    0xd5,  4, 0x01, 0x05, 0xfd, 0x01,
              EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  4, 0x01, 0x05, 0xfd, 0x01,
              EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  3, 0x01, 0x05, 0xfd, EPC_FLAG_RULE_GET, // Node Class List

    0x00 /*** Terminater ***/
};
#endif

//=============================================================================
#if defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
const uint8_t gNodeProfileEPC[] = {
    0x80,  1, 0x30, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
    0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              0x0e, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,
    0xd3,  3, 0x00, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Instances
    0xd4,  2, 0x00, 0x02, EPC_FLAG_RULE_GET, // N of Classes (includes Node Prof)
    0xd5,  4, 0x01, 0x02, 0x6f, 0x01,
              EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  4, 0x01, 0x02, 0x6f, 0x01,
              EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  3, 0x01, 0x02, 0x6f, EPC_FLAG_RULE_GET, // Node Class List

    0x00 /*** Terminater ***/
};
#endif

//=============================================================================
// HA Switch Class (Aircon Custom)
//=============================================================================
#if defined(APP_OKAYA_HAx1_AIRCON) \
	|| defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	|| defined(APP_OKAYA_HAx1_WATER_HEATER)
const uint8_t gSwitchEPC[] = {
    0x80,  1, 0x31, (EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
					 | EPC_FLAG_RULE_SETUP),
#if (ELL_USE_FIXED_PDC != 0)
    0x81,  1, 0x00, (EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
					 | EPC_FLAG_RULE_SETUP | EPC_FLAG_RULE_GETUP,
#else
    0x81, 17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO
              | EPC_FLAG_RULE_SETUP,
#endif
    0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
              EOJ_DEF, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x88,  1, 0x42, EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

    0x00 /*** Terminater ***/
};

//=============================================================================
const uint8_t gSwitchEPCRange[] = {
    0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
    // 0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x81, ELL_PROP_VARIABLE,
    0x00 /*** Terminator ***/
};
#endif

//=============================================================================
// Electric Lock Class
//=============================================================================
#if defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
const uint8_t gElectricLockEPC[] = {
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
    0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
    0x88,  1, 0x42, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO),
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

	0xe0,  1, 0x41, (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_SET
					 | EPC_FLAG_RULE_ANNO | EPC_FLAG_RULE_SETUP),
	// 0x41 = Locked
	// 0x42 = Unlocked

    0x00 /*** Terminater ***/
};

//=============================================================================
const uint8_t gElectricLockEPCRange[] = {
    0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
    // 0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x81, ELL_PROP_VARIABLE,
	0xe0, ELL_PROP_LIST, 0x02, 0x41, 0x42,
    0x00 /*** Terminator ***/
};
#endif


#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
static uint8_t gInstallationLocation[1][17];
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

    if (obj->eoj == 0x05fd01) {
        store = &gInstallationLocation[0][0];
    } else {
        return (FALSE);
    }

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

    if (obj->eoj == 0x05fd01) {
        store = &gInstallationLocation[0][0];
    } else {
        return (0);
    }

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
// Set Property Callback
//=============================================================================
static bool_t SW_SetProperty(ELL_Object_t *obj,
                             uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
	ELL_AdpReqMsg_t *msg;

    if (obj == NULL || edt == NULL || pdc == 0) return (FALSE);

    switch (epc) {
    case 0x80:
		// -------------------------------- Set Power State Property (0x80) ---
        if (!ELL_CheckProperty(obj, epc, pdc, edt)) {
            return (FALSE);
        }
        if (obj->eoj == 0x05fd01) {
			msg = ELLAdp_AllocMsg(0);
			if (msg == NULL) {
				return (FALSE);
			}

			msg->eoj = 0x05fd01;
			msg->cmd_id = ELL_ADP_REQ_CHANGE_HA_STATE;
			msg->ha_number = 0;
			if (edt[0] == 0x30) {
				msg->ha_state = HA_STATE_ON;
			} else {
				msg->ha_state = HA_STATE_OFF;
			}
			if (!_msgq_send(msg)) {
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

#if defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
	case 0xe0:
		// ------------------------ Set Electric Lock State Property (0xE0) ---
        if (!ELL_CheckProperty(obj, epc, pdc, edt)) {
            return (FALSE);
        }
		if (obj->eoj == 0x026f01) {
			msg = ELLAdp_AllocMsg(0);
			if (msg == NULL) {
				return (FALSE);
			}

			msg->eoj = 0x026f01;
			msg->cmd_id = ELL_ADP_REQ_CHANGE_HA_STATE;
			msg->ha_number = 0;
			if (edt[0] == 0x41) {
				msg->ha_state = HA_STATE_ON;
			} else {
				msg->ha_state = HA_STATE_OFF;
			}
			if (!_msgq_send(msg)) {
				_msg_free(msg);
				return (FALSE);
			}

			return (TRUE);
		}
        return (ELL_SetProperty(obj, epc, pdc, edt));
#endif

    default:
        break;
    }

    return (FALSE);
}

#if (ELL_USE_FIXED_PDC != 0)
//=============================================================================
// Get Property Callback
//=============================================================================
static uint8_t SW_GetProperty(ELL_Object_t *obj,
                              uint8_t epc, uint8_t *edt, int max)
{
    if (obj == NULL || edt == NULL) return (0);

    switch (epc) {
    case 0x81:
        if (max < 17) return (0);
        return (SW_GetInstallationLocation(obj, edt));
        break;

    default:
        break;
    }

    return (0);
}
#endif


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

	HaState_t ha_state;
	uint8_t edt[1];

    ELL_InitObjects();

#if defined(APP_OKAYA_HAx1_AIRCON) \
	|| defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	|| defined(APP_OKAYA_HAx1_WATER_HEATER)
    // ----------------------------------- Construct Switch Object (Aircon) ---
    obj = ELL_RegisterObject(0x05fd01, gSwitchEPC);
    if (obj == NULL) return (FALSE);
    ELL_AddRangeRule(obj, gSwitchEPCRange);

#if (ELL_USE_FIXED_PDC != 0)
    ELL_AddCallback(obj, SW_SetProperty, SW_GetProperty);
    gInstallationLocation[0][0] = 0x00;
#else
    ELL_AddCallback(obj, SW_SetProperty, NULL);
    ELL_InitProperty(obj, 0x81, 1, installation_location);
#endif

	ELL_GetProperty(obj, 0x83, id_number, 17);
	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
	MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
	ELL_InitProperty(obj, 0x83, 17, id_number);

	// Init Power State Property (0x80)
	ha_state = HaControl_GetState(0);
	edt[0] = (ha_state == HA_STATE_ON) ? 0x30 : 0x31;
	ELL_InitProperty(obj, 0x80, 1, edt);

#elif defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)

    // ------------------------------------- Construct Electric Lock Object ---
    obj = ELL_RegisterObject(0x026f01, gElectricLockEPC);
    if (obj == NULL) return (FALSE);
    ELL_AddRangeRule(obj, gElectricLockEPCRange);

#if (ELL_USE_FIXED_PDC != 0)
    ELL_AddCallback(obj, SW_SetProperty, SW_GetProperty);
    gInstallationLocation[0][0] = 0x00;
#else
    ELL_AddCallback(obj, SW_SetProperty, NULL);
    ELL_InitProperty(obj, 0x81, 1, installation_location);
#endif

	// Init Set Lock State Property (0xE0)
	ha_state = HaControl_GetState(0);
	edt[0] = (ha_state == HA_STATE_ON) ? 0x41 : 0x42;
	ELL_InitProperty(obj, 0xe0, 1, edt);

#endif

    // -------------------------------------- Construct Node Profile Object ---
    obj = ELL_RegisterObject(0x0ef001, gNodeProfileEPC);
    if (obj == NULL) return (FALSE);

	// ------------------------------------ Init ID Property (Node Profile) ---
	ELL_GetProperty(obj, 0x83, id_number, 17);
	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
	MEM_Copy(&id_number[11], (uint8_t *)mac, 6);
	ELL_InitProperty(obj, 0x83, 17, id_number);

    return (TRUE);
}

//=============================================================================
// Influte HA State to ECHONET Lite Object's Property
//=============================================================================
static void SW_InfluteHAState2ELL(HaState_t ha_state, uint32_t eoj)
{
    ELL_Object_t *obj;
    uint8_t edt[2];

	obj = ELL_FindObject(eoj);
	if (obj != NULL) {
#if defined(APP_OKAYA_HAx1_AIRCON) \
	|| defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	|| defined(APP_OKAYA_HAx1_WATER_HEATER)
		edt[0] = (ha_state == HA_STATE_ON) ? 0x30 : 0x31;
		ELL_SetProperty(obj, 0x80, 1, edt);
#elif defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
		edt[0] = (ha_state == HA_STATE_ON) ? 0x41 : 0x42;
		ELL_SetProperty(obj, 0xe0, 1, edt);
#endif
	}
}

//=============================================================================
// Check HA State
//=============================================================================
static void SW_CheckHAState(uint8_t ha_number, uint32_t eoj)
{
	HaState_t ha_state;

	ha_state = HaControl_GetState(ha_number);
	if (ha_state != HA_STATE_ERROR) {
		SW_InfluteHAState2ELL(ha_state, eoj);
	}
}

//=============================================================================
// Control HA State
//=============================================================================
static void SW_ControlHAState(uint8_t ha_number,
							  HaState_t ha_state, uint32_t eoj)
{
	HaState_t new_state;

	new_state = HaControl_ChangeState(ha_number, ha_state);
	if (new_state != HA_STATE_ERROR) {
		SW_InfluteHAState2ELL(new_state, eoj);
	}
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
	msg->ha_number = 0;
	msg->ha_state = HA_STATE_OFF;

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
		case ELL_ADP_REQ_CHANGE_HA_STATE:
			// ------------------------------------------- Control HA State ---
			SW_ControlHAState(req_msg->ha_number,
							  req_msg->ha_state, req_msg->eoj);
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
#if defined(APP_OKAYA_HAx1_AIRCON) \
	|| defined(APP_OKAYA_HAx1_FLOOR_HEATER) \
	|| defined(APP_OKAYA_HAx1_WATER_HEATER)
		_time_delay(500);
		SW_CheckHAState(0, 0x05fd01);
#elif defined(APP_OKAYA_HAx1_ELECTRIC_LOCK)
		_time_delay(500);
		SW_CheckHAState(0, 0x026f01);
#endif
	}
}

#endif /* APP_ENL_ADAPTER */

/******************************** END-OF-FILE ********************************/
