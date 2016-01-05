#include "fsl_platform_common.h"
#include "bootloader_common.h"
#include "bootloader/bootloader_eeprom.h"
#include "dspi/hal/fsl_dspi_hal.h"
#include "microseconds/microseconds.h"
#include <assert.h>
#include <string.h>

#define EEPROM_SPI_INSTANCE	HW_SPI0	/* SPI0 */
#define EEPROM_SPI_BPS		5000000

#define SPI_MEMORY_READ_ERROR	(0xFF)
#define SPI_MEMORY_STATUS_ERROR	(0xFF)
#define SPI_MEMORY_EMPTY		(0xFF)

// SPI0
#define SPI0_PCS0_GPIO_PIN_NUM	0	// PIN 0 in the PTD group
#define SPI0_PCS0_ALT_MODE		2	// ALT mode for SPI0 PCS0 functionality for pin 0
#define SPI0_SCK_GPIO_PIN_NUM	1	// PIN 1 in the PTD group
#define SPI0_SCK_ALT_MODE		2	// ALT mode for SPI0 SCK functionality for pin 1
#define SPI0_SOUT_GPIO_PIN_NUM	2	// PIN 2 in the PTD group
#define SPI0_SOUT_ALT_MODE		2	// ALT mode for SPI0 SOUT functionality for pin 2
#define SPI0_SIN_GPIO_PIN_NUM	3	// PIN 3 in the PTD group
#define SPI0_SIN_ALT_MODE		2	// ALT mode for SPI0 SIN functionality for pin 3

static dspi_master_config_t s_stSpiConfig =
{
	.isEnabled						= true,
	.whichCtar						= kDspiCtar0,
	.bitsPerSec						= EEPROM_SPI_BPS,
	.isSckContinuous				= false,
	.whichPcs						= kDspiPcs0,
	.pcsPolarity					= kDspiPcs_ActiveLow,
	.masterInSample					= kDspiSckToSin_0Clock,
	.isModifiedTimingFormatEnabled	= false,
	.isTxFifoDisabled				= false,
	.isRxFifoDisabled				= false,
	.dataConfig.bitsPerFrame		= 8,
	.dataConfig.clkPolarity			= kDspiClockPolarity_ActiveHigh,
	.dataConfig.clkPhase			= kDspiClockPhase_FirstEdge,
	.dataConfig.direction			= kDspiMsbFirst
};

void eeprom_init(void)
{
	uint32_t calculatedBaudRate;

	// Enable the pins for SPI0 of SMK_Gateway
	BW_PORT_PCRn_MUX(HW_PORTD, SPI0_PCS0_GPIO_PIN_NUM, SPI0_PCS0_ALT_MODE); // SPI0_PCS0 is ALT2 for pin PTD0
	BW_PORT_PCRn_MUX(HW_PORTD, SPI0_SCK_GPIO_PIN_NUM,  SPI0_SCK_ALT_MODE);  // SPI0_SCK is ALT2 for pin PTD1
	BW_PORT_PCRn_MUX(HW_PORTD, SPI0_SOUT_GPIO_PIN_NUM, SPI0_SOUT_ALT_MODE); // SPI0_SOUT is ALT2 for pin PTD2
	BW_PORT_PCRn_MUX(HW_PORTD, SPI0_SIN_GPIO_PIN_NUM,  SPI0_SIN_ALT_MODE);  // SPI0_SIN is ALT2 for pin PTD3
	
	s_stSpiConfig.sourceClockInHz = SystemCoreClock,
 
	/* Enable clock for DSPI */
	HW_SIM_SCGC6_SET(BM_SIM_SCGC6_SPI0);
 
	/* Reset the DSPI module */
    dspi_hal_reset(EEPROM_SPI_INSTANCE);
	
	/* Init DSPI as Master */
	dspi_hal_master_init(EEPROM_SPI_INSTANCE, &s_stSpiConfig, &calculatedBaudRate);
	
    /* DSPI system enable */
    dspi_hal_enable(EEPROM_SPI_INSTANCE);
	BW_SPI_MCR_HALT(EEPROM_SPI_INSTANCE, 0);
	
    /* flush the fifos*/
    dspi_hal_flush_fifos(EEPROM_SPI_INSTANCE, true, true);
	
	return;
}

static int eeprom_addr_to_buffer(uint32_t addr, uint8_t *buffer)
{
    int i;

#if _DEBUG
	assert(buffer);
#endif
	
    for (i = SPI_MEMORY_ADDRESS_BYTES; i; i--)
    {
        buffer[i-1] = (uint8_t)(addr & 0xFF);
        addr >>= 8;
    }

    return SPI_MEMORY_ADDRESS_BYTES;
}

static void eeprom_write(uint8_t *data, uint32_t length)
{
	dspi_command_config_t config = {
										.isChipSelectContinuous = true,
										.whichCtar				= s_stSpiConfig.whichCtar,
										.whichPcs				= s_stSpiConfig.whichPcs,
										.isEndOfQueue			= false,
										.clearTransferCount		= false				};
	uint32_t i;
	
#if _DEBUG
	assert(data);
	assert(length);
#endif
	
	dspi_hal_flush_fifos(EEPROM_SPI_INSTANCE, true, true);
	BW_SPI_SR_EOQF(EEPROM_SPI_INSTANCE, 1);

	for(i=0; i<length; i++)
	{
		/* last byte */
		if (i == (length-1))
		{
			config.isChipSelectContinuous = false;
			config.isEndOfQueue			  = true;
		}
	  
		while(!BR_SPI_SR_TFFF(EEPROM_SPI_INSTANCE));
		
		BW_SPI_SR_TFFF(EEPROM_SPI_INSTANCE, 1);
		dspi_hal_write_data_master_mode(EEPROM_SPI_INSTANCE, &config, (uint16_t)(*(data+i)));
	}

	while(!BR_SPI_SR_EOQF(EEPROM_SPI_INSTANCE));

	return;
}

static void eeprom_read(uint8_t *command, uint32_t commandlength, uint8_t *data, uint32_t datalength)
{
	dspi_command_config_t config = {
										.isChipSelectContinuous = true,
										.whichCtar				= s_stSpiConfig.whichCtar,
										.whichPcs				= s_stSpiConfig.whichPcs,
										.isEndOfQueue			= false,
										.clearTransferCount		= false				};
	uint32_t i;
	const uint16_t dummy = 0;
	
#if _DEBUG
	assert(command);
	assert(commandlength);
	assert(data);
	assert(datalength);
#endif
	
	dspi_hal_flush_fifos(EEPROM_SPI_INSTANCE, true, true);
	BW_SPI_SR_EOQF(EEPROM_SPI_INSTANCE, 1);

	for(i=0; i<commandlength; i++)
	{
		BW_SPI_SR_TCF(EEPROM_SPI_INSTANCE, 1);
		dspi_hal_write_data_master_mode(EEPROM_SPI_INSTANCE, &config, (uint16_t)(*(command+i)));
		while(!BR_SPI_SR_TCF(EEPROM_SPI_INSTANCE));
	}
	
	dspi_hal_flush_fifos(EEPROM_SPI_INSTANCE, false, true);
	(void)dspi_hal_read_data(EEPROM_SPI_INSTANCE);		/* dummy read for reset FIFO */
	
	for(i=0; i<datalength; i++)
	{
		/* last byte */
		if (i == (datalength-1))
		{
			config.isChipSelectContinuous = false;
			config.isEndOfQueue			  = true;
		}
	  
		BW_SPI_SR_TCF(EEPROM_SPI_INSTANCE, 1);
		BW_SPI_SR_RFDF(EEPROM_SPI_INSTANCE, 1);
		
		dspi_hal_write_data_master_mode(EEPROM_SPI_INSTANCE, &config, dummy);
		while(!BR_SPI_SR_TCF(EEPROM_SPI_INSTANCE));
		while(!BR_SPI_SR_RFDF(EEPROM_SPI_INSTANCE));
		
		*(data+i) = (uint8_t)(dspi_hal_read_data(EEPROM_SPI_INSTANCE) & 0xFF);
	}

	return;
}

static void eeprom_milliseconds_wait(uint32_t ms)
{
	uint32_t i;	/* roop counter */

	for (i=0; i<ms; i++)
	{
		microseconds_delay(1000);
	}

	return;
}

void eeprom_chip_erase (void)
{
	uint8_t buffer = SPI_MEMORY_CHIP_ERASE;	/* sending data */
	
	/* write enable */
	eeprom_set_write_latch(true);
	if (!(eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
#if _DEBUG
		monitor_printf("eeprom_chip_erase: Error: write enable failed.\n");
#endif
		return;
	}

	eeprom_write(&buffer, 1);
	
	/* write disable */	
	eeprom_set_write_latch(false);
#if _DEBUG
	if ((eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
		monitor_printf("memory_chip_erase: Error: write disable failed.\n");
	}
#endif
	return;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_set_write_latch
* Comments  : This function sets latch to enable/disable memory write
*             operation
*
*END*----------------------------------------------------------------------*/
void eeprom_set_write_latch(bool enable)
{
	uint8_t buffer;	/* sending data */

	if (enable)
	{
		buffer = SPI_MEMORY_WRITE_LATCH_ENABLE;
	}
	else
	{
		buffer = SPI_MEMORY_WRITE_LATCH_DISABLE;
	}

	eeprom_write(&buffer, 1);
	
	/* Wait till finish writiing */
	eeprom_milliseconds_wait(SPI_MEMORY_DELAY_WRITESTATUS);

	return;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_set_protection
* Comments  : This function sets write protection in memory status register
*
*END*----------------------------------------------------------------------*/
void eeprom_set_protection (bool protect)
{
	uint8_t buffer[2] = {SPI_MEMORY_WRITE_STATUS};	/* sending data */

	/* Each write operation must be enabled in memory */
	eeprom_set_write_latch(true);
	if (!(eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
#if _DEBUG
		monitor_printf("eeprom_set_protection: Error: write enable failed.\n");
#endif
		return;
	}

	/* write data setting */
	buffer[1] = eeprom_read_status();

	if (buffer[1] != SPI_MEMORY_STATUS_ERROR)
	{
		if (protect)
		{
			buffer[1] |= SPI_MEMORY_STATUS_BPL;
		}
		else
		{
			buffer[1] &= ~(SPI_MEMORY_STATUS_BPL);
		}

		/* Write instruction */
		eeprom_write(buffer, sizeof(buffer)/sizeof(buffer[0]));
		
		/* Wait till finish writiing */
		eeprom_milliseconds_wait(SPI_MEMORY_DELAY_WRITESTATUS);
	}
#if _DEBUG
	else
	{
		monitor_printf("eeprom_set_protection: Error: write enable failed.\n");
	}
#endif

	eeprom_set_write_latch (false);

#if _DEBUG
	if (!(eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
		monitor_printf("eeprom_set_protection: Error: write disable failed.\n");
	}
#endif
	return;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_read_status
* Comments  : This function reads memory status register
* Return:
*         Status read.
*
*END*----------------------------------------------------------------------*/
uint8_t eeprom_read_status(void)
{
	uint8_t command = SPI_MEMORY_READ_STATUS;	/* EEPROM command */
	uint8_t state = SPI_MEMORY_STATUS_ERROR;	/* EEPROM status */

	eeprom_read(&command, 1, &state, 1);

	return state;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_write_byte
* Comments  : This function writes a data byte to memory
*
*
*END*----------------------------------------------------------------------*/
void eeprom_write_byte (uint32_t addr, uint8_t data)
{
#if _DEBUG
	assert(addr < (SPI_MEMORY_SECTOR_SIZE *	SPI_MEMORY_SECTOR_NUMBER));
#endif
	
	if (eeprom_read_byte(addr) == SPI_MEMORY_EMPTY)
	{
		eeprom_write_data(addr, 1, &data);
	}
#if _DEBUG
	else
	{
		monitor_printf("eeprom_write_byte: Error: Data exist. Erase before write.\n");
	}
#endif	

	return;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_read_byte
* Comments  : This function reads a data byte from memory
* Return:
*         Byte read.
*
*END*----------------------------------------------------------------------*/
uint8_t eeprom_read_byte(uint32_t addr)
{
	uint8_t data;	/* read data */
	
	if (eeprom_read_data(addr, 1, &data) != 1)
	{
#if _DEBUG
		monitor_printf("eeprom_read_byte: Error: read data failed.\n");
#endif
		data = SPI_MEMORY_READ_ERROR;
	}

	return data;
}
/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_write_data
* Comments  : This function writes data buffer to memory using page write
* Return:
*         Number of bytes written.
*
*END*----------------------------------------------------------------------*/
void eeprom_write_data (uint32_t addr, uint32_t size, uint8_t *data)
{
	uint32_t count;
	uint8_t buffer[SPI_MEMORY_ADDRESS_BYTES + 1 + SPI_MEMORY_PAGE_SIZE] = {SPI_MEMORY_WRITE_DATA};
	uint32_t i;
	
#if _DEBUG
	assert(size);
	assert(data);
	assert(!(addr & (SPI_MEMORY_PAGE_SIZE-1)));
	assert((addr+size) <= (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER));
#endif
	
	/* Each write operation must be enabled in memory */
	if(eeprom_read_status() & SPI_MEMORY_STATUS_BUSY)
	{
#if _DEBUG
		monitor_printf("memory_write_data: Other write operation is in progress.\n");
#endif
		return;
	}

	/* Send data */
	for(i=addr; i<(addr+size); i += SPI_MEMORY_PAGE_SIZE)
	{
		/* Enable write status registor */
		eeprom_set_write_latch(true);
		if(!(eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
		{
#if _DEBUG
			monitor_printf("memory_write_data: Error: write enable error.\n");
#endif
			return;
		}
	
	 	eeprom_addr_to_buffer(i, &(buffer[1]));
		count = (((i+SPI_MEMORY_PAGE_SIZE) > (addr+size)) ? (size & (SPI_MEMORY_PAGE_SIZE-1)) : SPI_MEMORY_PAGE_SIZE);
		memcpy(&buffer[SPI_MEMORY_ADDRESS_BYTES + 1], data+(i-addr), count);
		
		eeprom_write(&buffer[0], (count+SPI_MEMORY_ADDRESS_BYTES+1));

		/* Each write operation must be enabled in memory */
		while((eeprom_read_status() & SPI_MEMORY_STATUS_BUSY));
	}	

	return;
}

/*FUNCTION*---------------------------------------------------------------
*
* Function Name : memory_read_data
* Comments  : This function reads data from memory into given buffer
* Return:
*         Number of bytes read.
*
*END*----------------------------------------------------------------------*/
uint32_t eeprom_read_data(uint32_t addr, uint32_t size, uint8_t *data)
{
	uint8_t buffer[SPI_MEMORY_ADDRESS_BYTES + 2] = {SPI_MEMORY_READ_DATA};

#if _DEBUG
	assert(size);
	assert(data);
	assert((addr+size) <= (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER));
#endif	

    /* Read instruction, address */
    eeprom_addr_to_buffer(addr, &(buffer[1]));

	/* Write command, address and dummy data */
	eeprom_read(&buffer[0], sizeof(buffer)/sizeof(buffer[0]), data, size);
	
    return size;
}

void eeprom_sector_erase(uint32_t sector)
{
	uint8_t buffer[SPI_MEMORY_ADDRESS_BYTES + 1] = {SPI_MEMORY_SECTOR_ERASE};	/* sending data */

#if _DEBUG
	assert(sector < SPI_MEMORY_SECTOR_NUMBER);
#endif

	/* write enable */
	eeprom_set_write_latch (true);
	if (!(eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
#if _DEBUG
		monitor_printf("memory_sector_erase: Error: write enable failed.\n");
#endif
		return;
	}
	
	/* sector erase command */
	eeprom_addr_to_buffer((sector * SPI_MEMORY_SECTOR_SIZE), &(buffer[1]));	
	eeprom_write(buffer, sizeof(buffer)/sizeof(buffer[0]));
	
	/* wait till finish erasing */
	while (eeprom_read_status() & SPI_MEMORY_STATUS_BUSY);

	/* write disable */	
	eeprom_set_write_latch (false);
#if _DEBUG
	if ((eeprom_read_status() & SPI_MEMORY_STATUS_WEL))
	{
		monitor_printf("memory_sector_erase: Error: write disable failed.\n");
	}
#endif

	return;
}
