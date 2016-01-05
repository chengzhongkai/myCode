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
#if !defined (BOOTLOADER_HOST)
#include "device/fsl_device_registers.h"
#include "utilities/fsl_rtos_abstraction.h"
#include "flash/flash.h"
#include "microseconds/microseconds.h"
#endif

#include "bootloader/shutdown_cleanup.h"
#include "bootloader_common.h"

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

// See shutdown_cleanup.h for documentation of this function.
void shutdown_cleanup(bool isShutdown)
{
#if !defined (BOOTLOADER_HOST)
    // Clear (flush) the flash cache.
    flash_cache_clear();

    // If we are permanently exiting the bootloader, there are a few extra things to do.
    if (isShutdown)
    {
		// Shutdown microseconds driver.
        microseconds_shutdown();

        // Turn off interrupts.
        lock_acquire();

#if defined(HW_RCM_FM)
        // Disable force ROM.
        BW_RCM_FM_FORCEROM(0);

        // Clear status register (bits are w1c).
        BW_RCM_MR_BOOTROM(3);
#endif // defined(HW_RCM_FM)

        // De-initialize hardware such as disabling port clock gate
        deinit_hardware();
    }

    // Memory barriers for good measure.
    __ISB();
    __DSB();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
