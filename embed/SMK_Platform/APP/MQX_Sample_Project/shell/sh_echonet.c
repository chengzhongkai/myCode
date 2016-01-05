/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "echonetlitelite.h"

#include "rand.h"

extern _ip_address parse_ip(char *str);

extern bool_t ELL_SendPacketEx(void *dest, const uint8_t *buf, int len);

static uint8_t gSHSendBuffer[1024];

//=============================================================================
static void usage(void)
{
	printf("Usage: echo [IP Address] [EOJ] [ESV] [EPC] ([EDT])\n"
		   "  ECHONET Lite Command\n"
		   "    ESV is 'get' or 'set'\n"
		   "    EPC takes 80 to FF (described as hex.)\n"
		   "    EDT would be omitted when ESV is 'get'\n");
}

//=============================================================================
#define _hex(ch) (((ch) >= '0' && (ch) <= '9') ? ((ch) - '0') : (((ch) >= 'a' && (ch) <= 'f') ? ((ch) - 'a' + 10) : (((ch) >= 'A' && (ch) <= 'F') ? ((ch) - 'A' + 10) : 0)))

//=============================================================================
static uint8_t hex2byte(char *str)
{
	uint8_t ret;

	if (strlen(str) != 2) return (0);

	ret = _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);

	return (ret);
}

//=============================================================================
static uint32_t hex2eoj(char *str)
{
	uint32_t ret;

	if (strlen(str) != 6) return (0);

	ret = _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);
	str ++;
	ret <<= 4;
	ret |= _hex(*str);

	return (ret);
}

//=============================================================================
int32_t Shell_echonet(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("echo\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc < 5) {
		usage();
		return (0);
	}

	bool_t multicast;
	uint32_t ipaddr;
	if (*argv[1] == '*') {
		multicast = TRUE;
		ipaddr = 0;
	} else {
		multicast = FALSE;
		ipaddr = parse_ip(argv[1]);
	}

	uint32_t deoj = hex2eoj(argv[2]);

	uint8_t esv;
	if (strcasecmp(argv[3], "get") == 0) {
		esv = 0x62;
	}
	else if (strcasecmp(argv[3], "set") == 0) {
		esv = 0x61;
	}
	else {
		usage();
		return (0);
	}

	ELL_Property_t prop_list[2];
	uint8_t edt[256];

	prop_list[0].epc = hex2byte(argv[4]);
	prop_list[0].pdc = argc - 5;
	prop_list[0].edt = edt;
	prop_list[1].epc = 0;

	for (int cnt = 0; cnt < (argc - 5); cnt ++) {
		edt[cnt] = hex2byte(argv[cnt + 5]);
	}

	int send_size = ELL_MakePacket((uint16_t)FSL_GetRandom(),
								   0x05ff01, deoj, esv, prop_list,
								   gSHSendBuffer, sizeof(gSHSendBuffer));

	if (multicast) {
        ELLDbg_PrintPacket("to", 0, gSHSendBuffer, send_size);
		ELL_SendPacketEx(NULL, gSHSendBuffer, send_size);
	} else {
        ELLDbg_PrintPacket("to", ipaddr, gSHSendBuffer, send_size);
		ELL_SendPacketEx(&ipaddr, gSHSendBuffer, send_size);
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
