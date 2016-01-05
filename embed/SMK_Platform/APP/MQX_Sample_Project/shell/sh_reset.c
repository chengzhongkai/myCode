/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include "mqx_utils.h"

//=============================================================================
static void usage(void)
{
	printf("Usage: reset\n   Cause software reset\n");
}

//=============================================================================
int32_t Shell_reset(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);

	if (print_usage) {
		if (short_help) {
			printf("reset\n");
		} else {
			usage();
		}
		return (0);
	}

	MQX_SWReset();

	return (0);
}

/******************************** END-OF-FILE ********************************/
