/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "eeprom_device.h"

//=============================================================================
#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This task requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero. Please recompile BSP with this option.
#endif

#if ! BSPCFG_ENABLE_FLASHX
#error This task requires BSPCFG_ENABLE_FLASHX defined non-zero. Please recompile kernel with this option.
#else
#define FLASH_CHANNEL "flashx:flexram0"
#endif

/* FlexNVM parttition setting */
#define FLEXNVM_EE_SPLIT	FLEXNVM_EE_SPLIT_1_1			/* EEPROM Split Factor */
#define FLEXNVM_EE_SIZE		FLEXNVM_EE_SIZE_4096			/* EEPROM Size */
#ifndef FLASH_CLEAR
	#define FLEXNVM_PARTITION	FLEXNVM_PART_CODE_DATA0_EE128	/* FlexNVM Partition Code */
#else
	#define FLEXNVM_PARTITION	50	/* FlexNVM Partition Code */
#endif
/* If you want to change partition, you have to erase entire internalflash memory before.
   See K64 Sub-Family Reference Manual 29.4.12.15,*/

//=============================================================================
#if (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_4096)
#define FLASHEEPROM_SIZE	4096
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_2048)
#define FLASHEEPROM_SIZE	2048
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_1024)
#define FLASHEEPROM_SIZE	1024
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_512)
#define FLASHEEPROM_SIZE	512
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_256)
#define FLASHEEPROM_SIZE	256
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_128)
#define FLASHEEPROM_SIZE	128
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_64)
#define FLASHEEPROM_SIZE	64
#elif (FLEXNVM_EE_SIZE == FLEXNVM_EE_SIZE_32)
#define FLASHEEPROM_SIZE	32
#else
#define FLASHEEPROM_SIZE	0
#endif

//=============================================================================
#define FLASHEEPROM_FLEXRAM_BASE (uint8_t *)0x14000000

#define FLASHEEPROM_READ_READY 1

//=============================================================================
MQX_FILE *EEPROMDev_Open(void)
{
	MQX_FILE *fd;

	/* Function Control Code for FlexRAM available for EEPROM */
	const uint8_t modeset = FLEXNVM_FLEXRAM_EE;

	FLEXNVM_PROG_PART_STRUCT param;

	fd = fopen(FLASH_CHANNEL, NULL);
	if (fd == NULL) {
		return (NULL);
	}

	/* switch FlexRAM to EEPROM mode */
	_io_ioctl(fd, FLEXNVM_IOCTL_SET_FLEXRAM_FN, (void *)&modeset);

    /* check FlexNVM partition settings */
	if ((_io_ioctl(fd, FLEXNVM_IOCTL_GET_PARTITION_CODE, &param) != IO_OK)
		|| (param.EE_DATA_SIZE_CODE != (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE))
		|| (param.FLEXNVM_PART_CODE != FLEXNVM_PARTITION)) {
		/* not initialized */
		if ((param.EE_DATA_SIZE_CODE == 0x3F)
			&& (param.FLEXNVM_PART_CODE == 0x0F)) {
		 	/* Initialize FlexNVM partition and EEPROM size */
			param.EE_DATA_SIZE_CODE = (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE);
			param.FLEXNVM_PART_CODE = FLEXNVM_PARTITION;
			_io_ioctl(fd, FLEXNVM_IOCTL_SET_PARTITION_CODE, &param);
		}

		/* check FlexNVM partition settings again */
		if ((_io_ioctl(fd, FLEXNVM_IOCTL_GET_PARTITION_CODE, &param) != IO_OK)
			|| (param.EE_DATA_SIZE_CODE != (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE))
			|| (param.FLEXNVM_PART_CODE != FLEXNVM_PARTITION)) {
			err_printf("\nFlexNVM configuration error\n"
					   "Must erase whole internal chip.\n");
			fclose(fd);
			return (NULL);
		}
	}

	return (fd);
}

//=============================================================================
void EEPROMDev_Close(MQX_FILE *fd)
{
	if (fd != NULL) {
		fclose(fd);
	}
}

//=============================================================================
uint32_t EEPROMDev_Read(MQX_FILE *fd, uint32_t ofs, uint32_t len,
						uint8_t *buf, uint32_t size)
{
	uint32_t status;

	uint8_t *src_ptr;
	int32_t read_size;

	if (fd == NULL || buf == NULL || len == 0 || size == 0) {
		return (0);
	}

	/* check EEPROM readable */
	_io_ioctl(fd, FLEXNVM_IOCTL_GET_EERDY, &status);

	if (status == FLASHEEPROM_READ_READY) {
		if (size >= len) {
			read_size = len;
		} else {
			read_size = size;
		}
		if ((ofs + read_size) > FLASHEEPROM_SIZE) {
			read_size = FLASHEEPROM_SIZE - ofs;
		}
		if (read_size <= 0) {
			return (0);
		}
		src_ptr = FLASHEEPROM_FLEXRAM_BASE + ofs;
		memcpy(buf, src_ptr, read_size);

		return (read_size);
	} else {
		return (0);
	}
}

//=============================================================================
static void EEPROMDev_Write_inner(MQX_FILE *fd, uint8_t *dest,
								  const uint8_t *src, uint32_t size)
{
	uint32_t cnt;

	_io_ioctl(fd, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
	for (cnt = 0; cnt < size; cnt++) {
		*dest ++ = *src ++;
		_io_ioctl(fd, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
	}
}

//=============================================================================
uint32_t EEPROMDev_Write(MQX_FILE *fd, uint32_t ofs, uint32_t len,
						 const uint8_t *buf, uint32_t size)
{
	uint32_t cnt;
	int32_t write_size;
	uint8_t *dest_ptr;

	uint32_t status;

	if (fd == NULL || buf == NULL || len == 0 || size == 0) {
		return (0);
	}

	// ---------------------------------------------------- Write to EEPROM ---
	dest_ptr = FLASHEEPROM_FLEXRAM_BASE + ofs;

	if (size >= len) {
		write_size = len;
	} else {
		write_size = size;
	}
	if ((ofs + write_size) > FLASHEEPROM_SIZE) {
		write_size = FLASHEEPROM_SIZE - ofs;
	}
	if (write_size <= 0) {
		return (0);
	}

	EEPROMDev_Write_inner(fd, dest_ptr, buf, write_size);

	// ------------------------------------------------------------- Verify ---
	_io_ioctl(fd, FLEXNVM_IOCTL_GET_EERDY, &status);

	if (status == FLASHEEPROM_READ_READY
		&& memcmp(buf, dest_ptr, write_size) == 0) {
		return (write_size);
	} else {
		return (0);
	}
}

/******************************** END-OF-FILE ********************************/
