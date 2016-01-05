/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include <string.h>
#include <stdarg.h>

#include "storage_api.h"
#include "flash_utils.h"

//=============================================================================
static void usage(void)
{
	printf("Usage: debug <target>\n"
		   "  Targets:\n"
		   "    flash\n\tFlash Test Sequence\n"
		   "    eeprom\n\tEEPROM Test Sequence\n");
}

//=============================================================================
#define sh_debug_msg(msg) printf("%d: %s\n", __LINE__, (msg))

#define check(src) do {	\
	if ((src)) {				\
		printf("OK\n");			\
	} else {					\
		printf("FAILED\n");		\
	}							\
} while (0)

#define check_uint32(src, dest) do {	\
	uint32_t res;				\
	printf(#src " => ");		\
	res = (src);				\
	printf("%d ...", res);		\
	if (res == (dest)) {		\
		printf("OK\n");			\
	} else {					\
		printf("FAILED\n");		\
	}							\
} while (0)
#define check_bool(src, dest) do {	\
	bool res;					\
	printf(#src " => ");		\
	res = (src);				\
	if (res) {					\
		printf("true ...");		\
	} else {					\
		printf("false ...");	\
	}							\
	if (res == (dest)) {		\
		printf("OK\n");			\
	} else {					\
		printf("FAILED\n");		\
	}							\
} while (0)
#define check_ptr(src, dest) do {	\
	void *res;					\
	printf(#src " => ");		\
	res = (void *)(src);		\
	if (res == NULL) {			\
		printf("NULL ...");		\
	} else {					\
		printf("0x%08X ...", res);	\
	}							\
	if (res == (dest)) {		\
		printf("OK\n");			\
	} else {					\
		printf("FAILED\n");		\
	}							\
} while (0)

//=============================================================================
static uint32_t test_flash(MQX_FILE *fd, uint32_t addr, uint32_t size)
{
	uint8_t *buf;
	uint32_t cnt;
	uint32_t res_size;

	printf("Read/Write (addr=0x%08X, size=%d)\n", addr, size);

	buf = _mem_alloc(size);
	if (buf == NULL) {
		return (1);
	}

	for (cnt = 0; cnt < size; cnt ++) {
		buf[cnt] = (cnt & 0xff);
	}

	res_size = Storage_Write(fd, addr, size, buf);
	if (res_size != size) {
		printf("Write Failed...");
		_mem_free(buf);
		return (2);
	}

	memset(buf, 0, size);
	res_size = Storage_Read(fd, addr, size, buf);
	if (res_size != size) {
		printf("Read Failed...");
		_mem_free(buf);
		return (3);
	}

	for (cnt = 0; cnt < size; cnt ++) {
		if (buf[cnt] != (cnt & 0xff)) break;
	}
	if (cnt != size) {
		printf("Verify Failed...");
		_mem_free(buf);
		return (4);
	}

	return (0);
}

//=============================================================================
bool test_flash_empty(MQX_FILE *fd, uint32_t addr, uint32_t size)
{
	bool res;

	printf("Check Empty (addr=0x%08X, size=%d)\n", addr, size);

	res = Storage_IsEmpty(fd, addr, size);
	if (res) {
		printf("Empty...");
	} else {
		printf("Not Empty...");
	}

	return (res);
}

//=============================================================================
bool test_eeprom(uint32_t id, uint32_t size)
{
	uint8_t *backup = NULL;
	uint8_t *buf;
	uint32_t org_size;
	int cnt;
	uint32_t res_size;
	bool verify = true;

	printf("EEPROM Read/Write (id=%d, size=%d) ...", id, size);

	org_size = EEPROM_Size(id);
	if (org_size > 0) {
		backup = _mem_alloc(org_size);
		if (backup != NULL) {
			if (EEPROM_Read(id, backup, org_size) != org_size) {
				_mem_free(backup);
				backup = NULL;
			}
		}
	}

	buf = _mem_alloc(size);
	if (buf == NULL) return (false);

	for (cnt = 0; cnt < size; cnt ++) {
		buf[cnt] = (cnt & 0xff);
	}

	res_size = EEPROM_Write(id, buf, size);
	if (res_size != size) {
		printf("Write Failed ...");
		verify = false;
	}

	res_size = EEPROM_Read(id, buf, size);
	if (res_size != size) {
		printf("Read Failed ...");
		verify = false;
	}

	if (verify) {
		for (cnt = 0; cnt < size; cnt ++) {
			if (buf[cnt] != (cnt & 0xff)) break;
		}
		if (cnt != size) {
			printf("Verify Failed ...");
		}
	}

	if (backup != NULL) {
		EEPROM_Write(id, backup, org_size);
		_mem_free(backup);
	}

	_mem_free(buf);

	return (true);
}

//=============================================================================
int32_t Shell_debug(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("debug [flash]\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 2) {
		usage();
		return (0);
	}

	if (strcmp(argv[1], "flash") == 0) {
		// -------------------------------------------- Flash Test Sequence ---
		MQX_FILE *fd;
		int sec;

		printf("Open Storage\n");
		fd = Storage_Open();
		if (fd == NULL) {
			sh_debug_msg("Storage Open Failed");
			return (0);
		}

		printf("Erase All Sectors (it takes several seconds...)\n");
		if (!Storage_EraseSector(fd, 0, 15)) {
			sh_debug_msg("Erase All Sector Failed");
			goto debug_flash_end;
		}

		// Page Size = 256
		// Sector Size = 64KB

		if (!test_flash_empty(fd, 0x00000000, 4096)) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		/*** the top of sector and page ***/
		if (test_flash(fd, 0x00000000, 4096) != 0) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		if (test_flash_empty(fd, 0x00000000, 4096)) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		if (test_flash_empty(fd, 0x00000000 + 2048, 4096)) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		/*** the top of page, but not the top of sector ***/
		/*** the last area of NOR Flash ***/
		if (test_flash(fd, 0x00100000 - 4096, 4096) != 0) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		/*** across the sectors ***/
		if (test_flash(fd, 0x00010000 - 2048, 4096) != 0) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		/*** start address is not the top of page ***/
		if (test_flash(fd, 0x00020080 - 2048, 4096) == 0) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}

		/*** Write data to already written area ***/
		if (test_flash(fd, 0x00000000, 4096) == 0) {
			printf("FAILED\n");
		} else {
			printf("OK\n");
		}


		/*** Illegal Parameter Test ***/
		printf("\nIllegal Parameter Test\n");

		printf("test Storage_Close(NULL);\n");
		Storage_Close(NULL);

		uint8_t *buf;
		buf = _mem_alloc(4096);

		printf("test Storage_Read();\n");
		check_uint32(Storage_Read(NULL, 0x00000000, 4096, buf), 0);
		check_uint32(Storage_Read(fd, 0x00100000, 4096, buf), 0);
		check_uint32(Storage_Read(fd, 0x00000000, 0x00100001, buf), 0);
		check_uint32(Storage_Read(fd, 0x000f0000, 0x00010001, buf), 0);
		check_uint32(Storage_Read(fd, 0x00000000, 4096, NULL), 0);

		printf("test Storage_Write();\n");
		check_uint32(Storage_Write(NULL, 0x00000000, 4096, buf), 0);
		check_uint32(Storage_Write(fd, 0x00100000, 4096, buf), 0);
		check_uint32(Storage_Write(fd, 0x00000000, 0x00100001, buf), 0);
		check_uint32(Storage_Write(fd, 0x000f0000, 0x00010001, buf), 0);
		check_uint32(Storage_Write(fd, 0x00000000, 4096, NULL), 0);

		_mem_free(buf);

		printf("test Storage_EraseSector();\n");
		check_bool(Storage_EraseSector(NULL, 0, 0), false);
		check_bool(Storage_EraseSector(fd, 16, 17), false);
		check_bool(Storage_EraseSector(fd, 10, 16), false);
		check_bool(Storage_EraseSector(fd, 2, 1), false);

		printf("test Storage_IsEmpty();\n");
		check_bool(Storage_IsEmpty(NULL, 0x00000000, 4096), false);
		check_bool(Storage_IsEmpty(fd, 0x00100000, 4096), false);
		check_bool(Storage_IsEmpty(fd, 0x00000000, 0x00100001), false);
		check_bool(Storage_IsEmpty(fd, 0x000f0000, 0x00010001), false);

#if 0
		printf("fd = 0x%08X\n", fd);
		check_ptr(Storage_Open(), NULL);
#endif

		printf("\nAt last, Erase Sector by Sector (it takes several seconds...)\n");
		for (sec = 0; sec < 16; sec ++) {
			printf("Erase Sector %d\n", sec);
			if (!Storage_EraseSector(fd, sec, sec)) {
				sh_debug_msg("Erase All Sector Failed");
				goto debug_flash_end;
			}
		}

	debug_flash_end:
		printf("Close Storage\n");
		Storage_Close(fd);

		printf("Finished.\n");
	}
	else if (strcmp(argv[1], "eeprom") == 0) {
		// ------------------------------------------- EEPROM Test Sequence ---
		uint8_t buf[32] = {0};

		check(test_eeprom(FLASHROM_MACADDR_ID, FLASHROM_MACADDR_SIZE));
		check(test_eeprom(FLASHROM_MACHINE_ID, FLASHROM_MACHINE_SIZE));
		check(test_eeprom(FLASHROM_SERIAL_ID, FLASHROM_SERIAL_SIZE));
		check(test_eeprom(FLASHROM_FTPID_ID, FLASHROM_FTPID_SIZE));
		check(test_eeprom(FLASHROM_SUBDOMAIN_ID, FLASHROM_SUBDOMAIN_SIZE));
		check(test_eeprom(FLASHROM_PATH_ID, FLASHROM_PATH_SIZE));
		check(test_eeprom(FLASHROM_DHCP_ID, FLASHROM_DHCP_SIZE));
		check(test_eeprom(FLASHROM_STATICIP_ID, FLASHROM_STATICIP_SIZE));
		check(test_eeprom(FLASHROM_SUBNET_ID, FLASHROM_SUBNET_SIZE));
		check(test_eeprom(FLASHROM_GATEWAY_ID, FLASHROM_GATEWAY_SIZE));


		printf("\nIllegal Parameter Test\n");
		check_uint32(EEPROM_Read(FLASHROM_MACADDR_ID, buf, 0), 0);
		check_uint32(EEPROM_Read(FLASHROM_MACADDR_ID, NULL, 6), 0);
		check_uint32(EEPROM_Read(11, buf, 1), 0);
		check_uint32(EEPROM_Read(FLASHROM_MACADDR_ID, buf, 7), 6);
		check_uint32(EEPROM_Read(FLASHROM_MACADDR_ID, buf, 5), 5);

		uint8_t mac_addr[6];
		EEPROM_Read(FLASHROM_MACADDR_ID, mac_addr, 6);

		check_uint32(EEPROM_Write(FLASHROM_MACADDR_ID, buf, 0), 0);
		check_uint32(EEPROM_Write(FLASHROM_MACADDR_ID, NULL, 6), 0);
		check_uint32(EEPROM_Write(11, buf, 1), 0);
		check_uint32(EEPROM_Write(FLASHROM_MACADDR_ID, buf, 7), 6);
		check_uint32(EEPROM_Write(FLASHROM_MACADDR_ID, buf, 5), 5);

		EEPROM_Write(FLASHROM_MACADDR_ID, mac_addr, 6);
	}
	else {
		usage();
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
