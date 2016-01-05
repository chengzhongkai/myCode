/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _SHELL_COMMAND_H
#define _SHELL_COMMAND_H

#include <mqx.h>
#include <shell.h>

extern int32_t Shell_enet_reg(int32_t argc, char *argv[]);
extern int32_t Shell_enet_reset(int32_t argc, char *argv[]);

extern int32_t Shell_eeprom(int32_t argc, char *argv[]);
extern int32_t Shell_update(int32_t argc, char *argv[]);
extern int32_t Shell_version(int32_t argc, char *argv[]);
extern int32_t Shell_info(int32_t argc, char *argv[]);
extern int32_t Shell_reset(int32_t argc, char *argv[]);

extern int32_t Shell_debug(int32_t argc, char *argv[]);

#endif /* !_SHELL_COMMAND_H */

/******************************** END-OF-FILE ********************************/
