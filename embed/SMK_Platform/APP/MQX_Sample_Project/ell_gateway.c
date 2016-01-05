/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "echonetlitelite.h"
#include "websocket.h"
#include "Config.h"
#include "Tasks.h"

#include <ipcfg.h>
#include <timer.h>

#include "mqx_utils.h"
#include "flash_utils.h"

#ifdef APP_ENL_CONTROLLER

//=============================================================================
extern bool_t ELL_SendPacketEx(void *dest, const uint8_t *buf, int len);

#define ELL_GATEWAY_SEOJ 0x05ff01

static uint16_t gGatewayTID = 0x0001;

static uint8_t gGatewaySendBuf[1024];

//=============================================================================
// Message System
//=============================================================================
typedef struct {
	uint16_t tid;
	uint32_t eoj;
	IPv4Addr_t ipaddr;
	uint8_t epc;
	uint8_t pdc;
	uint8_t *edt;
} ELL_GW_RecvProp_t;

typedef struct {
	IPv4Addr_t ipaddr;
	uint32_t eoj;
	uint8_t epc;
} ELL_GW_GetEPC_t;

typedef struct {
	IPv4Addr_t ipaddr;
	uint32_t eoj;
	uint8_t epc;
	uint8_t pdc;
	uint8_t *edt;
} ELL_GW_AnnoEPC_t;

typedef struct {
	IPv4Addr_t ipaddr;
	uint32_t eoj;
	uint8_t epc;
	uint8_t pdc;
	uint8_t *edt;
} ELL_GW_SetEPC_t;

typedef struct {
	MESSAGE_HEADER_STRUCT header;

	uint8_t msg_id;
	union {
		ELL_GW_RecvProp_t recv_prop;

		ELL_GW_GetEPC_t get_epc;
		ELL_GW_AnnoEPC_t anno_epc;
		ELL_GW_SetEPC_t set_epc;
	} param;
} ELL_GatewayMsg_t;

enum {
	ELL_GW_MSG_NONE = 0,
	ELL_GW_MSG_RECV_PROP,
	ELL_GW_MSG_RECV_TIMEOUT,
	ELL_GW_MSG_REFRESH,

	ELL_GW_MSG_GET_EPC,
	ELL_GW_MSG_SET_EPC,
	ELL_GW_MSG_ANNO_EPC,

	ELL_GW_MSG_KEEP_ALIVE,
	ELL_GW_MSG_SERVER_STOP
};

//=============================================================================
static _pool_id gELLGWAppPoolID = 0;
static _queue_id gELLGWAppQID = 0;

static _pool_id gELLGWSvrPoolID = 0;
static _queue_id gELLGWSvrQID = 0;

//=============================================================================
typedef struct {
	IPv4Addr_t ipaddr;
	uint32_t eoj;
	uint8_t dev_id[32];
	uint8_t epc_map[64];
} ELL_DeviceInfo_t;

#define devIdLen(dev_id) ((dev_id)[1] + 2)

static uint8_t gDeviceInfoIdx = 0;
static ELL_DeviceInfo_t gDeviceInfoTable[4] = {0};

static uint8_t gDeviceInfoWorkIdx = 0;

typedef enum {
	ELL_GW_STATE_NONE = 0,
	ELL_GW_STATE_INIT,
	ELL_GW_STATE_NODE_PROFILE_ID,
	ELL_GW_STATE_OBJECT_ID,
	ELL_GW_STATE_PROP_LIST_0x9D,
	ELL_GW_STATE_PROP_LIST_0x9E,
	ELL_GW_STATE_PROP_LIST_0x9F,

	ELL_GW_STATE_NORMAL
} ELL_GatewayState_t;
static ELL_GatewayState_t gGatewayState;

static enum {
	ELL_GW_SVR_STATE_NONE = 0,
	ELL_GW_SVR_STATE_START,

	ELL_GW_SVR_STATE_NORMAL
} gGatewaySvrState = ELL_GW_SVR_STATE_NONE;

//=============================================================================
static void getEPC(IPv4Addr_t ipaddr, uint32_t eoj, uint8_t epc)
{
	int send_size;
	uint8_t epc_list[2];

	epc_list[0] = epc;
	epc_list[1] = 0;
	send_size = ELL_MakeGetPacket(gGatewayTID, ELL_GATEWAY_SEOJ, eoj, epc_list,
								  gGatewaySendBuf, sizeof(gGatewaySendBuf));
	if (send_size == 0) return;
	gGatewayTID ++;

#ifdef DEBUG
	ELLDbg_PrintPacket("to", ipaddr, gGatewaySendBuf, send_size);
#endif
	if (ipaddr == 0) {
		ELL_SendPacketEx(NULL, gGatewaySendBuf, send_size);
	} else {
		ELL_SendPacketEx(&ipaddr, gGatewaySendBuf, send_size);
	}
}

//=============================================================================
static void setEPC(IPv4Addr_t ipaddr,
				   uint32_t eoj, uint8_t epc, uint8_t pdc, uint8_t *edt)
{
	int send_size;
	ELL_Property_t prop_list[2];

	prop_list[0].epc = epc;
	prop_list[0].pdc = pdc;
	prop_list[0].edt = edt;
	prop_list[1].epc = 0;
	prop_list[1].pdc = 0;
	prop_list[1].edt = NULL;

	send_size = ELL_MakePacket(gGatewayTID, ELL_GATEWAY_SEOJ,
							   eoj, ESV_SetI, prop_list,
							   gGatewaySendBuf, sizeof(gGatewaySendBuf));
	if (send_size == 0) return;
	gGatewayTID ++;

#ifdef DEBUG
	ELLDbg_PrintPacket("to", ipaddr, gGatewaySendBuf, send_size);
#endif
	if (ipaddr == 0) {
		ELL_SendPacketEx(NULL, gGatewaySendBuf, send_size);
	} else {
		ELL_SendPacketEx(&ipaddr, gGatewaySendBuf, send_size);
	}
}

//=============================================================================
static void multicastGetEPC(uint32_t eoj, uint8_t epc)
{
	getEPC(0, eoj, epc);

#if 0
	int send_size;
	uint8_t epc_list[2];

	epc_list[0] = epc;
	epc_list[1] = 0;
	send_size = ELL_MakeGetPacket(gGatewayTID, ELL_GATEWAY_SEOJ, eoj, epc_list,
								  gGatewaySendBuf, sizeof(gGatewaySendBuf));
	if (send_size == 0) return;
	gGatewayTID ++;

#ifdef DEBUG
	ELLDbg_PrintPacket("to", 0, gGatewaySendBuf, send_size);
#endif
	ELL_SendPacketEx(NULL, gGatewaySendBuf, send_size);
#endif
}

//=============================================================================
static ELL_GatewayMsg_t *ELL_AllocGWMsg(_pool_id pool_id,
										_queue_id dest_qid, _queue_id src_qid)
{
	ELL_GatewayMsg_t *msg;

	if (pool_id == MSGPOOL_NULL_POOL_ID) {
		return (NULL);
	}

	// ------------------------------------------------------ Alloc Message ---
	msg = _msg_alloc(pool_id);
	if (msg == NULL) {
		return (NULL);
	}

	// ---------------------------------------------- Init Message Contents ---
	msg->header.SOURCE_QID = src_qid;
	msg->header.TARGET_QID = dest_qid;
	msg->header.SIZE = sizeof(ELL_GatewayMsg_t);

	msg->msg_id = 0;

	return (msg);
}

//=============================================================================
static void ELL_RecvProperty(const ELL_Header_t *header, const void *src,
							 const ELL_Property_t *prop)
{
	if (header == NULL || prop == NULL) return;

	printf("TID:0x%04x, SEOJ:0x%06x, DEOJ:0x%06x, ESV:0x%02x\n",
		   header->tid, header->seoj, header->deoj, header->esv);

	if (src == NULL) {
		printf("from Multicast\n");
	} else {
		IPv4Addr_t addr = *((IPv4Addr_t *)src);
		printf("from %d.%d.%d.%d\n",
			   (addr >> 24) & 0xff, (addr >> 16) & 0xff,
			   (addr >> 8) & 0xff, addr & 0xff);
	}

	printf("EPC:0x%02x, PDC:%d\n", prop->epc, prop->pdc);
	if (prop->pdc == 0 || prop->edt == NULL) {
		printf("EDT:null\n");
	} else {
		printf("EDT:");
		for (int cnt = 0; cnt < prop->pdc; cnt ++) {
			printf("0x%02x, ", prop->edt[cnt]);
		}
		printf("\n");
	}

#if 0
	if (header->esv != ESV_Get_Res && header->esv != ESV_INF) {
		return;
	}
	if (prop->pdc == 0) {
		return;
	}
#endif

	ELL_GatewayMsg_t *msg;

	msg = ELL_AllocGWMsg(gELLGWAppPoolID, gELLGWAppQID, 0);
	if (msg == NULL) return;

	msg->msg_id = ELL_GW_MSG_RECV_PROP;
	msg->param.recv_prop.tid = header->tid;
	msg->param.recv_prop.eoj = header->seoj;
	msg->param.recv_prop.ipaddr = *((IPv4Addr_t *)src);
	msg->param.recv_prop.epc = prop->epc;
	msg->param.recv_prop.pdc = prop->pdc;
	if (prop->pdc == 0) {
		msg->param.recv_prop.edt = NULL;
	} else {
		msg->param.recv_prop.edt = _mem_alloc(prop->pdc);
		if (msg->param.recv_prop.edt == NULL) {
			_msg_free(msg);
			return;
		}
		MEM_Copy(msg->param.recv_prop.edt, prop->edt, prop->pdc);
	}

	if (!_msgq_send(msg)) {
		if (msg->param.recv_prop.edt != NULL) {
			_mem_free(msg->param.recv_prop.edt);
		}
		_msg_free(msg);
	}
}

//=============================================================================
static void ELL_GetD6Timeout(_timer_id id, void *param,
							 uint32_t unknown1, uint32_t unknown2)
{
	ELL_GatewayMsg_t *msg;

	msg = ELL_AllocGWMsg(gELLGWAppPoolID, gELLGWAppQID, 0);
	if (msg == NULL) return;

	msg->msg_id = ELL_GW_MSG_RECV_TIMEOUT;

	if (!_msgq_send(msg)) {
		_msg_free(msg);
	}
}

//=============================================================================
static void ELL_AddDeviceInfo(IPv4Addr_t ipaddr, uint32_t eoj)
{
	if (gDeviceInfoIdx >= 4) return;

	ELL_DeviceInfo_t *table = &gDeviceInfoTable[gDeviceInfoIdx];
	gDeviceInfoIdx ++;

	table->ipaddr = ipaddr;
	table->eoj = eoj;
}

//=============================================================================
static bool_t ELL_GetNodeProfileID(void)
{
	ELL_DeviceInfo_t *table;

	if (gDeviceInfoWorkIdx >= gDeviceInfoIdx) return (FALSE);

	table = &gDeviceInfoTable[gDeviceInfoWorkIdx];

	getEPC(table->ipaddr, 0x0ef001, 0x83);

	return (TRUE);
}

//=============================================================================
static bool_t ELL_GetObjectID(void)
{
	ELL_DeviceInfo_t *table;

	if (gDeviceInfoWorkIdx >= gDeviceInfoIdx) return (FALSE);

	table = &gDeviceInfoTable[gDeviceInfoWorkIdx];

	getEPC(table->ipaddr, table->eoj, 0x83);

	return (TRUE);
}

//=============================================================================
static void ELL_AddDeviceID(uint8_t dev_id[32])
{
	if (gDeviceInfoWorkIdx >= 4) return;

	ELL_DeviceInfo_t *table = &gDeviceInfoTable[gDeviceInfoWorkIdx];
	gDeviceInfoWorkIdx ++;

	MEM_Copy(table->dev_id, dev_id, 32);
}

//=============================================================================
static bool_t ELL_GetProp(uint8_t epc)
{
	ELL_DeviceInfo_t *table;

	if (gDeviceInfoWorkIdx >= gDeviceInfoIdx) return (FALSE);

	table = &gDeviceInfoTable[gDeviceInfoWorkIdx];

	getEPC(table->ipaddr, table->eoj, epc);

	return (TRUE);
}

//=============================================================================
// flag 0x01 : Get
// flag 0x02 : Set
// flag 0x04 : Anno
//=============================================================================
static void ELL_AddPropList(uint8_t pdc, uint8_t *edt, uint8_t flag)
{
	int idx;
	int base;
	int cnt;
	uint8_t bit;

	if (gDeviceInfoWorkIdx >= 4) return;

	ELL_DeviceInfo_t *table = &gDeviceInfoTable[gDeviceInfoWorkIdx];
	gDeviceInfoWorkIdx ++;

	if (pdc < 17) {
		if (edt[0] != (pdc - 1)) {
			// mismatch
			return;
		}
		edt ++;
		pdc --;
		while (pdc > 0) {
			idx = *edt - 0x80;
			if ((idx & 1) == 0) {
				table->epc_map[idx / 2] |= flag;
			} else {
				table->epc_map[idx / 2] |= flag << 4;
			}
			edt ++;
			pdc --;
		}
	} else {
		// bitmap
		edt ++;
		pdc --;
		for (base = 0; base < 16; base ++) {
			bit = 0x01;
			for (cnt = 0; cnt < 8; cnt ++) {
				if ((*edt & bit) != 0) {
					idx = (cnt * 0x10) + base;
					if ((idx & 1) == 0) {
						table->epc_map[idx / 2] |= flag;
					} else {
						table->epc_map[idx / 2] |= flag << 4;
					}
				}
				bit <<= 1;
			}
			edt ++;
			pdc --;
		}
	}
}

//=============================================================================
static void ELL_HandleRecvProp(ELL_GW_RecvProp_t *recv_prop)
{
	assert(recv_prop != NULL);

	switch (gGatewayState) {
	case ELL_GW_STATE_INIT:
		if (recv_prop->epc == 0xd6 && recv_prop->pdc > 0) {
			uint8_t num_eoj;
			num_eoj = recv_prop->edt[0];
			if (recv_prop->pdc == (num_eoj * 3 + 1)) {
				uint8_t *edt_ptr = &recv_prop->edt[1];
				while (num_eoj > 0) {
					ELL_AddDeviceInfo(recv_prop->ipaddr, ELL_EOJ(edt_ptr));
					edt_ptr += 3;
					num_eoj --;
				}
			}
		}
		break;

	case ELL_GW_STATE_NODE_PROFILE_ID:
		if (recv_prop->epc == 0x83) {
			if (recv_prop->pdc == 17) {
				uint8_t dev_id[32] = {0};
				dev_id[0] = 0x04;
				dev_id[1] = 20;
				MEM_Copy(&dev_id[2], recv_prop->edt, 17);
#if 0
				dev_id[19] = (recv_prop->eoj >> 16) & 0xff;
				dev_id[20] = (recv_prop->eoj >> 8) & 0xff;
				dev_id[21] = recv_prop->eoj & 0xff;
#else
				ELL_DeviceInfo_t *info = &gDeviceInfoTable[gDeviceInfoWorkIdx];
				dev_id[19] = (info->eoj >> 16) & 0xff;
				dev_id[20] = (info->eoj >> 8) & 0xff;
				dev_id[21] = info->eoj & 0xff;
#endif
				ELL_AddDeviceID(dev_id);
			} else {
				gDeviceInfoWorkIdx ++;
				// failed...
			}
			if (!ELL_GetNodeProfileID()) {
				// next state
				gGatewayState = ELL_GW_STATE_OBJECT_ID;
				gDeviceInfoWorkIdx = 0;
				if (!ELL_GetObjectID()) {
					// failed...
				}
			}
		}
		break;

	case ELL_GW_STATE_OBJECT_ID:
		if (recv_prop->epc == 0x83) {
			if (recv_prop->pdc > 0) {
				if (recv_prop->pdc == 17) {
					uint8_t dev_id[32] = {0};
					dev_id[0] = 0x03;
					dev_id[1] = 17;
					MEM_Copy(&dev_id[2], recv_prop->edt, 17);
					ELL_AddDeviceID(dev_id);
				} else {
					gDeviceInfoWorkIdx ++;
					// failed...
				}
			} else {
				gDeviceInfoWorkIdx ++;
			}
			if (!ELL_GetObjectID()) {
				// next state
				gGatewayState = ELL_GW_STATE_PROP_LIST_0x9D;
				gDeviceInfoWorkIdx = 0;
				if (!ELL_GetProp(0x9d)) {
					// failed...
				}
			}
		}
		break;

	case ELL_GW_STATE_PROP_LIST_0x9D:
		if (recv_prop->epc == 0x9d) {
			if (recv_prop->pdc > 0) {
				ELL_AddPropList(recv_prop->pdc, recv_prop->edt, 0x04);
			} else {
				// failed...
				gDeviceInfoWorkIdx ++;
			}
			if (!ELL_GetProp(0x9d)) {
				// next state
				gGatewayState = ELL_GW_STATE_PROP_LIST_0x9E;
				gDeviceInfoWorkIdx = 0;
				if (!ELL_GetProp(0x9e)) {
					// failed...
				}
			}
		}
		break;

	case ELL_GW_STATE_PROP_LIST_0x9E:
		if (recv_prop->epc == 0x9e) {
			if (recv_prop->pdc > 0) {
				ELL_AddPropList(recv_prop->pdc, recv_prop->edt, 0x02);
			} else {
				// failed...
				gDeviceInfoWorkIdx ++;
			}
			if (!ELL_GetProp(0x9e)) {
				// next state
				gGatewayState = ELL_GW_STATE_PROP_LIST_0x9F;
				gDeviceInfoWorkIdx = 0;
				if (!ELL_GetProp(0x9f)) {
					// failed...
				}
			}
		}
		break;

	case ELL_GW_STATE_PROP_LIST_0x9F:
		if (recv_prop->epc == 0x9f) {
			if (recv_prop->pdc > 0) {
				ELL_AddPropList(recv_prop->pdc, recv_prop->edt, 0x01);
			} else {
				// failed...
				gDeviceInfoWorkIdx ++;
			}
			if (!ELL_GetProp(0x9f)) {
				// next state
				gGatewayState = ELL_GW_STATE_NORMAL;
				gDeviceInfoWorkIdx = 0;

#if 1 // local server
				uint32_t ipaddr = (192 << 24) | (168 << 16) | (11 << 8) | 3;
#else
				// slb-ftp.smk.co.jp (54.64.191.209)
				uint32_t ipaddr = (54 << 24) | (64 << 16) | (191 << 8) | 209;
#endif
				_task_create(0, TN_ELL_GATEWAY_SVR_TASK, ipaddr);
			}
		}
		break;

	case ELL_GW_STATE_NORMAL:
	{
		ELL_GatewayMsg_t *msg = ELL_AllocGWMsg(gELLGWSvrPoolID,
											   gELLGWSvrQID, 0);
		if (msg != NULL) {
			msg->msg_id = ELL_GW_MSG_ANNO_EPC;
			msg->param.anno_epc.ipaddr = recv_prop->ipaddr;
			msg->param.anno_epc.eoj = recv_prop->eoj;
			msg->param.anno_epc.epc = recv_prop->epc;
			msg->param.anno_epc.pdc = recv_prop->pdc;
			msg->param.anno_epc.edt = recv_prop->edt;

			if (_msgq_send(msg)) {
				recv_prop->edt = NULL; // to avoid _msg_free()
			} else {
				_msg_free(msg);
			}
		}
	}
		break;

	default:
		break;
	}
}

//=============================================================================
void ELL_GatewayApp_Task(uint32_t param)
{
	ELL_GatewayMsg_t *msg;

	// ------------------------------------------------ Create Message Pool ---
	gELLGWAppPoolID = _msgpool_create(sizeof(ELL_GatewayMsg_t), 20, 0, 0);
	if (gELLGWAppPoolID == MSGPOOL_NULL_POOL_ID) {
		err_printf("[ELL GW] Cannot Create Message Pool\n");
		_task_block();
	}

	// ----------------------------------------- Open Request Message Queue ---
	gELLGWAppQID = _msgq_open(ELL_GATEWAY_APP_MESSAGE_QUEUE, 0);
	if (gELLGWAppQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("[ELL GW] Cannot Open Request Message Queue\n");
		_task_block();
	}

	// -------------------------------------- Set Receive Property Callback ---
	ELL_SetRecvPropertyCallback(ELL_RecvProperty);

    msg_printf("\n[ELL GW] START\n");

	gGatewayState = ELL_GW_STATE_INIT;

	// ----------------------Multicast "Get 0xD6" to Node Profile(0x0EF001) ---
	multicastGetEPC(0x0ef001, 0xd6);
	_timer_start_oneshot_after(ELL_GetD6Timeout, NULL,
							   TIMER_ELAPSED_TIME_MODE, 5000);

	while (1) {
		msg = _msgq_receive(gELLGWAppQID, 0);
		if (msg == NULL) continue;

		switch (msg->msg_id) {
		case ELL_GW_MSG_RECV_PROP:
			ELL_HandleRecvProp(&msg->param.recv_prop);
			if (msg->param.recv_prop.edt != NULL) {
				_mem_free(msg->param.recv_prop.edt);
				msg->param.recv_prop.edt = NULL;
			}
			break;

		case ELL_GW_MSG_RECV_TIMEOUT:
			printf("Receive Finished\n");
			gGatewayState = ELL_GW_STATE_NODE_PROFILE_ID;
			gDeviceInfoWorkIdx = 0;
			if (!ELL_GetNodeProfileID()) {
				/*** FINISHED ***/
			}
			break;

		case ELL_GW_MSG_REFRESH:
			multicastGetEPC(0x0ef001, 0xd6);
			break;

		case ELL_GW_MSG_GET_EPC:
			getEPC(msg->param.get_epc.ipaddr,
				   msg->param.get_epc.eoj, msg->param.get_epc.epc);
			break;

		case ELL_GW_MSG_SET_EPC:
			setEPC(msg->param.set_epc.ipaddr,
				   msg->param.set_epc.eoj, msg->param.set_epc.epc,
				   msg->param.set_epc.pdc, msg->param.set_epc.edt);
			if (msg->param.set_epc.edt != NULL) {
				_mem_free(msg->param.set_epc.edt);
				msg->param.set_epc.edt = NULL;
			}
			break;

		default:
			break;
		}

		_msg_free(msg);
	}
}

//=============================================================================
#define WS_COMMAND_MAX_LEN 256
static uint8_t gWSCommand[WS_COMMAND_MAX_LEN];
static uint8_t gWSTID = 0;

//=============================================================================
static void send_cmd(WebSocket_t *websock, uint8_t cmd, uint8_t tid,
					 const uint8_t *frame_data, uint16_t len)
{
	int cnt, idx;
	uint8_t sum;

	if (len > (WS_COMMAND_MAX_LEN - 5)) return;

	if (frame_data == NULL) {
		len = 0;
	}

	gWSCommand[0] = cmd;
	gWSCommand[1] = tid;
	gWSCommand[2] = (len >> 8) & 0xff;
	gWSCommand[3] = len & 0xff;
	idx = 4;
	if (frame_data != NULL && len > 0) {
		memcpy(&gWSCommand[4], frame_data, len);
		idx += len;
	}
	sum = 0;
	for (cnt = 0; cnt < idx; cnt ++) {
		sum += gWSCommand[cnt];
	}
	gWSCommand[idx] = sum;
	idx ++;

	WS_SendBinaryFrame(websock, gWSCommand, idx);
}

//=============================================================================
static void incWSTID(void)
{
	gWSTID ++;
	if (gWSTID == 0) {
		gWSTID = 1;
	}
}

//=============================================================================
// Connection Start (=> Server)
//=============================================================================
static void cld_sendConnectionStart(WebSocket_t *websock)
{
	_enet_address mac;

	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
	send_cmd(websock, 0x10, gWSTID, (uint8_t *)mac, 6);
	incWSTID();
	_time_delay(1000);
}

//=============================================================================
// Notify Device List (=> Server)
//=============================================================================
static void cld_sendNotifyDeviceList(WebSocket_t *websock)
{
	uint32_t max;
	uint8_t *data;
	int cnt, idx;
	ELL_DeviceInfo_t *info;

	max = gDeviceInfoIdx * (32 + 2) + 1;
	data = _mem_alloc(max);
	if (data == NULL) {
		// ERROR
		return;
	}

	data[0] = gDeviceInfoIdx;
	idx = 1;
	for (cnt = 0; cnt < gDeviceInfoIdx; cnt ++) {
		info = &gDeviceInfoTable[cnt];

		// Device ID
		MEM_Copy(&data[idx], info->dev_id, devIdLen(info->dev_id));
		idx += devIdLen(info->dev_id);

		// Class ID
		data[idx] = (info->eoj >> 16) & 0xff;
		idx ++;
		data[idx] = (info->eoj >> 8) & 0xff;
		idx ++;
	}

	send_cmd(websock, 0x12, gWSTID, data, idx);
	incWSTID();

	_mem_free(data);

	_time_delay(1000);
}

//=============================================================================
// Domain Start (=> Server)
//=============================================================================
static void cld_sendDomainStart(WebSocket_t *websock)
{
	_enet_address mac;

	ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
	send_cmd(websock, 0x14, gWSTID, NULL, 0);
	incWSTID();
}

//=============================================================================
static void cld_sendNotifyProperty(WebSocket_t *websock, uint8_t dev_id[32],
								   uint8_t epc, uint8_t pdc, uint8_t *edt)
{
	uint16_t len;
	uint8_t *data;
	int idx;

	if (pdc == 0 || edt == NULL) return;

	len = devIdLen(dev_id) + 2 + pdc;
	data = _mem_alloc(len);
	if (data == NULL) return;

	MEM_Copy(&data[0], dev_id, devIdLen(dev_id));
	idx = devIdLen(dev_id);
	data[idx] = epc;
	idx ++;
	data[idx] = pdc;
	idx ++;
	MEM_Copy(&data[idx], edt, pdc);
	idx += pdc;
	assert(idx == len);

	send_cmd(websock, 0x24, gWSTID, data, len);
	incWSTID();

	_mem_free(data);
}

//=============================================================================
static bool_t getIPAddrAndEOJ(const uint8_t dev_id[32],
							  IPv4Addr_t *ipaddr, uint32_t *eoj)
{
	int cnt;
	ELL_DeviceInfo_t *info;
	int len;

	len = devIdLen(dev_id);
	for (cnt = 0; cnt < gDeviceInfoIdx; cnt ++) {
		info = &gDeviceInfoTable[cnt];
		if (info->dev_id[0] == dev_id[0]) {
			if (MEM_Compare(&info->dev_id, dev_id, len) == 0) {
				if (ipaddr != NULL) {
					*ipaddr = info->ipaddr;
				}
				if (eoj != NULL) {
					*eoj = info->eoj;
				}
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

//=============================================================================
static bool_t getDevID(IPv4Addr_t ipaddr, uint32_t eoj, uint8_t dev_id[32])
{
	int cnt;
	ELL_DeviceInfo_t *info;

	for (cnt = 0; cnt < gDeviceInfoIdx; cnt ++) {
		info = &gDeviceInfoTable[cnt];
		if (info->ipaddr == ipaddr && info->eoj == eoj) {
			MEM_Copy(dev_id, info->dev_id, devIdLen(info->dev_id));
			return (TRUE);
		}
	}
	return (FALSE);
}

//=============================================================================
static uint8_t get_next_epc(ELL_DeviceInfo_t *info, uint8_t epc)
{
	int cnt, idx;

	for (cnt = ((epc < 0x80) ? 0 : (epc - 0x80 + 1)); cnt < 128; cnt ++) {
		idx = cnt / 2;
		if ((cnt & 1) == 0) {
			if ((info->epc_map[idx] & 0x01) != 0) { // Get Flag
				return (cnt + 0x80);
			}
		} else {
			if ((info->epc_map[idx] & 0x10) != 0) { // Get Flag
				return (cnt + 0x80);
			}
		}
	}
	return (0);
}

//=============================================================================
static void ELL_GW_Reset(_timer_id id, void *param,
						 uint32_t unknown1, uint32_t unknown2)
{
	MQX_SWReset();
}

//=============================================================================
static void onOpen(WebSocket_t *websock, uint32_t status)
{
	printf("[ELL GW Svr] WebSocket Open (0x%X)\n", status);

	if (status != WS_STATUS_OK) {
		printf("[ELL GW Svr] Reset after 1min.\n");

		_timer_start_oneshot_after(ELL_GW_Reset, NULL, 
								   TIMER_ELAPSED_TIME_MODE, 1000 * 60);
	}
}

//=============================================================================
static void onClose(WebSocket_t *websock, uint32_t status)
{
	printf("[ELL GW Svr] WebSocket Close (0x%X)\n", status);

#if 0
	printf("[ELL GW Svr] Reset after 1min.\n");

	_timer_id id;
	id = _timer_start_oneshot_after(ELL_GW_Reset, NULL, 
									TIMER_ELAPSED_TIME_MODE, 1000 * 60);
	if (id == TIMER_NULL_ID) {
		printf("[ELL GW Svr] Launching Timer Failed\n");
	}
#else
	ELL_GatewayMsg_t *msg;

	msg = ELL_AllocGWMsg(gELLGWSvrPoolID, gELLGWSvrQID, 0);
	if (msg == NULL) return;

	msg->msg_id = ELL_GW_MSG_SERVER_STOP;

	if (!_msgq_send(msg)) {
		_msg_free(msg);
	}
#endif
}

//=============================================================================
static void onError(WebSocket_t *websock, uint32_t status)
{
	printf("[ELL GW Svr] WebSocket Error (0x%X)\n", status);
}

//=============================================================================
static void onReceive(WebSocket_t *websock,
					  uint8_t opcode, const uint8_t *buf, uint32_t len)
{
	int cnt;
	char *str;

	static uint8_t ret_work[10 + 2 + 256];

	printf("WebSocket Received (opcode=0x%X):\n", opcode);

	if (opcode == WS_OPCODE_TEXT) {
		str = _mem_alloc(len + 1);
		memcpy(str, buf, len);
		str[len] = '\0';
		printf("%s\n", str);
		_mem_free(str);
	}
	else if (opcode == WS_OPCODE_BINARY) {
		for (cnt = 0; cnt < len; cnt ++) {
			printf("0x%02X ", buf[cnt]);
			if (((cnt + 1) & 0xf) == 0) printf("\n");
		}
		if ((cnt & 0xf) != 0) printf("\n");

		if (len >= 5) {
			uint8_t cmd;
			uint8_t tid;
			uint16_t frm_len;
			const uint8_t *frm_data;

			uint32_t eoj;
			uint8_t epc;
			uint8_t pdc;
			uint8_t *edt;
			const uint8_t *dev_id;
			int dev_id_len;

			ELL_GatewayMsg_t *msg;

			cmd = buf[0];
			tid = buf[1];
			frm_len = ((uint16_t)buf[2] << 8) | (uint16_t)buf[3];
			if ((len - 5) == frm_len) {
				frm_data = &buf[4];
			}

			switch (cmd) {
			case 0x20:
				// Set Property Request

				dev_id = &frm_data[0];
				dev_id_len = devIdLen(dev_id);

				epc = frm_data[dev_id_len];
				pdc = frm_data[dev_id_len + 1];
				if (pdc > 0) {
					edt = _mem_alloc(pdc);
					if (edt == NULL) {
						return;
					}
					MEM_Copy(edt, &frm_data[dev_id_len + 2], pdc);
				} else {
					edt = NULL;
				}

				msg = ELL_AllocGWMsg(gELLGWAppPoolID, gELLGWAppQID, 0);
				if (msg == NULL) {
					if (edt != NULL) {
						_mem_free(edt);
					}
					return;
				}

				msg->msg_id = ELL_GW_MSG_SET_EPC;
				getIPAddrAndEOJ(dev_id, &msg->param.set_epc.ipaddr,
								&msg->param.set_epc.eoj);
				msg->param.set_epc.epc = epc;
				msg->param.set_epc.pdc = pdc;
				msg->param.set_epc.edt = edt;

				if (!_msgq_send(msg)) {
					if (edt != NULL) {
						_mem_free(edt);
					}
					_msg_free(msg);
				}
				break;

			case 0x22:
				// Get Property Request
				dev_id = &frm_data[0];
				dev_id_len = devIdLen(dev_id);

				epc = frm_data[dev_id_len];

				msg = ELL_AllocGWMsg(gELLGWAppPoolID, gELLGWAppQID, 0);
				if (msg == NULL) {
					if (edt != NULL) {
						_mem_free(edt);
					}
					return;
				}

				msg->msg_id = ELL_GW_MSG_GET_EPC;
				getIPAddrAndEOJ(dev_id, &msg->param.get_epc.ipaddr,
								&msg->param.get_epc.eoj);
				msg->param.get_epc.epc = epc;

				if (!_msgq_send(msg)) {
					_msg_free(msg);
				}
				break;
			default:
				break;
			}
		}
	}
}

//=============================================================================
static bool_t sendMsg2App_GetEPC(IPv4Addr_t ipaddr, uint32_t eoj, uint8_t epc)
{
	ELL_GatewayMsg_t *msg;

	msg = ELL_AllocGWMsg(gELLGWSvrPoolID, gELLGWAppQID, 0);
	if (msg == NULL) {
		return (FALSE);
	}

	msg->msg_id = ELL_GW_MSG_GET_EPC;
	msg->param.get_epc.ipaddr = ipaddr;
	msg->param.get_epc.eoj = eoj;
	msg->param.get_epc.epc = epc;

	if (!_msgq_send(msg)) {
		_msg_free(msg);
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
static uint32_t ELL_GetWSHostIPAddr(void)
{
	uint8_t buf[FLASHROM_SGWS_HOST_SIZE + 1];
	in_addr temp;
	HOSTENT_STRUCT_PTR dnsdata;

	if (EEPROM_Read(FLASHROM_SGWS_HOST_ID,
					buf, FLASHROM_SGWS_HOST_SIZE) == 0) {
		return (0);
	}
	buf[FLASHROM_SGWS_HOST_SIZE] = '\0';

	if (inet_pton(AF_INET, (char *)buf, &temp, sizeof(temp)) == RTCS_OK) {
		return ((uint32_t)temp.s_addr);
	}

	dnsdata = gethostbyname((char *)buf);
	if (dnsdata != NULL && *(uint32_t *)dnsdata->h_addr_list[0] != 0) {
		return (*(uint32_t *)dnsdata->h_addr_list[0]);
	}

	return (0);
}

//=============================================================================
static uint16_t ELL_GetWSHostPortNo(void)
{
	uint8_t buf[2];
	
	if (EEPROM_Read(FLASHROM_SGWS_PORTNO_ID, buf, 2) == 0) {
		return (0);
	}
	return (((uint16_t)buf[0] << 8) + (uint16_t)buf[1]);
}

//=============================================================================
static WebSocket_t *ELL_CreateWSHandle(void)
{
	WebSocket_t *websock;
	uint32_t ipaddr;
	static char path[FLASHROM_SGWS_PATH_SIZE + 1];
	uint16_t portno;

	// --------------------------------------------- WebSocket Host Address ---
	ipaddr = ELL_GetWSHostIPAddr();
	if (ipaddr == 0) {
		err_printf("[ELL GW Svr] Host Address Error\n");
		_task_block();
	}

	// ---------------------------------------------- WebSocket Host PortNo ---
	portno = ELL_GetWSHostPortNo();

	// ----------------------------------------------------- WebSocket Path ---
	if (EEPROM_Read(FLASHROM_SGWS_PATH_ID,
					path, FLASHROM_SGWS_PATH_SIZE) == 0) {
		err_printf("[ELL GW Svr] Host Address Error\n");
		_task_block();
	}
	path[FLASHROM_SGWS_PATH_SIZE] = '\0';

	msg_printf("[ELL GW Svr] Connect to %d.%d.%d.%d(:%d) %s\n",
			   (ipaddr >> 24) & 0xff, (ipaddr >> 16) & 0xff,
			   (ipaddr >> 8) & 0xff, ipaddr & 0xff, portno, path);

	// -------------------------------------------- Create WebSocket Handle ---
	websock = WS_CreateHandle(WS_SECURITY_TLS1,
							  ipaddr, portno, NULL, path, NULL);
	if (websock == NULL) {
		err_printf("[ELL GW Svr] Failed to Create WebSocket Handle\n");
		_task_block();
	}

	return (websock);
}

//=============================================================================
static void ELL_GW_KeepAlive(_timer_id id, void *param,
							 uint32_t unknown1, uint32_t unknown2)
{
	ELL_GatewayMsg_t *msg;

	msg = ELL_AllocGWMsg(gELLGWSvrPoolID, gELLGWSvrQID, 0);
	if (msg == NULL) return;

	msg->msg_id = ELL_GW_MSG_KEEP_ALIVE;

	if (!_msgq_send(msg)) {
		_msg_free(msg);
	}
}

//=============================================================================
void ELL_GatewaySvr_Task(uint32_t param)
{
	ELL_GatewayMsg_t *msg;
	uint32_t ipaddr = param;
	WebSocket_t *websock;

	// ------------------------------------------------ Create Message Pool ---
	gELLGWSvrPoolID = _msgpool_create(sizeof(ELL_GatewayMsg_t), 20, 0, 0);
	if (gELLGWSvrPoolID == MSGPOOL_NULL_POOL_ID) {
		err_printf("[ELL GW] Cannot Create Message Pool\n");
		_task_block();
	}

	// ----------------------------------------- Open Request Message Queue ---
	gELLGWSvrQID = _msgq_open(ELL_GATEWAY_SVR_MESSAGE_QUEUE, 0);
	if (gELLGWSvrQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("[ELL GW Svr] Cannot Open Request Message Queue\n");
		_task_block();
	}

	// -------------------------------------------- Create WebSocket Handle ---
	websock = ELL_CreateWSHandle();
	if (websock == NULL) {
		err_printf("[ELL GW Svr] Failed to Create WebSocket Handle\n");
		_task_block();
	}
	WS_OnOpen(websock, onOpen);
	WS_OnClose(websock, onClose);
	WS_OnError(websock, onError);
	WS_OnReceive(websock, onReceive);

	// ---------------------------------------------------- Start WebSocket ---
	if (!WS_Start(websock, 9)) {
		err_printf("[ELL GW Svr] Not Start WebSocket\n");
		_task_block();
	}

	gGatewaySvrState = ELL_GW_SVR_STATE_START;

	cld_sendConnectionStart(websock);

	cld_sendNotifyDeviceList(websock);

	int dev_idx = 0;
	int cur_epc = 0;
	ELL_DeviceInfo_t *info;

	if (dev_idx < gDeviceInfoIdx) {
		info = &gDeviceInfoTable[dev_idx];
		cur_epc = get_next_epc(info, cur_epc);
		if (cur_epc == 0) {
			_task_block();
		}
		sendMsg2App_GetEPC(info->ipaddr, info->eoj, cur_epc);
	}

	// --------------------------------------------------- Keep Alive Timer ---
	_timer_id keep_alive;
	keep_alive = _timer_start_periodic_every(ELL_GW_KeepAlive, NULL,
											 TIMER_ELAPSED_TIME_MODE,
											 1000 * 60);

	while (1) {
		msg = _msgq_receive(gELLGWSvrQID, 0);

		switch (msg->msg_id) {
		case ELL_GW_MSG_ANNO_EPC:
		{
			uint8_t dev_id[32];
			if (getDevID(msg->param.anno_epc.ipaddr,
						 msg->param.anno_epc.eoj, dev_id)) {
				cld_sendNotifyProperty(websock, dev_id,
									   msg->param.anno_epc.epc,
									   msg->param.anno_epc.pdc,
									   msg->param.anno_epc.edt);
			}
			if (msg->param.anno_epc.edt != NULL) {
				_mem_free(msg->param.anno_epc.edt);
				msg->param.anno_epc.edt = NULL;
			}

			if (gGatewaySvrState == ELL_GW_SVR_STATE_START) {
				cur_epc = get_next_epc(info, cur_epc);
				while (cur_epc == 0) {
					dev_idx ++;
					if (dev_idx < gDeviceInfoIdx) {
						info = &gDeviceInfoTable[dev_idx];
						cur_epc = get_next_epc(info, cur_epc);
					} else {
						gGatewaySvrState = ELL_GW_SVR_STATE_NORMAL;
						cld_sendDomainStart(websock);
						break;
					}
				}
				if (cur_epc > 0
					&& gGatewaySvrState == ELL_GW_SVR_STATE_START) {
					sendMsg2App_GetEPC(info->ipaddr, info->eoj, cur_epc);
				}
			}
		}
			break;

		case ELL_GW_MSG_KEEP_ALIVE:
			WS_SendPingFrame(websock, NULL, 0);
			break;

		case ELL_GW_MSG_SERVER_STOP:
		{
			printf("[ELL GW Svr] Reset after 1min.\n");

			_timer_id id;
			id = _timer_start_oneshot_after(ELL_GW_Reset, NULL, 
											TIMER_ELAPSED_TIME_MODE, 1000 * 60);
			if (id == TIMER_NULL_ID) {
				printf("[ELL GW Svr] Launching Timer Failed\n");
			}
		}
			break;

		default:
			break;
		}

		_msg_free(msg);
	}
}

#endif /* APP_ENL_CONTROLLER */

/******************************** END-OF-FILE ********************************/
