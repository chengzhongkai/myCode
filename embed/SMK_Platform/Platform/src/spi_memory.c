/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "spi_memory.h"

#define SPI_MEMORY_READ_ERROR	(0xFF)
#define SPI_MEMORY_STATUS_ERROR	(0xFF)
#define SPI_MEMORY_EMPTY		(0xFF)

//=============================================================================
// Fills in given address into buffer in correct byte order
//=============================================================================
static int memory_addr_to_buffer(uint32_t addr, uint8_t *buffer)
{
    int i;

    for (i = SPI_MEMORY_ADDRESS_BYTES; i; i--)
    {
        buffer[i-1] = (uint8_t)(addr & 0xFF);
        addr >>= 8;
    }

    return SPI_MEMORY_ADDRESS_BYTES;
}

//=============================================================================
// This function erases the whole memory SPI chip
//=============================================================================
void memory_chip_erase(MQX_FILE_PTR spifd)
{
	_mqx_int result;
	uint8_t buffer[1] = {SPI_MEMORY_CHIP_ERASE};
	
	/* write enable */
	memory_set_write_latch(spifd, true);
	if (!(memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_chip_erase: Error: write enable failed.\n");
		return;
	}
	
	/* write command */
	result = fwrite(buffer, sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);
	fflush(spifd);

	if (result == sizeof(buffer)/sizeof(buffer[0]))
	{
		/* wait till finish erasing */
		_time_delay(SPI_MEMORY_DELAY_CHIPERASE);
		while (memory_read_status(spifd) & SPI_MEMORY_STATUS_BUSY)
		{
			_time_delay(1);
		}
	}
	else
	{
		err_printf("memory_chip_erase: Error: chip erase failed.\n");
	}
	
	/* write disable */	
	memory_set_write_latch (spifd, false);
	if ((memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_chip_erase: Error: write disable failed.\n");
	}
}

//=============================================================================
// This function sets latch to enable/disable memory write operation
//=============================================================================
void memory_set_write_latch(MQX_FILE_PTR spifd, bool enable)
{
	_mqx_int result;
	uint8_t buffer[1];

	if (enable)
	{
		buffer[0] = SPI_MEMORY_WRITE_LATCH_ENABLE;
	} else {
		buffer[0] = SPI_MEMORY_WRITE_LATCH_DISABLE;
	}

	/* Write instruction */
	result = fwrite(buffer, sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);
	fflush (spifd);

	/* Wait till finish writiing */
	_time_delay(SPI_MEMORY_DELAY_WRITESTATUS);
	
	if (result != (sizeof(buffer) / sizeof(buffer[0])))
	{
		err_printf("memory_set_write_latch: Error: WEL register unchanged.\n");
	}

	return;
}

//=============================================================================
// This function sets write protection in memory status register
//=============================================================================
void memory_set_protection(MQX_FILE_PTR spifd, bool protect)
{
	_mqx_int result;
	uint8_t buffer[2] = {SPI_MEMORY_WRITE_STATUS};

	/* Each write operation must be enabled in memory */
	memory_set_write_latch(spifd, true);
	if (!(memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_set_protection: Error: write enable failed.\n");
		return;
	}

	/* write data setting */
	buffer[1] = memory_read_status(spifd);

	if (buffer[1] != SPI_MEMORY_STATUS_ERROR)
	{
		if (protect)
		{
			buffer[1] |= SPI_MEMORY_STATUS_BPL;
		} else {
			buffer[1] &= ~(SPI_MEMORY_STATUS_BPL);
		}

		/* Write instruction */
		result = fwrite(&buffer[0], sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);
		fflush (spifd);

		if (result != (sizeof(buffer) / sizeof(buffer[0])))
		{
			err_printf("memory_set_protection: Error: write status disabled yet.\n");
		}

		/* Wait till finish writiing */
		_time_delay(SPI_MEMORY_DELAY_WRITESTATUS);
	}
	else
	{
		err_printf("memory_set_protection: Error: write enable failed.\n");
	}
	
	memory_set_write_latch (spifd, false);
	if (!(memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_set_protection: Error: write disable failed.\n");
	}
	
	return;
}

//=============================================================================
// This function reads memory status register
//=============================================================================
uint8_t memory_read_status(MQX_FILE_PTR spifd)
{
	_mqx_int result;
	uint8_t buffer[1] = {SPI_MEMORY_READ_STATUS};
	uint8_t state = SPI_MEMORY_STATUS_ERROR;

	/* Write instruction */
	result = fwrite(buffer, sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);
	
	if (result != sizeof(buffer)/sizeof(buffer[0]))
	{
		/* Stop transfer */
		fflush (spifd);
		err_printf("memory_read_status: Error: write command failed.\n");
		return state;
	}

	/* Read memory status */
	result = fread(&state, sizeof(state), sizeof(state)/sizeof(uint8_t), spifd);
	fflush (spifd);

	if (result != sizeof(state)/sizeof(uint8_t))
	{
		err_printf("memory_read_status: Error: read status failed.\n");
	}

	return state;
}

//=============================================================================
// This function writes a data byte to memory
//=============================================================================
void memory_write_byte(MQX_FILE_PTR spifd, uint32_t addr, uint8_t data)
{
	uint8_t writedata[SPI_MEMORY_PAGE_SIZE];
 
 	if (memory_read_byte(spifd, addr) != SPI_MEMORY_EMPTY)
	{
		err_printf("memory_write_byte: Error: Data exist. Erase before write.\n");
		return;
	}

	memset(&writedata[0], SPI_MEMORY_EMPTY, SPI_MEMORY_PAGE_SIZE);
	
	writedata[(addr & (SPI_MEMORY_PAGE_SIZE-1))] = data;

	if (memory_write_data(spifd,
						  (addr & ~(SPI_MEMORY_PAGE_SIZE-1)),
						  (addr & (SPI_MEMORY_PAGE_SIZE-1))+1,
						  &writedata[0])
		!= (addr & (SPI_MEMORY_PAGE_SIZE-1))+1)
	{
		err_printf("memory_write_byte: write data failed.\n");
	}
	return;
}

//=============================================================================
// This function reads a data byte from memory
//=============================================================================
uint8_t memory_read_byte(MQX_FILE_PTR spifd, uint32_t addr)
{
	uint8_t data;
	
	if (memory_read_data(spifd, addr, sizeof(data), &data) != sizeof(data))
	{
		err_printf("memory_read_byte: Error: read data failed.\n");
		data = SPI_MEMORY_READ_ERROR;
	}
	
	return data;
}

//=============================================================================
// This function writes data buffer to memory using page write
//=============================================================================
uint32_t memory_write_data(MQX_FILE_PTR spifd,
						   uint32_t addr, uint32_t size, const uint8_t *data)
{
	_mqx_int result;
	uint32_t count;
	uint8_t buffer[SPI_MEMORY_ADDRESS_BYTES + 1] = {SPI_MEMORY_WRITE_DATA};
	uint32_t i;
	
	if (((addr & (SPI_MEMORY_PAGE_SIZE-1)) != 0) || (size == 0) || ((addr+size) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) || (data == NULL))
	{
		err_printf("memory_write_data: input data error.\n");
		return 0;
	}
	
	/* Each write operation must be enabled in memory */
	if(memory_read_status (spifd) & SPI_MEMORY_STATUS_BUSY)
	{
		err_printf("memory_write_data: Other write operation is in progress.\n");
		return 0;
	}

	/* Send data */
	for(i=addr; i<(addr+size); i += SPI_MEMORY_PAGE_SIZE)
	{
		/* Enable write status registor */
		memory_set_write_latch (spifd, TRUE);
		if(!(memory_read_status (spifd) & SPI_MEMORY_STATUS_WEL))
		{
			err_printf("memory_write_data: Error: write enable error.\n");
			return 0;
		}
	
	 	memory_addr_to_buffer(i, &(buffer[1]));
		if (fwrite(&buffer[0], sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd) != (sizeof(buffer)/sizeof(buffer[0])))
		{	
			fflush (spifd);
			err_printf("memory_write_data: Error: write command failed.\n");
			return 0;
		}
	
		count = (((i+SPI_MEMORY_PAGE_SIZE) > (addr+size)) ? (size & (SPI_MEMORY_PAGE_SIZE-1)) : SPI_MEMORY_PAGE_SIZE);
		
  		result = fwrite((void *)(data+(i-addr)), sizeof(uint8_t), count, spifd);
		fflush (spifd);

		/* Each write operation must be enabled in memory */
		_time_delay (SPI_MEMORY_DELAY_BYTEPRG);
		while((memory_read_status (spifd) & SPI_MEMORY_STATUS_BUSY))
		{
			_time_delay(1);
		}

		if (result != count)
		{
			err_printf("memory_write_data: Error: write data failed.");
			break;
		}
	}	

	/* Disable write status registor achieves automatically when write ends */
//	memory_set_write_latch (spifd, FALSE);
//	if((memory_read_status (spifd) & SPI_MEMORY_STATUS_WEL))
//	{
//		fputs("memory_write_data: Error: write disable error.", stderr);
//		result = 0;
//	}

	return ((result == count) ? size : 0);
}

//=============================================================================
// This function reads data from memory into given buffer
//=============================================================================
uint32_t memory_read_data (MQX_FILE_PTR spifd,
						   uint32_t addr, uint32_t size, uint8_t *data)
{
    _mqx_int result;
	uint8_t buffer[SPI_MEMORY_ADDRESS_BYTES + 2] = {SPI_MEMORY_READ_DATA};

	if ((size == 0) || ((addr+size) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) || (data == NULL))
	{
		err_printf("memory_resd_data: input data error.\n");
		return 0;
	}
	
    /* Read instruction, address */
    memory_addr_to_buffer(addr, &(buffer[1]));

	/* Write command, address and dummy data */
    result = fwrite (&buffer[0], sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);

    if (result != sizeof(buffer)/sizeof(buffer[0]))
    {
        /* Stop transfer */
        fflush (spifd);
		err_printf("memory_read_data: Error: write command failed.\n");
		return 0;
    }

    /* Read size bytes of data */
    result = fread(data, sizeof(uint8_t), (_mqx_int)size, spifd);
    fflush (spifd);

    if (result != size)
    {
		err_printf("memory_read_data: Error: read data failed.\n");
		result = 0;
    }

    return (uint32_t)result;
}

//=============================================================================
// Erase One Sector
//=============================================================================
bool memory_sector_erase(MQX_FILE_PTR spifd, uint32_t sector)
{
	_mqx_int result;
	uint8_t buffer[4] = {SPI_MEMORY_SECTOR_ERASE};
	bool erase = false;
	
	if (sector > (SPI_MEMORY_SECTOR_NUMBER-1))
	{
		err_printf("memory_sector_erase: Error: invalid sector.\n");
		return false;
	}
	
	/* write enable */
	memory_set_write_latch (spifd, true);
	if (!(memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_sector_erase: Error: write enable failed.\n");
		return false;
	}
	
	/* sector erase command */
	memory_addr_to_buffer((sector * SPI_MEMORY_SECTOR_SIZE), &(buffer[1]));
	result = fwrite(buffer, sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), spifd);
	fflush (spifd);

	if (result == sizeof(buffer)/sizeof(buffer[0]))
	{
		/* wait till finish erasing */
		_time_delay(SPI_MEMORY_DELAY_SECTORERASE);
		while (memory_read_status(spifd) & SPI_MEMORY_STATUS_BUSY)
		{
			_time_delay(1);
		}
		
		erase = true;
	}
	else
	{
		err_printf("memory_sector_erase: Error: sector erase failed.\n");
	}

	/* write disable */	
	memory_set_write_latch(spifd, false);
	if ((memory_read_status(spifd) & SPI_MEMORY_STATUS_WEL))
	{
		err_printf("memory_sector_erase: Error: write disable failed.\n");
		erase = false;
	}
	
	return erase;
}

/******************************** END-OF-FILE ********************************/
