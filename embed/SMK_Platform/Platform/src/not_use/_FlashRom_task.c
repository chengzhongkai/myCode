#include <string.h>
#include <mqx.h>
#include <bsp.h>
#include <message.h>
#include <flashx.h>
#include <flash_ftfe.h>
//#include "config.h"
#include "FlashRom_task.h"
#if _DEBUG
#include <assert.h>
#include <stdlib.h>
#endif

#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This task requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero. Please recompile BSP with this option.
#endif

#if ! BSPCFG_ENABLE_FLASHX
#error This task requires BSPCFG_ENABLE_FLASHX defined non-zero. Please recompile kernel with this option.
#else
#define FLASH_CHANNEL		"flashx:flexram0"
#endif

/* Flash size */
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

#define FLASHEEPROM_FLEXRAM_BASE	0x14000000
#define FLASHEEPROM_READ_READY		1

static void FlashRom_read_exec(MQX_FILE_PTR flashid, GROUPWARE_MESSAGE_STRUCT_PTR message);
static void FlashRom_write_exec(MQX_FILE_PTR flashid, GROUPWARE_MESSAGE_STRUCT_PTR message);

typedef struct flash_rom_table
{
	uint32_t	*pAddress;
	uint32_t	dwBytesize;
} FLASH_ROM_TABLE, * FLASH_ROM_TABLE_PTR;

const FLASH_ROM_TABLE stFlashDataTable[] =
{
	{(uint32_t *)FLASHROM_MACADDR_ADDR,		FLASHROM_MACADDR_SIZE},		/* MAC address */
	{(uint32_t *)FLASHROM_MACHINE_ADDR,		FLASHROM_MACHINE_SIZE},		/* Machine code */
	{(uint32_t *)FLASHROM_SERIAL_ADDR,		FLASHROM_SERIAL_SIZE},		/* Serial number */
	{(uint32_t *)FLASHROM_FTPID_ADDR,		FLASHROM_FTPID_SIZE},		/* FTP domain ID */
	{(uint32_t *)FLASHROM_SUBDOMAIN_ADDR,	FLASHROM_SUBDOMAIN_SIZE},	/* FTP subdomain */
	{(uint32_t *)FLASHROM_PATH_ADDR,		FLASHROM_PATH_SIZE},		/* FTP path */
	{(uint32_t *)FLASHROM_DHCP_ADDR,		FLASHROM_DHCP_SIZE},		/* DHCP enable */
	{(uint32_t *)FLASHROM_STATICIP_ADDR,	FLASHROM_STATICIP_SIZE},	/* Static IP Address */
	{(uint32_t *)FLASHROM_SUBNET_ADDR,		FLASHROM_SUBNET_SIZE},		/* Static subnet mask */
	{(uint32_t *)FLASHROM_GATEWAY_ADDR,		FLASHROM_GATEWAY_SIZE},		/* Static Gateway Address */
#ifdef EXEO_PLATFORM
	{(uint32_t *)FLASHROM_CONFIG_ADDR,		FLASHROM_CONFIG_SIZE},		/* Configration data (all of above data) */
	{(uint32_t *)FLASHROM_CALIBRATION_ADDR,	FLASHROM_CALIBRATION_SIZE},	/* Sensor calibration data */
	{(uint32_t *)FLASHROM_MISC_ADDR,		FLASHROM_MISC_SIZE},		/* Misc setting data */
#endif
};

static _task_id gFlashRomTID = MQX_NULL_TASK_ID;
static _queue_id gFlashRomQID = 0;

_task_id FlashRom_GetTaskID(void)
{
    return (gFlashRomTID);
}

_queue_id FlashRom_GetQueueID(void)
{
    return (gFlashRomQID);
}

/* Task */
void FlashRom_task(uint32_t dummy)
{
    MQX_FILE_PTR					flashfd;	/* Flash ROM file ID */
	FLEXNVM_PROG_PART_STRUCT		flexparam;	/* FlexNVM setting parameter */
	_queue_id 						queueid;	/* message queue id */
	GROUPWARE_MESSAGE_STRUCT_PTR	receivemsg;	/* structure of received message */
	const uint8_t 					modeset =  FLEXNVM_FLEXRAM_EE;	/* Function Control Code for FlexRAM available for EEPROM */
    _queue_number q_no = (_queue_number)dummy;

    /* Open the flash device */
	flashfd = fopen(FLASH_CHANNEL, NULL);
    if (flashfd == NULL)
	{
		_task_destroy(MQX_NULL_TASK_ID);
	}

	/* switch FlexRAM to EEPROM mode */
	_io_ioctl(flashfd, FLEXNVM_IOCTL_SET_FLEXRAM_FN, (void *)&modeset);

    /* check FlexNVM partition settings */
	if ( (_io_ioctl(flashfd, FLEXNVM_IOCTL_GET_PARTITION_CODE, &flexparam) != IO_OK) ||
		 (flexparam.EE_DATA_SIZE_CODE != (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE))       ||
         (flexparam.FLEXNVM_PART_CODE != FLEXNVM_PARTITION)                            )
	{
		/* not initialized */
		if( (flexparam.EE_DATA_SIZE_CODE == 0x3F) &&
            (flexparam.FLEXNVM_PART_CODE == 0x0F)   )
		{
		  	/* Initialize FlexNVM partition and EEPROM size */
			flexparam.EE_DATA_SIZE_CODE = (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE);
			flexparam.FLEXNVM_PART_CODE = FLEXNVM_PARTITION;
			_io_ioctl(flashfd, FLEXNVM_IOCTL_SET_PARTITION_CODE, &flexparam);

			/* 初期値の書き込みもここでする? */
		}

		/* check FlexNVM partition settings again */
		if ( (_io_ioctl(flashfd, FLEXNVM_IOCTL_GET_PARTITION_CODE, &flexparam) != IO_OK) ||
			 (flexparam.EE_DATA_SIZE_CODE != (FLEXNVM_EE_SPLIT | FLEXNVM_EE_SIZE))       ||
        	 (flexparam.FLEXNVM_PART_CODE != FLEXNVM_PARTITION)                            )
		{
#if _DEBUG
			printf("\nFlexNVM configuration error\nMust erase whole internal chip.\n");
#endif

#ifdef FLASH_CLEAR
			_io_ioctl(flashfd, FLASH_IOCTL_ERASE_CHIP, NULL);
// パーティションを切りなおしたいなど、flexNVMの状態をすべて初期状態に戻したいときは
// flexparam.EE_DATA_SIZE_CODEもしくはflexparam.FLEXNVM_PART_CODEの設定を変更したうえで
// 上の行を有効にする。ただし、上の行を実行するとプログラム領域の書き込み内容も消去される。
#endif
			_task_destroy(MQX_NULL_TASK_ID);
		}
	}

	/* Initialize message queue */
	queueid = _msgq_open_system(q_no, 0, &_task_ready, _task_get_td(_task_get_id()));
	if (queueid == 0)
	{
#if _DEBUG
		fprintf(stderr, "FlashRom_task: Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
#endif
		fclose(flashfd);
		_task_destroy(MQX_NULL_TASK_ID);
	}

    gFlashRomQID = queueid;
    gFlashRomTID = _task_get_id();

	/* task */
	while(1) {

		receivemsg = _msgq_poll(queueid);

		if (receivemsg != NULL)
		{
			switch (receivemsg->data[0])
			{
				case FLASHROM_REQUEST_READ:		/* READ */

					FlashRom_read_exec(flashfd, receivemsg);
					break;

				case FLASHROM_REQUEST_WRITE:		/* WRITE */

					FlashRom_write_exec(flashfd, receivemsg);
					break;

				default:

					receivemsg->data[0] = FLASHROM_REQUEST_ERROR_RES;
			}

			receivemsg->header.TARGET_QID = receivemsg->header.SOURCE_QID;
			receivemsg->header.SOURCE_QID = queueid;

			if (_msgq_send(receivemsg) == false)
			{
				if (receivemsg->data[0] == FLASHROM_REQUEST_READ_RES)
				{
			  		_mem_free((void *)receivemsg->data[3]);
				}
				_msg_free(receivemsg);
			}
		}
		else
		{
			_task_block();
		}
	}
}

/* Read from Flash */
static void FlashRom_read_exec(MQX_FILE_PTR flashid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	uint8_t		*flashdata;		/* data read area */
	uint32_t	flashstatus;	/* EEPROM readable status */
	uint32_t	address;		/* read start address */
	uint32_t	size;			/* read size */

#if _DEBUG
	//assert(flashid);
	//assert(message);
#endif

	message->data[0] = FLASHROM_REQUEST_READ_ERROR;

	/* check size and address */
	if(message->data[1] >= sizeof(stFlashDataTable)/sizeof(stFlashDataTable[0]))
	{
		return;
	}

	address = (uint32_t)stFlashDataTable[message->data[1]].pAddress;
	size    = stFlashDataTable[message->data[1]].dwBytesize;

	/* check EEPROM readable */
	_io_ioctl(flashid, FLEXNVM_IOCTL_GET_EERDY, &flashstatus);

	if (flashstatus == FLASHEEPROM_READ_READY)
	{
		flashdata = (uint8_t *)_mem_alloc_system(size);

		if (flashdata != NULL)
		{
			/* read data */
			memcpy(flashdata, (uint8_t *)(address + FLASHEEPROM_FLEXRAM_BASE), size);
			message->data[0] = FLASHROM_REQUEST_READ_RES;
			message->data[2] = size;
			message->data[3] = (uint32_t)flashdata;
		}
	}

	return;
}

/* Write to EEPROM */
static void FlashRom_write_exec(MQX_FILE_PTR flashid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	uint32_t 	address;		/* write start address */
	uint32_t	size;			/* write size */
	uint8_t		*data;			/* write data pointer */
	uint32_t	flashstatus;	/* EEPROM readable status */
	uint32_t	i;				/* roop count */

#if _DEBUG
//	assert(flashid);
//	assert(message);
#endif

	message->data[0] = FLASHROM_REQUEST_WRITE_ERROR;

	size    = message->data[2];
	data    = (uint8_t *)message->data[3];

	/* check message */
	if( (message->data[1] >= sizeof(stFlashDataTable)/sizeof(stFlashDataTable[0])) ||
	    (size > stFlashDataTable[message->data[1]].dwBytesize)                     ||
	    (size == 0)                                                                ||
	    (data == NULL)                                                               )
	{
		if (data != NULL)
		{
			_mem_free(data);
		}
		return;
	}

	address = (uint32_t)stFlashDataTable[message->data[1]].pAddress;
	if 	((address + size) >= FLASHEEPROM_SIZE)
	{
		_mem_free(data);
		return;
	}

	/* write */
	_io_ioctl(flashid, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
	for (i=0; i<size; i++)
	{
		*(uint8_t *)(address + FLASHEEPROM_FLEXRAM_BASE + i) = *(data+i);
		_io_ioctl(flashid, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
	}

	/* compare */
	_io_ioctl(flashid, FLEXNVM_IOCTL_GET_EERDY, &flashstatus);

	if ( (flashstatus == FLASHEEPROM_READ_READY) &&
		 (memcmp(data, (uint8_t *)(address + FLASHEEPROM_FLEXRAM_BASE), size) == 0))
	{
		message->data[0] = FLASHROM_REQUEST_WRITE_RES;
	}

	_mem_free(data);

	return;
}
