/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <shell.h>

#include "Config.h"

#include "shell_command.h"

//=============================================================================
static const SHELL_COMMAND_STRUCT shell_commands[] = {
	{ "help",       Shell_help       },

	{ "ipconfig",   Shell_ipconfig   },
	{ "ping",       Shell_ping       },

#if APP_DEBUG
	{ "eeprom",     Shell_eeprom     },
	{ "debug",      Shell_debug      },
	{ "update",	    Shell_update     },
#endif

#if 0
	{ "enet_reg",   Shell_enet_reg   },
	{ "enet_reset", Shell_enet_reset },
#endif

	{ "version",    Shell_version    },
	{ "reset",      Shell_reset      },

#if APP_DEBUG
	{ "dns",        Shell_dns        },
	{ "info",       Shell_info       },
	{ "test",       Shell_test       },

	{ "cloud",      Shell_cloud      },

#if 0
	{ "echo",       Shell_echonet    },
#endif

	{ "mqtt",       Shell_mqtt       },
#endif

#if 0 // do not exit
	{ "exit", Shell_exit },
#endif

	{ NULL, NULL }
};

//=============================================================================
static void Console_Task(uint32_t param);

//=============================================================================
static _task_id gConsoleTaskID = 0;

//=============================================================================
static bool gStartShell = false;
bool Console_InShell(void)
{
	return (gStartShell);
}

//=============================================================================
uint32_t Console_StartTask(uint32_t priority)
{
	uint32_t org_err;
	uint32_t result;
	TASK_TEMPLATE_STRUCT template;

	org_err = _task_get_error();

	_mem_zero((uint8_t *)&template, sizeof(template));

	template.TASK_ADDRESS = Console_Task;
	template.TASK_STACKSIZE = 1024 * 16;
	template.TASK_PRIORITY = priority;
	template.TASK_NAME = "console";
	template.CREATION_PARAMETER = 0;

	gConsoleTaskID = _task_create(0, 0, (uint32_t)&template);
	if (gConsoleTaskID == MQX_NULL_TASK_ID) {
		err_printf("[Console] Creating Task Failed (0x%08x)\n",
				   _task_get_error());
		result = MQX_INVALID_TASK_ID;
	} else {
		result = MQX_OK;
	}

	_task_set_error(org_err);

	return (result);
}

//=============================================================================
static void Console_Task(uint32_t param)
{
	int ch;

	do {
		ch = getchar();
	} while (ch != '\n');

	gStartShell = true;

	Shell(shell_commands, NULL);
}

/******************************** END-OF-FILE ********************************/
