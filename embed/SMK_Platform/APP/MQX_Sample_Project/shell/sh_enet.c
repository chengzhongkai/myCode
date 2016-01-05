/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "enet.h"

//=============================================================================
int32_t Shell_enet_reg(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);

	if (print_usage) {
		if (short_help) {
			printf("enet_reg\n");
		} else {
			printf("enet_reg\n  dump lan8720ai registers.\n");
		}
		return (0);
	}

	_enet_handle handle;

	handle = ENET_get_device_handle(0);
	if (handle == NULL) {
		printf("no handle\n");
		return (0);
	}

	bool res;
	uint32_t reg;

	res = ENET_read_mii(handle, 0 /* LAN8720_REG_BMCR */, &reg, 1000);
	printf("Basic Control Register(0)\n");
	if (res) {
		printf(" 0x%04x\n", reg);
	} else {
		printf(" Timeout\n");
	}

	res = ENET_read_mii(handle, 1, &reg, 1000);
	printf("Basic Status Register(1)\n");
	if (res) {
		printf(" 0x%04x\n", reg);
	} else {
		printf(" Timeout\n");
	}

	res = ENET_read_mii(handle, 2, &reg, 1000);
	printf("PHY ID1 Register(2)\n");
	if (res) {
		printf(" 0x%04x\n", reg);
	} else {
		printf(" Timeout\n");
	}

	res = ENET_read_mii(handle, 3, &reg, 1000);
	printf("PHY ID2 Register(3)\n");
	if (res) {
		printf(" 0x%04x\n", reg);
	} else {
		printf(" Timeout\n");
	}

	res = ENET_read_mii(handle, 4, &reg, 1000);
	printf("Auto Negotiation Advertizement Register(4)\n");
	if (res) {
		printf(" 0x%04x\n", reg);
	} else {
		printf(" Timeout\n");
	}

	return (0);
}

//=============================================================================
int32_t Shell_enet_reset(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);

	if (print_usage) {
		if (short_help) {
			printf("enet_reset\n");
		} else {
			printf("enet_reset\n  soft reset lan8720ai.\n");
		}
		return (0);
	}

	_enet_handle handle;

	handle = ENET_get_device_handle(0);
	if (handle == NULL) {
		printf("no handle\n");
		return (0);
	}

	bool res;
	uint32_t reg;

	res = FALSE;
	do {
		res = ENET_read_mii(handle, 4, &reg, 1000);
		if (!res) break;

		// set bit 8, 7, 6, 5
		reg |= 0x01e0;
		res = ENET_write_mii(handle, 4, reg, 1000);
		if (!res) break;

		res = ENET_read_mii(handle, 4, &reg, 1000);
		if (res) {
			printf("Set Reg 4 to 0x%04x\n", reg);
		}

		res = ENET_read_mii(handle, 0, &reg, 1000);
		if (!res) break;

		// set bit 12, clear bit 10
		reg |= 0x1000;
		reg &= 0xfbff;
		res = ENET_write_mii(handle, 0, reg, 1000);
		if (!res) break;

		res = ENET_read_mii(handle, 0, &reg, 1000);
		if (res) {
			printf("Set Reg 0 to 0x%04x\n", reg);
		}

		// soft reset (set bit 15 only)
		res = ENET_write_mii(handle, 0, 0x8000, 1000);
	} while (0);

	if (!res) {
		printf("failed\n");
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
