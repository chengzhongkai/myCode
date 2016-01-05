/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "Config.h"

//=============================================================================
static void usage(void)
{
	printf("Usage: info\n   Show firmware information\n");
}

//=============================================================================
int32_t Shell_info(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("info\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 1) {
		usage();
		return (0);
	}

	printf(APP_MAIN_NAME "\n");
	printf(APP_SUB_NAME "\n");
	printf(APP_FW_NAME "\n");

	return (0);
}

/******************************** END-OF-FILE ********************************/
