#include "bootloader_common.h"
#include "utilities/fsl_rtos_abstraction.h"

#include "bootloader/bootloader_eeprom.h"
#include "bootloader/bootloader_update.h"
#include "bootloader/bootloader_led.h"
#include "bootloader_config.h"
#include "flash/flash.h"

#include <string.h>

#define FILE_LENGTH_ERROR	0xFFFFFFFF

#define FILEDATE_LATEST	1
#define FILEDATE_SAME	0
#define FILEDATE_OLD	2

static uint32_t update_read_bytelength(void);
static uint32_t update_check_date(void);
static bool update_check_forced(void);
static uint32_t update_version_to_value(VERSION_DATA_PTR data);
static void update_error_stop(void);

bool update_check_magicword(void)
{
	const uint8_t data[] = MAGICWORD_DATA;
	uint8_t	savedata[SPI_MEMORY_PAGE_SIZE];
 
	eeprom_read_data(UPDATE_MAGICWORD_ADDRESS, SPI_MEMORY_PAGE_SIZE, &savedata[0]);
	
	return ((memcmp(&data[0], &savedata[0], sizeof(data)) == 0) ? true : false);
}

void update_exec(void)
{
 	uint32_t bytelength;				/* byte length of update program */
	uint32_t failedaddress;				/* verify : failed address */
	uint8_t  faileddata;				/* verify : failed data */
	flash_driver_t internalflash;		/* flash driver handle */
	union {
		uint8_t		bytedata[BLOCK_SIZE];
		uint32_t	dworddata[BLOCK_SIZE/4];
	} programdata;						/* update program read/write buffer */
	uint32_t datecomp;					/* date check result */
	uint32_t i;							/* roop counter */
	status_t error;						/* error */

	/* check length */
	bytelength = update_read_bytelength();
	if (bytelength == FILE_LENGTH_ERROR)
	{
		return;
	}

	/* check date and forced flag */
	datecomp = update_check_date();
	if ( (datecomp != FILEDATE_LATEST) && (update_check_forced() == false) )
	{
#if _DEBUG
		monitor_printf("bootloader : Update file isn't latest\n");
#endif
		return;
	}
	
#if _DEBUG
	monitor_printf("Program update start.\r\n");
#endif

	if (flash_init(&internalflash) != kStatus_Success)
	{
#if _DEBUG
		monitor_printf("Error : Flash memory initialize failed.\r\n");
#endif
		return;
	}

	if (flash_erase(&internalflash, BL_APP_VECTOR_TABLE_ADDRESS, UPDATE_PROGRAM_MAXSIZE) != kStatus_Success)
	{
#if _DEBUG
		monitor_printf("Error : Flash memory erasesing failed.\r\n");
#endif
		return;
	}

	for(i=0; i<bytelength; i+=sizeof(programdata))
	{
		eeprom_read_data((i+UPDATE_PROGRAM_ADDRESS), sizeof(programdata), &programdata.bytedata[0]);
		
		lock_acquire();
		error = flash_program(&internalflash, (i+BL_APP_VECTOR_TABLE_ADDRESS), &programdata.dworddata[0], sizeof(programdata));
		lock_release();

		if (error != kStatus_Success)
		{
#if _DEBUG
			monitor_printf("Error : programming failed. Stop.\r\n");
#endif
			update_error_stop();
		}
		
		lock_acquire();
		error = flash_verify_program(&internalflash, (i+BL_APP_VECTOR_TABLE_ADDRESS), sizeof(programdata), &programdata.bytedata[0], kFlashMargin_User, &failedaddress, &faileddata);
		lock_release();
		
		if (error != kStatus_Success)
		{
#if _DEBUG
			monitor_printf("Error : Verification failed at %X.\r\n", failedaddress);
#endif
			update_error_stop();
		}
	}
	
#if _DEBUG
	monitor_printf("Update complete.\r\n");
#endif

	for (i=8; i<SPI_MEMORY_SECTOR_NUMBER; i++)
	{
		eeprom_sector_erase(i);
	}

	return;
}

static uint32_t update_read_bytelength(void)
{
	char data[8];		/* EEPROM char data */
	uint32_t length;	/* result */
	
	eeprom_read_data(UPDATE_LENGTH_ADDRESS, sizeof(data), (uint8_t *)&data[0]);
	length = atoi(data);
	
	if ((length == 0) || (length > UPDATE_PROGRAM_MAXSIZE))
	{
#if _DEBUG
		monitor_printf("Size error\r\n");
#endif
		length = FILE_LENGTH_ERROR;
	}

	return length;
}

static uint32_t update_check_date(void)
{
 	uint32_t eepromdate;					/* compile date value of software in external SPI EEPROM */
	uint32_t currentdate;					/* compile date value of current software */
	uint8_t buffer[sizeof(VERSION_DATA)];	/* read buffer for version data */
	uint32_t result = FILEDATE_OLD;			/* result */

	/* read current software DATE */
	currentdate = update_version_to_value((VERSION_DATA_PTR)CURRENT_DATE_PLACE);

	/*  */
	eeprom_read_data(UPDATE_VERSION_ADDRESS, sizeof(VERSION_DATA), &buffer[0]);
	eepromdate = update_version_to_value((VERSION_DATA_PTR)&buffer[0]);

	if (eepromdate > currentdate)
	{
		result = FILEDATE_LATEST;
	}
	else if (eepromdate == currentdate)
	{
		result = FILEDATE_SAME;
	}

	return result;
}

static bool update_check_forced(void)
{
	union {
		uint8_t		bytedata[4];
		uint32_t	rawdata;
	} savedata;
 
	eeprom_read_data(UPDATE_FORCED_ADDRESS, sizeof(savedata), &savedata.bytedata[0]);
	
	return ((savedata.rawdata == 0) ? true : false);
}

static uint32_t update_version_to_value(const VERSION_DATA_PTR data)
{
	uint32_t result;	/* result */
	
	result =  ((uint32_t)data->bMajorVersion << 8)
			+ (uint32_t)data->bMinorVersion;
	
	return result;
}

static void update_error_stop(void)
{
	led_stop((ftm_pwm_param_t *)&stParamNormal);
	led_start((ftm_pwm_param_t *)&stParamError); 
	while(1);
}