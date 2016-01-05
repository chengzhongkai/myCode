/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "flash_utils.h"
#include "version.h"

#include "Config.h"

//=============================================================================
static void usage(void)
{
	printf("Usage: version\n   Print version information\n");
}

//=============================================================================
int32_t Shell_version(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("version\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 1) {
		usage();
		return (0);
	}

	// ------------------------------------------ Read & Print Machine Code ---
	char machine_id[FLASHROM_MACHINE_SIZE + 1];

	if (EEPROM_Read(FLASHROM_MACHINE_ID,
					machine_id, FLASHROM_MACHINE_SIZE) == 0) {
		puts("fail to read machine code data.\n");
		return (0);
	}

	int cnt;
	for (cnt = 0; cnt < FLASHROM_MACHINE_SIZE; cnt ++) {
		if (machine_id[cnt] >= 0x20 && machine_id[cnt] < 0x80) {
			putchar(machine_id[cnt]);
		}
		if (cnt == 6) {
			putchar('-');
		}
	}
	putchar('\n');

	printf(APP_FW_NAME "\n");

	// ------------------------------------------------- Print Version Info ---
	VERSION_DATA *data = (VERSION_DATA *)&stSoftwareVersion;
	printf("Version: %x.%02x\n", data->bMajorVersion, data->bMinorVersion);
	printf("Build Date: %x-%02x-%02x\n",
		   data->wYear, data->bMonth, data->bDate);

	return (0);
}

/******************************** END-OF-FILE ********************************/
