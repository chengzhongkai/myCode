/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

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

	SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA) | SCB_AIRCR_SYSRESETREQ_MASK;

	_time_delay(1000);

	return (0);
}

/******************************** END-OF-FILE ********************************/
