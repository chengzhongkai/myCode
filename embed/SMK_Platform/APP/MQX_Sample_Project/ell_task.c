/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "ell_task.h"

#include "Config.h"
#include "Tasks.h"

#if defined(APP_ENL_ADAPTER) || defined(APP_HAx2_AS_SWITCH_CLASS) || defined(APP_ENL_CONTROLLER)

#if defined(APP_ENL_CONTROLLER)
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
}
#endif

//=============================================================================
static uint8_t gRecvBuffer[ELL_RECV_BUF_SIZE];
static UDP_Handle_t gUDP;
static LWSEM_STRUCT gUDPSendSem;

static LWSEM_STRUCT gELLRscSem;

static _pool_id gELLRecvPacketPoolID = 0;
static _queue_id gELLRecvPacketQID = 0;

static LWEVENT_STRUCT gELLTaskSyncEvent;

static bool_t gELLTaskStart = FALSE;

//=============================================================================
bool_t ELL_SendPacket(void *handle, void *dest, const uint8_t *buf, int len)
{
    bool_t ret;

    if (handle == NULL) return (FALSE);

    _lwsem_wait(&gUDPSendSem);

    if (dest == NULL) {
#ifdef DEBUG
        ELLDbg_PrintPacket("to", 0, buf, len);
#endif
        ret = UDP_Multicast(handle, buf, len);
    } else {
        IPv4Addr_t addr = *((IPv4Addr_t *)dest);
#ifdef DEBUG
        ELLDbg_PrintPacket("to", addr, buf, len);
#endif
        ret = UDP_Send(handle, addr, buf, len);
    }

    _lwsem_post(&gUDPSendSem);

    return (ret);
}

//=============================================================================
bool_t ELL_SendPacketEx(void *dest, const uint8_t *buf, int len)
{
	return (ELL_SendPacket(&gUDP, dest, buf, len));
}

//=============================================================================
static void ELL_Lock(void *data)
{
    _lwsem_wait((LWSEM_STRUCT *)data);
}

//=============================================================================
static void ELL_Unlock(void *data)
{
    _lwsem_post((LWSEM_STRUCT * )data);
}

//=============================================================================
void ELL_StartTask(uint32_t flag)
{
	_lwevent_set(&gELLTaskSyncEvent, flag);
}

//=============================================================================
void ELL_NotifyReset(void)
{
	if (gELLTaskStart) {
		_lwsem_wait(&gUDPSendSem);

		UDP_Reset(&gUDP);

		_lwsem_post(&gUDPSendSem);

		ELL_StartUp(&gUDP);
	}
}

//=============================================================================
void ELL_Task(uint32_t param)
{
    int recv_size;
    IPv4Addr_t recv_addr;

	ELL_RecvPacketMsg_t *msg;

    _lwsem_create(&gUDPSendSem, 1);

    _lwsem_create(&gELLRscSem, 1);

	if (_lwevent_create(&gELLTaskSyncEvent, 0) != MQX_OK) {
		err_printf("[ENL] failed to create event\n");
		_task_block();
	}

	// -------------------------------------- Init ECHONET Lite Application ---
	if (!ELL_InitApp()) {
		err_printf("[ENL] failed to init app\n");
		_task_block();
	}

    // --------------------------------------------- Init ECHONET Lite Lite ---
    if (!ELL_ConstructObjects()) {
		err_printf("[ENL] failed to construct objects\n");
        _task_block();
    }

    // ---------------------------------------------------- Open Connection ---
    IPCFG_IP_ADDRESS_DATA ip_data;
    ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &ip_data);
    char prn_addr[RTCS_IP4_ADDR_STR_SIZE];
    inet_ntop(AF_INET, &ip_data.ip, prn_addr, sizeof(prn_addr));
    if (!UDP_Open(prn_addr, "224.0.23.0", 3610, &gUDP)) {
        err_printf("Cannot open %s\n", prn_addr);
        _task_block();
    }
    ELL_SetSendPacketCallback(ELL_SendPacket);

#if 0 // for TEST
	ELL_SetRecvPropertyCallback(ELL_RecvProperty);
#endif

    ELL_SetLockFunc(ELL_Lock, ELL_Unlock, &gELLRscSem);

    // _time_delay(1000);

	// -------------------------------------------------------- Start Tasks ---
	_task_create(0, TN_ELL_PKT_TASK, 0);
    _task_create(0, TN_ELL_INF_TASK, 0);
    _task_create(0, TN_ELL_ADP_CTRL_TASK, 0);
    _task_create(0, TN_ELL_ADP_SYNC_TASK, 0);

	// ------------------------------------------- Wait for All Tasks Ready ---
	if (_lwevent_wait_for(&gELLTaskSyncEvent,
						  ELL_TASK_START_ALL, TRUE, NULL) != MQX_OK) {
		_task_block();
	}

    msg_printf("\n[ELL RCV] START\n");

	gELLTaskStart = TRUE;

#if defined(APP_ENL_CONTROLLER)
	_task_create(0, TN_ELL_GATEWAY_APP_TASK, 0);
#endif

    // ---------------------------------------------------------- Main Loop ---
    while (1) {
        recv_size = UDP_Recv(&gUDP,
							 gRecvBuffer, ELL_RECV_BUF_SIZE, 0, &recv_addr);
        if (recv_size > 0) {
			msg = _msg_alloc(gELLRecvPacketPoolID);
			// msg = _msg_alloc_system(sizeof(ELL_RecvPacketMsg_t));
			if (msg != NULL) {
				msg->header.SOURCE_QID = 0;
				msg->header.TARGET_QID = gELLRecvPacketQID;
				msg->header.SIZE = sizeof(ELL_RecvPacketMsg_t);

				msg->buf = _mem_alloc_system(recv_size);
				if (msg->buf != NULL) {
					MEM_Copy(msg->buf, gRecvBuffer, recv_size);
					msg->size = recv_size;
					msg->addr = recv_addr;

					if (!_msgq_send(msg)) {
						err_printf("[ELL RCV] Cannot Send Message\n");
						_mem_free(msg->buf);
						_msg_free(msg);
					}
				} else {
					err_printf("[ELL RCV] Cannot Alloc Buffer\n");
					_msg_free(msg);
				}
			} else {
				err_printf("[ELL RCV] Cannot Alloc Message\n");
			}
        }
    }
}

//=============================================================================
// ECHOET Lite Packet Handler Task
//=============================================================================
void ELL_Handler_Task(uint32_t param)
{
	ELL_RecvPacketMsg_t *msg;

	// ------------------------------------------------ Create Message Pool ---
	gELLRecvPacketPoolID = _msgpool_create(sizeof(ELL_RecvPacketMsg_t),
										   ELL_MAX_RECV_PACKET_QUEUE_SIZE,
										   0, 0);
	if (gELLRecvPacketPoolID == MSGPOOL_NULL_POOL_ID) {
		err_printf("[ELL PKT] Cannot Create Message Pool\n");
		_task_block();
	}

	// ------------------------------------------------- Open Message Queue ---
	gELLRecvPacketQID = _msgq_open(ELL_RECV_PACKET_QUEUE,
								   ELL_MAX_RECV_PACKET_QUEUE_SIZE);
	if (gELLRecvPacketQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("[ELL PKT] Cannot Open Message Queue\n");
		_task_block();
	}

	ELL_StartTask(ELL_TASK_START_PKT);

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		msg = _msgq_receive(gELLRecvPacketQID, 0);
		if (msg != NULL) {
			if (msg->buf != NULL) {
#ifdef DEBUG
				ELLDbg_PrintPacket("from", msg->addr, msg->buf, msg->size);
#endif

				ELL_HandleRecvPacket(msg->buf, msg->size, &gUDP, &msg->addr);

				if (_mem_free(msg->buf) != MQX_OK) {
					err_printf("[ELL PKT] Free Memory Failed\n");
				}
			}
			_msg_free(msg);
		}
	}
}

//=============================================================================
// ECHONET Lite Check INF Property Task
//=============================================================================
void ELL_INF_Task(uint32_t param)
{
	ELL_StartTask(ELL_TASK_START_INF);

    // --------------------------------------------------- Initial Announce ---
    ELL_StartUp(&gUDP);

    while (1) {
        _time_delay(500);
        ELL_CheckAnnounce(&gUDP);
    }
}

#endif /* APP_ENL_ADAPTER */

/******************************** END-OF-FILE ********************************/
