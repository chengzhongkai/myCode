/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>
#include <ipcfg.h>

#include "Config.h"

#include "websocket.h"
#include "echonetlitelite.h"

WebSocket_t *gWebsock = NULL;
#define WS_COMMAND_MAX_LEN 256
static uint8_t gWSCommand[WS_COMMAND_MAX_LEN];

static void send_cmd(WebSocket_t *websock, uint8_t cmd, uint8_t tid,
					 const uint8_t *frame_data, uint16_t len);

//=============================================================================
static void onOpen(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Open (0x%X)\n", status);
}

//=============================================================================
static void onClose(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Close (0x%X)\n", status);
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
	} else {
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
			const uint8_t *edt;

			cmd = buf[0];
			tid = buf[1];
			frm_len = ((uint16_t)buf[2] << 8) | (uint16_t)buf[3];
			if ((len - 5) == frm_len) {
				frm_data = &buf[4];
			}

			switch (cmd) {
			case 0x20:
				// Set Property Request
				eoj = (uint32_t)frm_data[7];
				eoj <<= 8;
				eoj |= (uint32_t)frm_data[8];
				eoj <<= 8;
				eoj |= (uint32_t)frm_data[9];
				epc = frm_data[10];
				pdc = frm_data[11];
				if (pdc > 0) {
					edt = &frm_data[12];
				} else {
					edt = NULL;
				}
				ELL_SetPropertyVirtual(ELL_FindObject(eoj), epc, pdc, edt);
				break;
			case 0x22:
				// Get Property Request
				eoj = (uint32_t)frm_data[7];
				eoj <<= 8;
				eoj |= (uint32_t)frm_data[8];
				eoj <<= 8;
				eoj |= (uint32_t)frm_data[9];
				epc = frm_data[10];
				pdc = ELL_GetPropertyVirtual(ELL_FindObject(eoj),
											 epc, &ret_work[12], 256);
				memcpy(ret_work, frm_data, 10);
				ret_work[10] = epc;
				ret_work[11] = pdc;
				send_cmd(websock, 0x23, tid, ret_work, pdc + 12);
				break;
			default:
				break;
			}
		}
	}
}

//=============================================================================
static void onError(WebSocket_t *websock, uint32_t status)
{
	printf("WebSocket Error (0x%X)\n", status);
}


//=============================================================================
static void usage(void)
{
	printf("Usage: cloud [ip addr]\n   Connect to Cloud Server\n");
}

//=============================================================================
_ip_address parse_ip(char *str)
{
	int idx;
	_ip_address ip_addr = 0;
	uint32_t addr;
	char *anchor;

	idx = 0;
	addr = 0;
	for (; *str != '\0'; str ++) {
		if (*str >= '0' && *str <= '9') {
			addr = addr * 10 + (*str - '0');
		} else if (*str == '.') {
			if (addr < 256) {
				ip_addr = (ip_addr << 8) | addr;
			} else {
				return (0);
			}
			addr = 0;
			idx ++;
			if (idx >= 4) return (0);
		} else {
			return (0);
		}
	}
	if (idx < 3) return (0);

	if (addr < 256) {
		ip_addr = (ip_addr << 8) | addr;
	} else {
		return (0);
	}

	return (ip_addr);
}

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
static void notifyAnnoEPC(uint32_t eoj, uint8_t *epc_list, int len)
{
	int cnt;
	uint8_t pdc;
	static uint8_t work[12 + 256];
	_enet_address mac;

	if (gWebsock == NULL) return;

	if (len > 0) {
		ELL_Object_t *obj;

		obj = ELL_FindObject(eoj);
		if (obj != NULL) {
			ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
			for (cnt = 0; cnt < len; cnt ++) {
				pdc = ELL_GetProperty(obj, epc_list[cnt], &work[12], 256);
				if (pdc > 0) {
					work[0] = 0x09;
					memcpy(&work[1], (uint8_t *)mac, 6);
					work[7] = (eoj >> 16) & 0xff;
					work[8] = (eoj >> 8) & 0xff;
					work[9] = (eoj) & 0xff;
					work[10] = epc_list[cnt];
					work[11] = pdc;
					send_cmd(gWebsock, 0x24, 0x00, work, pdc + 12);
				}
			}
		}
	}
}

//=============================================================================
int32_t Shell_cloud(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("cloud\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 2) {
		usage();
		return (0);
	}

	uint32_t ipaddr = parse_ip(argv[1]);

	if (gWebsock == NULL) {
		ELL_SetNotifyAnnoEPCCallback(notifyAnnoEPC);

		gWebsock = WS_CreateHandle(WS_SECURITY_TLS1,
								   ipaddr, 0, NULL, "/websocket", NULL);
		if (gWebsock != NULL) {
			WS_OnOpen(gWebsock, onOpen);
			WS_OnClose(gWebsock, onClose);
			WS_OnReceive(gWebsock, onReceive);
			WS_OnError(gWebsock, onError);

			if (WS_Start(gWebsock, 9)) {
				// -------------------------------- Client Connection Start ---
				_enet_address mac;
				// -------------------------------------- Send 0x10 Command ---
				ipcfg_get_mac(BSP_DEFAULT_ENET_DEVICE, mac);
				send_cmd(gWebsock, 0x10, 0x00, (uint8_t *)mac, 6);
				_time_delay(1000);

				// node profile
				ELL_Object_t *obj;
				uint8_t edt[32];
				uint8_t pdc;
				obj = ELL_FindObject(0x0ef001);
				if (obj != NULL) {
					pdc = ELL_GetProperty(obj, 0xd6, edt, 32);
					if (pdc > 0) {
						int num_obj;
						int s_idx, d_idx, cnt;
						uint8_t dev_list[64];
						uint32_t eoj;
						ELL_Object_t *dev_obj;

						num_obj = edt[0];
						dev_list[0] = num_obj;
						s_idx = 1;
						d_idx = 1;
						for (cnt = 0; cnt < num_obj; cnt ++) {
							dev_list[d_idx ++] = 0x09;
							memcpy(&dev_list[d_idx], (uint8_t *)mac, 6);
							d_idx += 6;
							memcpy(&dev_list[d_idx], &edt[s_idx], 3);
							d_idx += 3;
							s_idx += 3;
						}
						// ------------------------------ Send 0x12 Command ---
						send_cmd(gWebsock, 0x12, 0x01, dev_list, d_idx);
						_time_delay(1000);

						// notify all property
						s_idx = 1;
						d_idx = 1;
						for (cnt = 0; cnt < num_obj; cnt ++) {
							eoj = (uint32_t)edt[s_idx ++];
							eoj <<= 8;
							eoj |= (uint32_t)edt[s_idx ++];
							eoj <<= 8;
							eoj |= (uint32_t)edt[s_idx ++];

							dev_obj = ELL_FindObject(eoj);
							if (dev_obj != NULL) {
								uint8_t epc_list[17];
								int cnt2;
								pdc = ELL_GetProperty(dev_obj,
													  0x9f, epc_list, 17);
								assert(pdc > 0);
								for (cnt2 = 0; cnt2 < epc_list[0]; cnt2 ++) {
									uint8_t prop[32];
									pdc = ELL_GetProperty(dev_obj,
														  epc_list[cnt2 + 1],
														  prop, 32);
									assert(pdc > 0);

									uint8_t notify_prop[64];
									memcpy(notify_prop, &dev_list[d_idx], 10);
									notify_prop[10] = epc_list[cnt2 + 1];
									notify_prop[11] = pdc;
									memcpy(&notify_prop[12], prop, pdc);
									// ------------------ Send 0x24 Command ---
									send_cmd(gWebsock, 0x24, 0x00,
											 notify_prop, 12 + pdc);
								}
							}
							d_idx += 10;
						}
					}
				}
				
			} else {
				WS_ReleaseHandle(gWebsock);
				gWebsock = NULL;
			}
		}
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
