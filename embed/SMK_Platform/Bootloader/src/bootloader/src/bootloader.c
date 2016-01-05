/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "bootloader/shutdown_cleanup.h"
#include "bootloader_common.h"
#include "microseconds/microseconds.h"
#include "utilities/fsl_rtos_abstraction.h"

#include "bootloader_config.h"
#include "bootloader/bootloader_eeprom.h"
#include "bootloader/bootloader_update.h"
#include "bootloader/bootloader_led.h"

static void bootloader_init(void);
void main(void);

static void bootloader_init(void)
{
    // Init the global irq lock
    lock_init();

    // Init pinmux and other hardware setup.
    init_hardware();

    // Start the lifetime counter
    microseconds_init();

#if _DEBUG
	// Initialize state monitor via UART4
	monitor_init();
#endif
	
	//Initialize external EEPROM on SPI0
	eeprom_init();	

	//Initialize LED
	led_init();

#if _DEBUG
    // Message so python instantiated debugger can tell the
    // bootloader application is running on the target.
    monitor_printf("\r\n\r\nRunning bootloader...\r\n");
    monitor_printf("Bootloader version %c%d.%d.%d\r\n",
					kBootloader_Version_Name,
					kBootloader_Version_Major,
					kBootloader_Version_Minor,
					kBootloader_Version_Bugfix);
#endif
}

//! @brief Entry point for the bootloader.
void main(void)
{
	bootloader_init();
	
	led_start((ftm_pwm_param_t *)&stParamNormal);

	if (update_check_magicword() == true)
	{

		update_exec();
	}

	//boot application
	led_deinit();
	shutdown_cleanup(true);

	__set_MSP(*(int *)BL_APP_VECTOR_TABLE_ADDRESS);
	SCB->VTOR = BL_APP_VECTOR_TABLE_ADDRESS;
	
	((void (*)())(*(int *)(BL_APP_VECTOR_TABLE_ADDRESS + 4)))();
	
#if _DEBUG
    // Should never end up here.
    monitor_printf("Warning: reached end of main()\r\n");
#endif
}

//! Since we never exit this gets rid of the C standard functions that cause
//! extra ROM size usage.
void exit(int arg)
{
}

//! @}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
