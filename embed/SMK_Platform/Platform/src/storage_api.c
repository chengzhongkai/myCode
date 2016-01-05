/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "spi_memory.h"
#include "storage_api.h"

#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This application requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero in user_config.h. Please recompile BSP with this option.
#endif

#ifndef BSP_SPI_MEMORY_CHANNEL
#error This application requires BSP_SPI_MEMORY_CHANNEL to be defined. Please set it to appropriate SPI channel number in user_config.h and recompile BSP with this option.
#endif

#if BSP_SPI_MEMORY_CHANNEL == 0
	#if ! BSPCFG_ENABLE_SPI0
		#error This application requires BSPCFG_ENABLE_SPI0 defined non-zero in user_config.h. Please recompile kernel with this option.
	#else
        #define SPI_NOR_FLASH_CHANNEL			"spi0:1"
		#define SPI_NOR_FLASH_BAUD_RATE			(5000000)
		#define SPI_NOR_FLASH_CLK_MODE			SPI_CLK_POL_PHA_MODE0
		#define SPI_NOR_FLASH_TRANSFER_MODE		SPI_DEVICE_MASTER_MODE
		#define SPI_NOR_FLASH_ENDIAN			SPI_DEVICE_BIG_ENDIAN
	#endif
#else
	#error Unsupported SPI channel number. Please check settings of BSP_SPI_MEMORY_CHANNEL in BSP.
#endif

//=============================================================================
#define STORAGE_MAX_SIZE (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)

//=============================================================================
// Open Storage
//=============================================================================
MQX_FILE *Storage_Open(void)
{
	MQX_FILE *fd;
	uint32_t ioctldata[3] = {0, 0, 0};

	// ---------------------------------------------------- Open SPI Driver ---
	fd = fopen(SPI_NOR_FLASH_CHANNEL, NULL);
    if (fd == NULL) {
		err_printf("[Storage] Error: opening SPI driver\n");
		return (NULL);
	}

	// ------------------------------------------------------- Set Baudrate ---
	ioctldata[0] = SPI_NOR_FLASH_BAUD_RATE;
    if (ioctl(fd, IO_IOCTL_SPI_SET_BAUD, &ioctldata[0]) != SPI_OK) {
		err_printf("[Storage] Error: SPI baud rate setting error\n");
		fclose(fd);
		return (NULL);
    }

	// ----------------------------------------------------- Set clock mode ---
	ioctldata[0] = SPI_NOR_FLASH_CLK_MODE;
	if (ioctl(fd, IO_IOCTL_SPI_SET_MODE, &ioctldata[0]) != SPI_OK) {
		err_printf("[Storage] Error: SPI click mode setting error\n");
		fclose(fd);
		return (NULL);
    }

    // --------------------------------------------------------- Set endian ---
	ioctldata[0] = SPI_NOR_FLASH_ENDIAN;
	if (ioctl(fd, IO_IOCTL_SPI_SET_ENDIAN, &ioctldata[0]) != SPI_OK) {
		err_printf("[Storage] Error: SPI endian setting error\n");
		fclose(fd);
		return (NULL);
    }

	// -------------------------------------------------- Set transfer mode ---
	ioctldata[0] = SPI_NOR_FLASH_TRANSFER_MODE;
	if (ioctl(fd, IO_IOCTL_SPI_SET_TRANSFER_MODE, &ioctldata[0]) != SPI_OK) {
		err_printf("[Storage] Error: SPI transfer mode setting error\n");
		fclose(fd);
		return (NULL);
    }

#if BSPCFG_ENABLE_SPI_STATS
	// --------------------------------------------------- Clear statistics ---
	if (ioctl(fd, IO_IOCTL_SPI_CLEAR_STATS, NULL) != SPI_OK) {
		err_printf("[Storage] Error: SPI status clearing error\n");
		fclose(fd);
		return (NULL);
	}
#endif

	return (fd);
}

//=============================================================================
// Close Storage
//=============================================================================
void Storage_Close(MQX_FILE *fd)
{
	// --------------------------------------------------- Close SPI Driver ---
	if (fd != NULL) {
		fclose(fd);
	}
}

//=============================================================================
// Read from Storage
//=============================================================================
uint32_t Storage_Read(MQX_FILE *fd,
					  uint32_t addr, uint32_t size, uint8_t *buf)
{
	uint32_t read_size;

	if ((fd == NULL) || (size == 0) || (buf == NULL)
		|| (addr > STORAGE_MAX_SIZE) || (size > STORAGE_MAX_SIZE)
		|| ((addr + size) > STORAGE_MAX_SIZE)) {
		return (0);
	}

	read_size = memory_read_data(fd, addr, size, buf);

	return (read_size);
}

//=============================================================================
// Write to Storage
//=============================================================================
uint32_t Storage_Write(MQX_FILE *fd,
					   uint32_t addr, uint32_t size, const uint8_t *buf)
{
	uint32_t write_size;

	if ((fd == NULL) || (size == 0) || (buf == NULL)
		|| (addr > STORAGE_MAX_SIZE) || (size > STORAGE_MAX_SIZE)
		|| ((addr + size) > STORAGE_MAX_SIZE)) {
		return (0);
	}

	if (!Storage_IsEmpty(fd, addr, size)) {
		return (0);
	}

	write_size = memory_write_data(fd, addr, size, buf);

	return (write_size);
}

//=============================================================================
// Erase Storage Sector
//=============================================================================
bool Storage_EraseSector(MQX_FILE *fd, uint32_t start_sec, uint32_t end_sec)
{
	uint32_t sec;

	if ((fd == NULL)
		|| (start_sec > end_sec) || (end_sec >= SPI_MEMORY_SECTOR_NUMBER)) {
		return (false);
	}

	for (sec = start_sec; sec <= end_sec; sec ++) {
		if (!memory_sector_erase(fd, sec)) {
			return (false);
		}
	}

	return (true);
}

//=============================================================================
// Check if Storage is Empty
//=============================================================================
bool Storage_IsEmpty(MQX_FILE *fd, uint32_t addr, uint32_t size)
{
	uint8_t *work;
	uint32_t read_size;
	uint32_t cnt;

	if ((fd == NULL) || (size == 0)
		|| (addr > STORAGE_MAX_SIZE) || (size > STORAGE_MAX_SIZE)
		|| ((addr + size) > STORAGE_MAX_SIZE)) {
		return (false);
	}

	work = _mem_alloc(size);
	if (work == NULL) {
		err_printf("[Storage] Error: size (%d) has error\n", size);
		return (false);
	}

	read_size = Storage_Read(fd, addr, size, work);
	if (read_size != size) {
		_mem_free(work);
		return (false);
	}

	for (cnt = 0; cnt < size; cnt ++) {
		if (work[cnt] != 0xff) break;
	}

	_mem_free(work);

	return ((cnt == size) ? true : false);
}

/******************************** END-OF-FILE ********************************/
