#include <string.h>
#include <mqx.h>
#include <bsp.h>
#include <spi.h>
#include <message.h>

#include "SpiEeprom.h"
#include "SpiEeprom_task.h"
#if _DEBUG
#include <assert.h>
#include <stdlib.h>
#endif

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
        #define EEPROM_CHANNEL		"spi0:1"
		#define EEPROM_BAUD_RATE	(5000000)
		#define EEPROM_CLK_MODE			SPI_CLK_POL_PHA_MODE0
		#define EEPROM_TRANSFER_MODE	SPI_DEVICE_MASTER_MODE
		#define EEPROM_ENDIAN			SPI_DEVICE_BIG_ENDIAN
	#endif
#else
	#error Unsupported SPI channel number. Please check settings of BSP_SPI_MEMORY_CHANNEL in BSP.
#endif

static MQX_FILE_PTR SpiEeprom_init(void);
static void SpiEeprom_read_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message);
static void SpiEeprom_write_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message);
static void SpiEeprom_erase_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message);
static void SpiEeprom_isempty_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message);

static _task_id gSpiEepromTID = MQX_NULL_TASK_ID;
static _queue_id gSpiEepromQID = 0;

_task_id SpiEeprom_GetTaskID(void)
{
    return (gSpiEepromTID);
}

_queue_id SpiEeprom_GetQueueID(void)
{
    return (gSpiEepromQID);
}

/* Task */
void SpiEeprom_task(uint32_t dummy)
{
    MQX_FILE_PTR					spifd;		/* SPI EEPROM file ID */
	_queue_id 						queueid;	/* message queue id */
	GROUPWARE_MESSAGE_STRUCT_PTR	receivemsg;	/* structure of received message */
    _queue_number q_no = (_queue_number)dummy;

    /* Initialize SPI EEPROM */
	spifd = SpiEeprom_init();
    if (spifd == NULL)
	{
		_task_destroy(MQX_NULL_TASK_ID);
	}

	/* Initialize message queue */
	queueid = _msgq_open_system(q_no, 0, &_task_ready, _task_get_td(_task_get_id()));
	if (queueid == 0)
	{
		fprintf(stderr, "SpiEeprom_task: Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
		fclose(spifd);
		_task_destroy(MQX_NULL_TASK_ID);
	}
    gSpiEepromQID = queueid;
    gSpiEepromTID = _task_get_id();

	/* task */
	while(1) {

		receivemsg = _msgq_poll(queueid);

		if (receivemsg != NULL)
		{
			switch (receivemsg->data[0])
			{
				case EEPROM_REQUEST_READ:		/* READ */

					SpiEeprom_read_exec(spifd, receivemsg);
					break;

				case EEPROM_REQUEST_WRITE:		/* WRITE */

					SpiEeprom_write_exec(spifd, receivemsg);
					break;

				case EEPROM_REQUEST_ERASE:		/* ERASE */

					SpiEeprom_erase_exec(spifd, receivemsg);
					break;

				case EEPROM_REQUEST_ISEMPTY:	/* ISEMPTY */

					SpiEeprom_isempty_exec(spifd, receivemsg);
					break;

				default:

					receivemsg->data[0] = EEPROM_REQUEST_ERROR_RES;
#if _DEBUG
					abort();
#endif
			}

			receivemsg->header.TARGET_QID = receivemsg->header.SOURCE_QID;
			receivemsg->header.SOURCE_QID = queueid;

			if (_msgq_send(receivemsg) == false)
			{
				if (receivemsg->data[0] == EEPROM_REQUEST_READ_RES)
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

/* Initialize */
static MQX_FILE_PTR SpiEeprom_init(void)
{
	MQX_FILE_PTR spifd;					/* SPI EEPROM file ID */
	uint32_t ioctldata[3] = {0, 0, 0};	/* data i/o buffer for ioctl */

    /* Open the SPI driver */
	spifd = fopen(EEPROM_CHANNEL, NULL);
    if (spifd == NULL)
	{
#if _DEBUG
		fputs("SpiEeprom_init : Error: opening SPI driver\n", stderr);
#endif
		return NULL;
	}

    /* Set baud rate */
	ioctldata[0] = EEPROM_BAUD_RATE;
    if (ioctl(spifd, IO_IOCTL_SPI_SET_BAUD, &ioctldata[0]) != SPI_OK)
    {
#if _DEBUG
		fputs("SpiEeprom_init : Error: SPI baud rate setting error\n", stderr);
#endif
		fclose(spifd);
		return NULL;
    }

	/* Set clock mode */
	ioctldata[0] = EEPROM_CLK_MODE;
	if (ioctl(spifd, IO_IOCTL_SPI_SET_MODE, &ioctldata[0]) != SPI_OK)
    {
#if _DEBUG
		fputs("SpiEeprom_init : Error: SPI click mode setting error\n", stderr);
#endif
		fclose(spifd);
		return NULL;
    }

    /* Set endian */
	ioctldata[0] = SPI_DEVICE_BIG_ENDIAN;
	if (ioctl(spifd, IO_IOCTL_SPI_SET_ENDIAN, &ioctldata[0]) != SPI_OK)
    {
#if _DEBUG
		fputs("SpiEeprom_init : Error: SPI endian setting error\n", stderr);
#endif
		fclose(spifd);
		return NULL;
    }

	/* Set transfer mode */
	ioctldata[0] = EEPROM_TRANSFER_MODE;
	if (ioctl(spifd, IO_IOCTL_SPI_SET_TRANSFER_MODE, &ioctldata[0]) != SPI_OK)
    {
#if _DEBUG
		fputs("SpiEeprom_init : Error: SPI transfer mode setting error\n", stderr);
#endif
		fclose(spifd);
		return NULL;
    }

#if BSPCFG_ENABLE_SPI_STATS
	/* Clear statistics */
	if (ioctl(spifd, IO_IOCTL_SPI_CLEAR_STATS, NULL) != SPI_OK)
	{
#if _DEBUG
		fputs("SpiEeprom_init : Error: SPI status clearing error\n", stderr);
#endif
		fclose(spifd);
		return NULL;
	}
#endif

	return spifd;
}

/* Read from EEPROM */
static void SpiEeprom_read_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	uint8_t	 *eepromdata;	/* data read area */
	uint32_t address;		/* read start address */
	uint32_t size;			/* read size */

#if _DEBUG
	assert(eepromid);
	assert(message);
#endif

	address = message->data[1];
	size    = message->data[2];

	message->data[0] = EEPROM_REQUEST_READ_ERROR;
	message->data[1] = 0;
	message->data[2] = 0;
	message->data[3] = 0;

	if( (size == 0) ||
		(address > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
		(size    > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
	    ((address + size) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) )
	{
		return;
	}

	eepromdata = (uint8_t *)_mem_alloc_system(size);

	if (eepromdata != NULL)
	{
		if (memory_read_data(eepromid, address, size, eepromdata) == size)
		{
			message->data[0] = EEPROM_REQUEST_READ_RES;
			message->data[2] = size;
			message->data[3] = (uint32_t)eepromdata;
		}
		else
		{
			_mem_free(eepromdata);
		}
	}

	return;
}

/* Write to EEPROM */
static void SpiEeprom_write_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	GROUPWARE_MESSAGE_STRUCT	temp;		/* temporal message data for erase */
	uint32_t 					address;	/* write start address */
	uint32_t					size;		/* write size */
	uint8_t						*data;		/* write data pointer */

#if _DEBUG
	assert(eepromid);
	assert(message);
#endif

	address = message->data[1];
	size    = message->data[2];
	data    = (uint8_t *)message->data[3];

	message->data[0] = EEPROM_REQUEST_WRITE_ERROR;
	message->data[1] = 0;
	message->data[2] = 0;
	message->data[3] = 0;

	if( (size == 0) ||
		(address > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
		(size    > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
	    ((address + size) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) )
	{
		if (data != NULL)
		{
			_mem_free(data);
		}
		return;
	}

	temp.data[1] = address;
	temp.data[2] = size;

	/* check writing area is empty */
	SpiEeprom_isempty_exec(eepromid, &temp);

	/* write */
	if ((temp.data[0] == EEPROM_REQUEST_ISEMPTY_RES) &&
		(memory_write_data(eepromid, address, size, data) == size))
	{
		message->data[0] = EEPROM_REQUEST_WRITE_RES;
	}

	_mem_free(data);

	return;
}

/* 石の仕様上、セクター単位の消去しかできない */
/* Erase EEPROM sector data */
static void SpiEeprom_erase_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	uint32_t startsector;	/* erase start sector */
	uint32_t endsector;		/* erase end sector */
	uint32_t i;				/* roop counter */

#if _DEBUG
	assert(eepromid);
	assert(message);
#endif

	startsector = message->data[1];
	endsector   = message->data[2];

	message->data[1] = 0;
	message->data[2] = 0;

	if ((startsector <= endsector) && (endsector < SPI_MEMORY_SECTOR_NUMBER))
	{
		for(i=startsector; i<=endsector; i++)
		{
			if (memory_sector_erase(eepromid, i) == false)
			{
				break;
			}
		}

		if (i > endsector)
		{
			message->data[0] = EEPROM_REQUEST_ERASE_RES;
		}
		else
		{
			message->data[0] = EEPROM_REQUEST_ERASE_ERROR;
		}
	}
	else
	{
		message->data[0] = EEPROM_REQUEST_ERASE_ERROR;
	}

	return;
}

/* Empty check */
static void SpiEeprom_isempty_exec(MQX_FILE_PTR eepromid, GROUPWARE_MESSAGE_STRUCT_PTR message)
{
	GROUPWARE_MESSAGE_STRUCT	temp;		/* temporal message data for read */
	uint32_t					address;	/* write start address */
	uint32_t					size;		/* write size */
	uint32_t					i;			/* roop counter */

#if _DEBUG
	assert(eepromid);
	assert(message);
#endif

	address = message->data[1];
	size    = message->data[2];

	message->data[0] = EEPROM_REQUEST_ISEMPTY_ERROR;
	message->data[1] = 0;
	message->data[2] = 0;

	if( (size == 0) ||
		(address > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
		(size    > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
	    ((address + size) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) )
	{
		return;
	}

	temp.data[1] = address;
	temp.data[2] = size;

	/* check the area is empty */
	SpiEeprom_read_exec(eepromid, &temp);

	if ((temp.data[0] == EEPROM_REQUEST_READ_RES) &&
		(temp.data[2] == size)                    &&
		(temp.data[3] != 0)                         )
	{
		for (i=0; i<size; i++)
		{
			if (*((uint8_t *)(temp.data[3]+i)) != 0xFF)
			{
				break;
			}
		}

		if (i == size)
		{
			message->data[0] = EEPROM_REQUEST_ISEMPTY_RES;
		}

		_mem_free((void *)temp.data[3]);
	}

	return;
}
