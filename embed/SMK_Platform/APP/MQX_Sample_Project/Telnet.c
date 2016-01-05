#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <ipcfg.h>
#include <shell.h>
#include <message.h>
#include <ctype.h>
#include <assert.h>

#include "Config.h"
#include "Tasks.h"

#include "Telnet.h"

#include "shell_command.h"

#include "storage_api.h"
#include "spi_memory.h"

#if APP_USE_LED_TASK
#if (GREEN_LED_EXIST != 0) || (ORANGE_LED_EXIST != 0)
#include "led_task.h"
#endif
#endif

#if JEMA_Control_EXIST
#include "HaControl.h"
#endif

#if RAIN_SENSOR_EXIST
#include "RainSensor.h"
#endif

#include "flash_utils.h"

#define SHELL_TELNETD_TIMEOUT		5000	/* Timeout period(ms) */

#if ! MQX_ENABLE_LOW_POWER
#error This application requires MQX_ENABLE_LOW_POWER to be defined non-zero in user_config.h. Please recompile BSP with this option.
#endif

#if ! MQX_USE_IDLE_TASK
#error This application requires MQX_USE_IDLE_TASK to be defined non-zero in user_config.h. Please recompile BSP with this option.
#endif

//=============================================================================
/* Shell function */
static void Telnetd_shell_fn(void *dummy);

//=============================================================================
/* Shell command function */
static int32_t flash_test(int32_t argc, char *argv[]);
#if APP_USE_LED_TASK
#if (GREEN_LED_EXIST || ORANGE_LED_EXIST)
static int32_t led_test(int32_t argc, char *argv[]);
#endif
#endif
#if JEMA_Control_EXIST
static int32_t jema_test(int32_t argc, char *argv[]);
#endif
#if (UART_1_EXIST || RS232C_EXIST)
static int32_t uart_test(int32_t argc, char *argv[]);
#endif
#if RAIN_SENSOR_EXIST
static int32_t rain_test(int32_t argc, char *argv[]);
#endif
static int32_t sleep_test(int32_t argc, char *argv[]);
static int32_t help_test(int32_t argc, char *argv[]);
#if (UART_1_DEBUG || RS232C_DEBUG)
static int32_t uartspeed_test(int32_t argc, char *argv[]);
#endif

//=============================================================================
/* other function */
static GROUPWARE_MESSAGE_STRUCT_PTR initialize_message(_queue_id sourceqid, _queue_number targetqnum);
static GROUPWARE_MESSAGE_STRUCT_PTR send_receive_message(_queue_id queueid, GROUPWARE_MESSAGE_STRUCT_PTR sendmsg, uint32_t timeout);

//=============================================================================
/* command table */
const SHELL_COMMAND_STRUCT Shell_commands[] = {
	{ "flash",		flash_test   },
#if APP_USE_LED_TASK
#if (GREEN_LED_EXIST || ORANGE_LED_EXIST)
	{ "led",		led_test     },
#endif
#endif
#if JEMA_Control_EXIST
	{ "jema",		jema_test    },
#endif
#if (UART_1_EXIST || RS232C_EXIST)
	{ "uart",		uart_test    },
#endif
#if RAIN_SENSOR_EXIST
	{ "rain",		rain_test    },
#endif
	{ "sleep",		sleep_test   },
	{ "eeprom",		Shell_eeprom },
	{ "update",		Shell_update  },
	{ "version",	Shell_version },
	{ "info",       Shell_info   },
	{ "help",		help_test    },
#if (UART_1_DEBUG || RS232C_DEBUG)
	{ "uspeed",		uartspeed_test },
#endif

	{ "reset",      Shell_reset },
	{ "exit",       Shell_exit  },

    { NULL,			NULL         }
};

//=============================================================================
/* Task setting */
static RTCS_TASK Telnetd_shell_template = {"Telnet_shell", 8, 5000, Telnetd_shell_fn, NULL};

//=============================================================================
/* help message */
static const char sUsage[]          = "Usage : ";
static const char sFlashCmdHelp[]   = "flash read <address> <length>\\n\n"
                                         "flash write <address> <data>\\n\n"
										 "flash testall\\n\n"
										 "flash erase (<sector number> <sector number>)\\n\n";
#if APP_USE_LED_TASK
#if (GREEN_LED_EXIST && ORANGE_LED_EXIST)
static const char sLedCmdHelp[]     = "led [G|O] (ON|OFF|BLINK)\\n\n";
#elif GREEN_LED_EXIST
static const char sLedCmdHelp[]     = "led G (ON|OFF|BLINK)\\n\n";
#elif ORANGE_LED_EXIST
static const char sLedCmdHelp[]     = "led O (ON|OFF|BLINK)\\n\n";
#else
static const char sLedCmdHelp[]     = "";
#endif
#endif

#if JEMA_Control_EXIST
static const char sJemaCmdHelp[]    = "jema [1|2] (ON|OFF)\\n\n";
#else
static const char sJemaCmdHelp[]    = "";
#endif

#if (UART_1_EXIST && RS232C_EXIST)
static const char sUartCmdHelp[]    = "uart [1|4] (<data>)\\n\n";
#elif UART_1_EXIST
static const char sUartCmdHelp[]    = "uart 1 (<data>)\\n\n";
#elif RS232C_EXIST
static const char sUartCmdHelp[]    = "uart 4 (<data>)\\n\n";
#else
static const char sUartCmdHelp[]    = "";
#endif

#if RAIN_SENSOR_EXIST
static const char sRainCmdHelp[]    = "rain\\n\n";
#else
static const char sRainCmdHelp[]    = "";
#endif

static const char sSleepCmdHelp[]   = "sleep\\n\n";
static const char sEepromCmdHelp[]  = "eeprom [mac|code|serial|ftpid|ftpsubd|ftppath|dhcp|ip|netmask|gateway] (<data>)\\n\n";
static const char sUpdateCmdHelp[]  = "update (<last 2 byte of IP>)\\n\n";
static const char sVersionCmdHelp[] = "version\\n\n";
#if (UART_1_DEBUG && RS232C_DEBUG)
static const char sUspeedCmdHelp[]  = "uspeed [1|4] ([2400|4800|9600|14400|19200|38400|57600|115200])\\n\n";
#elif UART_1_DEBUG
static const char sUspeedCmdHelp[]  = "uspeed 1 ([2400|4800|9600|14400|19200|38400|57600|115200])\\n\n";
#elif RS232C_DEBUG
static const char sUspeedCmdHelp[]  = "uspeed 4 ([2400|4800|9600|14400|19200|38400|57600|115200])\\n\n";
#else
static const char sUspeedCmdHelp[]  = "";
#endif
static const char sHelpCmdHelp[]    = "help\\n\n";

//=============================================================================
/* UART Baud rate */
#if UART_1_DEBUG
static uint32_t	ulUart1BaudRate = BSPCFG_SCI1_BAUD_RATE;
#endif

//=============================================================================
static void Telnetd_shell_fn (void *dummy)
{
	char name[32] = {0};		/* login name */
	char pass[32] = {0};		/* login password */
 	_mqx_int i = 0;				/* roop count */
 	_mqx_int errorcount = 0;	/* login error time count */

	bool connected = false;

	do
	{
		puts("login: ");
		i = 0;
		do
		{
			name[i] = getchar();
			i++;
		} while((name[i-1] != '\n') && (i < sizeof(name)));

		name[i-1] = '\0';

		puts("password: ");
		i = 0;
		do
		{
			pass[i] = getchar();
			puts("\b \b");	/* 一応ソーシャルハック防止 */
			i++;
		} while((pass[i-1] != '\n') && (i < sizeof(pass)));

		pass[i-1] = '\0';

		if ((strcmp(name, SHELL_COMMAND_USERNAME) == 0) && (strcmp(pass, SHELL_COMMAND_PASSWORD) == 0))
		{
			connected = true;
			Shell(Shell_commands, NULL);
			break;
		}

		puts("login failed\n\n");
		memset(name, 0, sizeof(name));
		memset(pass, 0, sizeof(pass));
		errorcount++;

	} while (errorcount < 3);

	if (!connected) {
		puts("abort connection\n");
		fflush(stdout);
		_time_delay(3000);
	}

	return;
}

//=============================================================================
_mqx_int initialize_Telnet(void)
{
	return TELNETSRV_init(SHELL_TELNETD_TASK_NAME, SHELL_TELNETD_PRIORITY, SHELL_TELNETD_STACK, &Telnetd_shell_template);
}

//=============================================================================
static int32_t flash_test(int32_t argc, char *argv[])
{
	/* check */
	if ((argc != 2) && (argc != 4))
	{
		printf("%s%s", sUsage, sFlashCmdHelp);
		return 0;
	}

	/* operand1 : command */
	if ((strcmp(argv[1], "read") == 0) && (argc == 4))	/* read data */
	{
		uint32_t address;
		uint32_t length = 0;
		_mqx_int i;
		uint8_t  *readdata;

		/* check address data */
		if ((*argv[2] == '0') && ((char)tolower((int)*(argv[2]+1)) == 'x'))
		{
			/* hexadecimal */
			for(i=2; i<strlen(argv[2]); i++)
			{
				if(isxdigit((int)(*(argv[2]+i))) == 0)
				{
					puts("Error : address error\n");
					return 0;
				}
			}

			sscanf(argv[2], "%x", &address);
		}
		else
		{
			/* decimal */
			for(i=0; i<strlen(argv[2]); i++)
			{
				if(isdigit((int)(*(argv[2]+i))) == 0)
				{
					puts("Error : address error\n");
					return 0;
				}
			}

			address = atoi(argv[2]);
		}

		/* check length data */
		for(i=0; i<strlen(argv[3]); i++)
		{
			if(isdigit((int)(*(argv[3]+i))) == 0)
			{
				break;
			}
		}

		if (i == strlen(argv[3]))
		{
			length = atoi(argv[3]);
		}

		if ( ((address + length) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
			 (length > 4096)                                                            ||
			 (length == 0)                                                                 )
		{
			puts("Error : length data error\n");
			return 0;
		}

		MQX_FILE *fd;
		uint32_t read_size;

		readdata = _mem_alloc(length);
		if (readdata == NULL) {
			puts("Error : memory exhausted\n");
			return (0);
		}

		fd = Storage_Open();
		if (fd == NULL) {
			puts("Error : storage open failed\n");
			_mem_free(readdata);
			return (0);
		}

		read_size = Storage_Read(fd, address, length, readdata);
		Storage_Close(fd);

		if (read_size == length)
		{
			_mqx_int j;

			for(i=0; i<length; i+=16)
			{
				printf("0x%05X | ", address+i);

				for (j=0; j<16; j++)
				{
					if ((i+j) < length)
					{
						printf("%02X ", *(readdata + i + j));
					}
					else
					{
						puts("   ");
					}
				}

				puts("| ");

				for (j=0; j<16; j++)
				{
					if ((i+j) < length)
					{
						if (isprint((int)(*(readdata + i + j))) != 0)
						{
							printf("%c", *((char *)(readdata + i + j)));
						}
						else
						{
							puts(".");
						}
					}
					else
					{
						break;
					}
				}

				puts("\n");
			}
		}
		else
		{
			puts("flash_test : Read error\n");
		}

		_mem_free(readdata);
	}
	else if ((strcmp(argv[1], "write") == 0) && (argc == 4))	/* write data */
	{
		uint32_t address;
		uint32_t length;
		uint8_t  *writedata;
		_mqx_int i;

		/* check address data */
		if ((*argv[2] == '0') && ((char)tolower((int)*(argv[2]+1)) == 'x'))
		{
			/* hexadecimal */
			for(i=2; i<strlen(argv[2]); i++)
			{
				if(isxdigit((int)(*(argv[2]+i))) == 0)
				{
					puts("Error : address error\n");
					return 0;
				}
			}

			sscanf(argv[2], "%x", &address);
		}
		else
		{
			/* decimal */
			for(i=0; i<strlen(argv[2]); i++)
			{
				if(isdigit((int)(*(argv[2]+i))) == 0)
				{
					puts("Error : address error\n");
					return 0;
				}
			}

			address = atoi(argv[2]);
		}

		if ((address & (SPI_MEMORY_PAGE_SIZE-1)) != 0)
		{
			puts("flash_test : Start address should be the multiple of 0x100.\n");
			return 0;
		}

		/* check data length */
		length = strlen(argv[3]);

		if ( ((address + length) > (SPI_MEMORY_SECTOR_SIZE * SPI_MEMORY_SECTOR_NUMBER)) ||
			 (length > 4096)                                                              )
		{
			puts("Error : Input data error\n");
			return 0;
		}

		MQX_FILE *fd;
		uint32_t write_size;

		fd = Storage_Open();
		if (fd == NULL) {
			puts("Error : storage open failed\n");
			return (0);
		}

		write_size = Storage_Write(fd, address, length, (const uint8_t *)argv[3]);
		Storage_Close(fd);

		if (write_size == length) {
			puts("flash_test : Write compelete\n");
		} else {
			puts("flash_test : Write error\n");
		}
	}
	else if ((strcmp(argv[1], "testall") == 0) && (argc == 2))	/* whole read/write test */
	{
		_mqx_int testnum = 1;

		puts("\nEEPROM read/write test start\nerasing data....\n");
		fflush(stdout);

		MQX_FILE *fd;

		fd = Storage_Open();
		if (fd == NULL) {
			puts("Error : storage open failed\n");
			return (0);
		}

		if (!Storage_EraseSector(fd, 0, (SPI_MEMORY_SECTOR_NUMBER - 1))) {
			puts("flash_test : Erase failed.\n");
			testnum = 0;
		}

		for (; testnum <= 2; testnum++)
		{
			_mqx_int sector;
			int testdata = (testnum == 1) ? 0xA5 : 0x5A;

			if (testnum == 0)
			{
				break;
			}

			for (sector = 0; sector < SPI_MEMORY_SECTOR_NUMBER; sector++)
			{
			 	_mqx_int page;

				printf("flash_test : Sector %d Test%d Start\n", sector, testnum);
				fflush(stdout);

				for (page=0; page<(SPI_MEMORY_SECTOR_SIZE/SPI_MEMORY_PAGE_SIZE); page++)
				{
					uint8_t writedata[SPI_MEMORY_PAGE_SIZE];
					uint8_t readdata[SPI_MEMORY_PAGE_SIZE];
					uint32_t startaddress = SPI_MEMORY_SECTOR_SIZE * sector + SPI_MEMORY_PAGE_SIZE * page;
					uint32_t read_size, write_size;

					/* write data */
					memset(&writedata[0], testdata, SPI_MEMORY_PAGE_SIZE);

					write_size = Storage_Write(fd, startaddress,
											   SPI_MEMORY_PAGE_SIZE,
											   writedata);

					if (write_size != SPI_MEMORY_PAGE_SIZE) {
						puts("flash_test : Write error\n");
						break;
					}

					/* read data */
					read_size = Storage_Read(fd, startaddress,
											 SPI_MEMORY_PAGE_SIZE,
											 readdata);

					if (read_size != SPI_MEMORY_PAGE_SIZE) {
						puts("flash_test : Read error\n");
						break;
					}

					/* compare */
					if (memcmp(readdata, writedata, SPI_MEMORY_PAGE_SIZE) != 0) {
						puts("flash_test : Compare error\n");
						break;
					}
				}

				printf("flash_test : Sector %d Test%d ", sector, testnum);
				if (page == (SPI_MEMORY_SECTOR_SIZE/SPI_MEMORY_PAGE_SIZE))
				{
					puts("OK\n");
				}
				else
				{
					puts("NG\n");
					break;
				}
			}

			puts("erasing data....\n");
			fflush(stdout);

			if (!Storage_EraseSector(fd, 0, (SPI_MEMORY_SECTOR_NUMBER - 1))) {
				puts("flash_test : Erase failed.\n");
				break;
			}

			if (sector < SPI_MEMORY_SECTOR_NUMBER)
			{
				break;
			}
		}

		Storage_Close(fd);

		if (testnum == 3)
		{
			puts("flash_test : Test compelete\n");
		}
		else
		{
			puts("flash_test : Test failed\n");
		}
	}
	else if (strcmp(argv[1], "erase") == 0)	/* erase chip */
	{
		uint32_t startsector = 0xFFFFFFFF;
		uint32_t endsector   = 0xFFFFFFFF;

		/* erasing sectors */
		if (argc == 2)
		{
			startsector = 0;
			endsector   = (SPI_MEMORY_SECTOR_NUMBER - 1);
			puts("flash_test : erase all sectors\n");
		}
		else //if (argc == 4)
		{
			_mqx_int i;

			/* start sector */
			for(i=0; i<strlen(argv[2]); i++)
			{
				if(isdigit((int)(*(argv[2]+i))) == 0)
				{
					break;
				}
			}

			if (i == strlen(argv[2]))
			{
				startsector = atoi(argv[2]);
			}

			/* end sector */
			for(i=0; i<strlen(argv[3]); i++)
			{
				if(isdigit((int)(*(argv[3]+i))) == 0)
				{
					break;
				}
			}

			if (i == strlen(argv[3]))
			{
				endsector = atoi(argv[3]);
			}

			if ( (startsector >= SPI_MEMORY_SECTOR_NUMBER) ||
				 (endsector > startsector)                   )
			{
				puts("flash_test : Sector data error\n");
				return 0;
			}

			printf("flash_test : erase start sector : %d\n", startsector);
			printf("flash_test : erase end sector   : %d\n", endsector);
		}

		printf("flash_test : This spends about %d seconds.\n", (endsector-startsector+1) * SPI_MEMORY_DELAY_SECTORERASE / 1000);
		fflush(stdout);

		MQX_FILE *fd;

		fd = Storage_Open();
		if (fd == NULL) {
			puts("Error : storage open failed\n");
			return (0);
		}

		if (Storage_EraseSector(fd, startsector, endsector)) {
			puts("flash_test : Erase compelete.\n");
		} else {
			puts("flash_test : Erase failed.\n");
		}

		Storage_Close(fd);
	}
	else
	{
		printf("%s%s", sUsage, sFlashCmdHelp);
	}

	return 0;
}

//=============================================================================
#if APP_USE_LED_TASK
#if (GREEN_LED_EXIST || ORANGE_LED_EXIST)
static int32_t led_test(int32_t argc, char *argv[])
{
	uint8_t led_number;
	uint8_t mode;

	/* check input */
	if ((argc > 3)	|| (argc <= 1) || (strlen(argv[1]) != 1))
	{
		printf("%s%s", sUsage, sLedCmdHelp);
		return 0;
	}

	/* operand1 : target */
#if GREEN_LED_EXIST
	if (tolower((int)*argv[1]) == 'g')
	{
		led_number = LED_GREEN;
	}
	else
#endif
#if ORANGE_LED_EXIST
	if (tolower((int)*argv[1]) == 'o')
	{
		led_number = LED_ORANGE;
	}
	else
#endif
	{
		printf("%s%s", sUsage, sLedCmdHelp);
		return (0);
	}

	/* operand2 : command */
	if (argc == 3)
	{
		int i;

		for (i=0; i<strlen(argv[2]); i++)
		{
			*(argv[2]+i) = (char)(tolower((int)*(argv[2]+i)));
		}

		if (strcmp(argv[2], "on") == 0)
		{
			mode = LED_MODE_ON;
		}
		else if (strcmp(argv[2], "off") == 0)
		{
			mode = LED_MODE_OFF;
		}
		else if (strcmp(argv[2], "blink") == 0)
		{
			mode = LED_MODE_BLINK_FAST;
		}
		else
		{
			printf("%s%s", sUsage, sLedCmdHelp);
			return (0);
		}

		LED_SetMode(led_number, mode);
	}
	else /* argc == 2 */
	{
		mode = LED_GetMode(led_number);
	}

	switch (led_number) {
	case LED_GREEN:
		printf("Green LED ");
		break;
	case LED_ORANGE:
		printf("Orange LED ");
		break;
	default:
		break;
	}

	switch (mode) {
	case LED_MODE_ON:
		printf("status : ON\n");
		break;
	case LED_MODE_OFF:
		printf("status : OFF\n");
		break;
	case LED_MODE_BLINK_FAST:
	case LED_MODE_BLINK_SLOW:
		printf("status : BLINK\n");
		break;
	default:
		printf(": Message Error\n");
		break;
	}

	return (0);
}
#endif
#endif

//=============================================================================
#if JEMA_Control_EXIST
static int32_t jema_test(int32_t argc, char *argv[])
{
	uint8_t ha_number;
	HaState_t ha_state, cur_state;

	/* check */
	if( (argc <= 1) ||
	    (argc > 3)  ||
	    (strlen(argv[1]) != 1) ||
		((*argv[1] != '1') && (*argv[1] != '2'))  )
	{
		printf("%s%s", sUsage, sJemaCmdHelp);
		return 0;
	}

	/* operand1 : target */
	if (*argv[1] == '1')
	{
		ha_number = 0;
	}
	else //if (*argv[1] == '2')
	{
		ha_number = 1;
	}

	/* operand2 : command */
	if (argc == 3)
	{
		int i;

		for (i=0; i<strlen(argv[2]); i++)
		{
			*(argv[2]+i) = (char)(tolower((int)*(argv[2]+i)));
		}

		if(strcmp(argv[2], "on") == 0)
		{
			ha_state = HA_STATE_ON;
		}
		else if(strcmp(argv[2], "off") == 0)
		{
			ha_state = HA_STATE_OFF;
		}
		else
		{
			puts(sUsage);
			puts(sJemaCmdHelp);
			return 0;
		}

		cur_state = HaControl_ChangeState(ha_number, ha_state);

		printf("jema : ");
		if (cur_state == ha_state) {
			switch (cur_state) {
			case HA_STATE_ON:
				printf("ON\n");
				break;
			case HA_STATE_OFF:
				printf("OFF\n");
				break;
			default:
				printf("Control Error\n");
				break;
			}
		} else {
			printf("Control Error\n");
		}
	}
	else /* argc == 2 */
	{
		cur_state = HaControl_GetState(ha_number);

		printf("jema : ");
		switch (cur_state) {
		case HA_STATE_ON:
			printf("ON\n");
			break;
		case HA_STATE_OFF:
			printf("OFF\n");
			break;
		default:
			printf("Status Error\n");
			break;
		}
	}

	return (0);
}
#endif

//=============================================================================
#if (UART_1_EXIST || RS232C_EXIST)
static int32_t uart_test(int32_t argc, char *argv[])
{
	MQX_FILE_PTR	uart;										/* UART file pointer */
	char			*senddata;									/* send data */
	char			*receivedata;								/* receive data */
	const char		defaulttestdata[] = SHELL_UART_TEST_DATA;	/* default test data */
	MQX_TICK_STRUCT	currenttime;								/* current tick time */
	MQX_TICK_STRUCT	starttime;									/* start tick time for blink */
	bool			overflow;									/* dummy variable for _time_diff function */
	int				i = 0;										/* byte count */
	uint32_t		buffersize;									/* max buffer size of select uart port */
	uint32_t		restbuffsize;								/* rest capacity of buffer size */
#if RS232C_EXIST
	char			uart4flag;									/* current I/O Flag of UART4 */
#endif

	if ( (argc < 2) ||
	     (argc > 3) ||
		 (strlen(argv[1]) != 1) ||
		 (
#if UART_1_EXIST
		  (*argv[1] != '1')
#if RS232C_EXIST
		  &&
#endif
#endif
#if RS232C_EXIST
		  (*argv[1] != '4')
#endif
		 )
	   )
	{
		puts(sUsage);
		puts(sUartCmdHelp);
		return 0;
	}

	/* device open */
	if (*argv[1] == '1')
	{
#if UART_1_EXIST
		uart = fopen("ittyb:", NULL);
		buffersize = BSPCFG_SCI1_QUEUE_SIZE;
#if UART_1_DEBUG
		if (uart != NULL)
		{
			ioctl(uart, IO_IOCTL_SERIAL_SET_BAUD, (void *)&ulUart1BaudRate);
		}
#endif
#endif
	}
	else // (*argv[1] == '4')
	{
#if RS232C_EXIST
		uart = fopen("ittye:", NULL);
		buffersize = BSPCFG_SCI4_QUEUE_SIZE;
		if (uart != NULL)
		{
			ioctl(uart, IO_IOCTL_SERIAL_GET_FLAGS, (void *)&uart4flag);
			ioctl(uart, IO_IOCTL_SERIAL_SET_FLAGS, NULL);
		}
#endif
	}

	if (uart == NULL)
	{
		puts("uart_test: UART Filesystem open failed.\n");
		return 0;
	}

	/* data */
	if (argc == 2)
	{
		senddata = (char *)&defaulttestdata[0];
	}
	else // argc == 3
	{
		if (strlen(argv[2]) > buffersize)
		{
			puts("uart_test: Too long data\n");
			return 0;
		}
		senddata = argv[2];
	}

	receivedata = (char *)_mem_alloc_system_zero(strlen(senddata)*2);
	if(receivedata == NULL)
	{
		puts("uart_test: Receive buffer allocation error\n");
		return 0;
	}

	/* test */
	fflush(uart);
	write(uart, senddata, strlen(senddata));

	_time_get_ticks(&starttime);
	do
	{
		_time_delay(1);
		_time_get_ticks(&currenttime);

	} while(_time_diff_milliseconds(&currenttime, &starttime, &overflow) < 100);

	/* IO_IOCTL_SERIAL_CAN_RECEIVEで出力されるデータの内容がI/O reference manualの記述と違う */
	/* 実際は受信バッファの残り容量を出力する模様 */
	ioctl(uart, IO_IOCTL_SERIAL_CAN_RECEIVE, &restbuffsize);
	while((restbuffsize < buffersize) && (i < (strlen(senddata)*2)))
	{
		*(receivedata+i) = (char)fgetc(uart);
		ioctl(uart, IO_IOCTL_SERIAL_CAN_RECEIVE, &restbuffsize);
		i++;
	}

	if(restbuffsize < buffersize)
	{
		volatile char dummy;

		while(restbuffsize < buffersize)
		{
			dummy = (char)fgetc(uart);
			ioctl(uart, IO_IOCTL_SERIAL_CAN_RECEIVE, &restbuffsize);
		}
	}

	/* output result */
	printf("UART %s Test : Send    : %s\n", argv[1], senddata);
	printf("UART %s Test : Receive : %s\n", argv[1], receivedata);
	printf("UART %s Test : ", argv[1]);
	if (strcmp(senddata, receivedata) == 0)
	{
		puts("OK\n");
	}
	else
	{
		puts("NG\n");
	}

	_mem_free(receivedata);

	fflush(uart);
#if RS232C_EXIST
	if (*argv[1] == '4')
	{
		ioctl(uart, IO_IOCTL_SERIAL_SET_FLAGS, (void *)&uart4flag);
	}
#endif
	fclose(uart);

	return 0;
}
#endif

//=============================================================================
#if RAIN_SENSOR_EXIST
static int32_t rain_test(int32_t argc, char *argv[])
{
 	GROUPWARE_MESSAGE_STRUCT_PTR	sendmsg;		/* send message */
 	GROUPWARE_MESSAGE_STRUCT_PTR	receivemsg;		/* receive message */
	_queue_id						queueid;		/* message queue ID */

	/* check input */
	if (argc > 2)
	{
		printf("%s%s", sUsage, sRainCmdHelp);
		return 0;
	}

	/* open queue */
	queueid = _msgq_open_system(MSGQ_FREE_QUEUE, 0, NULL, 0);
	if (queueid ==  MSGQ_NULL_QUEUE_ID)
	{
		printf("rain_test : Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
		return 0;
	}

	/* create message */
	sendmsg = initialize_message(queueid, RAIN_SENSOR_MESSAGE_QUEUE);
	if (sendmsg == NULL)
	{
		_msgq_close(queueid);
		return 0;
	}
	sendmsg->data[0] = RAIN_SENSOR_CMD_STATUS;

	/* send message & receive response */
	receivemsg = send_receive_message(queueid, sendmsg, SHELL_TELNETD_TIMEOUT);
	if (receivemsg == NULL)
	{
		_msgq_close(queueid);
		return 0;
	}

	/* output result */
	puts("rain_test : ");
	if ((receivemsg->data[0] == RAIN_SENSOR_CMD_STATUS_RES) && (receivemsg->data[1] == RAIN_SENSOR_STATUS_RAINY))
	{
		puts("detect rain\n");
	}
	else if ((receivemsg->data[0] == RAIN_SENSOR_CMD_STATUS_RES) && (receivemsg->data[1] == RAIN_SENSOR_STATUS_NORAIN))
	{
		puts("no rain\n");
	}
	else
	{
		puts("Message Error\n");
	}

	_msg_free(receivemsg);
	_msgq_close(queueid);
	return 0;
}
#endif

//=============================================================================
static int32_t sleep_test(int32_t argc, char *argv[])
{
	char cmddata1[] = "dummy";
	char cmddata2[] = "G";
	char cmddata3[] = "O";
	char cmddata4[] = "OFF";

	char *cmd[4] = {cmddata1};

	_task_id closeid;

	puts("Sleep start. Reset afterwords.\n");
	fflush(stdout);

#if RAIN_SENSOR_EXIST
	/* kill rain sensor */
	closeid = _task_get_id_from_name(RAIN_SENSOR_TASK_NAME);
	if (closeid != MQX_NULL_TASK_ID)
	{
		_task_destroy(closeid);
	}
#endif

	/* kill SPI EEPROM */
	((PORT_MemMapPtr)PORTD_BASE_PTR)->PCR[0] = 0;
	((PORT_MemMapPtr)PORTD_BASE_PTR)->PCR[1] = 0;
	((PORT_MemMapPtr)PORTD_BASE_PTR)->PCR[2] = 0;
	((PORT_MemMapPtr)PORTD_BASE_PTR)->PCR[3] = 0;

#if APP_USE_LED_TASK
#if GREEN_LED_EXIST
	LED_SetMode(LED_GREEN, LED_MODE_OFF);
#endif
#if ORANGE_LED_EXIST
	LED_SetMode(LED_ORANGE, LED_MODE_OFF);
#endif
#endif

#if UART_1_EXIST
	/* close UART1 */
	((PORT_MemMapPtr)PORTC_BASE_PTR)->PCR[3] = 0;
	((PORT_MemMapPtr)PORTC_BASE_PTR)->PCR[4] = 0;
#endif

#if RS232C_EXIST
	/* close UART4 */
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[24] = 0;
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[25] = 0;
#endif

	/* terminate Ethernet */
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[12] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[13] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[14] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[15] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[16] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[17] = 0;
	((PORT_MemMapPtr)PORTB_BASE_PTR)->PCR[0] = 0;
	((PORT_MemMapPtr)PORTB_BASE_PTR)->PCR[1] = 0;

	/* terminate JTAG */
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[0] = 0;
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[1] = 0;
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[2] = 0;
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[3] = 0;
	((PORT_MemMapPtr)PORTE_BASE_PTR)->PCR[4] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[0] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[1] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[2] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[3] = 0;
	((PORT_MemMapPtr)PORTA_BASE_PTR)->PCR[4] = 0;

	_lpm_set_operation_mode(LPM_OPERATION_MODE_STOP);

	/* may be unused */
	return 0;
}

#if (UART_1_DEBUG || RS232C_DEBUG)
static int32_t uartspeed_test(int32_t argc, char *argv[])
{
	uint32_t		*baud;		/* current baud rate */
#if RS232C_DEBUG
	MQX_FILE_PTR	uart;		/* UART file pointer */
	uint32_t		uart4baud;
#endif

	if ( (argc < 2) ||
	     (argc > 3) ||
		 (strlen(argv[1]) != 1) ||
		 (
#if UART_1_DEBUG
		  (*argv[1] != '1')
#if RS232C_DEBUG
		  &&
#endif
#endif
#if RS232C_DEBUG
		  (*argv[1] != '4')
#endif
		 )
	   )
	{
		puts(sUsage);
		puts(sUspeedCmdHelp);
		return 0;
	}

	/* device open */
	if (*argv[1] == '1')
	{
#if UART_1_DEBUG
		baud = &ulUart1BaudRate;
#endif
	}
	else // (*argv[1] == '4')
	{
#if RS232C_DEBUG
		uart = fopen("ittye:", NULL);
		baud = &uart4baud;

		if (uart == NULL)
		{
			puts("NG UART Filesystem open failed.\n");
			return 0;
		}
#endif
	}

	if (argc == 2)
	{
#if RS232C_DEBUG
		if (*argv[1] == '4')
		{
			ioctl(uart, IO_IOCTL_SERIAL_GET_BAUD, (void *)baud);
		}
#endif
		printf("UART %c Baud rate : %d\n", *argv[1], *baud);
	}
	else /* argc == 3 */
	{
		if( (strcmp(argv[2], "2400")   == 0) ||
	  		(strcmp(argv[2], "4800")   == 0) ||
	  		(strcmp(argv[2], "9600")   == 0) ||
	  		(strcmp(argv[2], "14400")  == 0) ||
	  		(strcmp(argv[2], "19200")  == 0) ||
	  		(strcmp(argv[2], "38400")  == 0) ||
	  		(strcmp(argv[2], "57600")  == 0) ||
	  		(strcmp(argv[2], "115200") == 0)   )
		{
			*baud = atoi(argv[2]);
#if RS232C_DEBUG
			if (*argv[1] == '4')
			{
				ioctl(uart, IO_IOCTL_SERIAL_SET_BAUD, (void *)baud);
			}
#endif
			printf("OK UART %c Baud rate in test set to %d.\n", *argv[1], *baud);
		}
		else
		{
			puts("NG Unsupport baud rate\n");
		}
	}

	return 0;
}
#endif


//=============================================================================
static int32_t help_test(int32_t argc, char *argv[])
{
  	printf("Command list\n%s%s%s%s%s%s%s%s%s%s%s",	sFlashCmdHelp,
		   											sLedCmdHelp,
													sJemaCmdHelp,
													sUartCmdHelp,
													sRainCmdHelp,
													sSleepCmdHelp,
													sEepromCmdHelp,
													sUpdateCmdHelp,
													sVersionCmdHelp,
													sUspeedCmdHelp,
													sHelpCmdHelp);
	return 0;
}

//=============================================================================
static GROUPWARE_MESSAGE_STRUCT_PTR initialize_message(_queue_id sourceqid, _queue_number targetqnum)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	message;	/* initialized message */

	assert(sourceqid != MSGQ_NULL_QUEUE_ID);
	assert(targetqnum);

	message = (GROUPWARE_MESSAGE_STRUCT_PTR)_msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT));
	if (message == NULL)
	{
		puts("Error : Message allocation failed.\n");
		return NULL;
	}

	message->header.SIZE       = sizeof(GROUPWARE_MESSAGE_STRUCT);
	message->header.SOURCE_QID = sourceqid;
	message->header.TARGET_QID = _msgq_get_id(0, targetqnum);

	/* Is target exist? */
	if (message->header.TARGET_QID == MSGQ_NULL_QUEUE_ID)
	{
		_msg_free(message);
		puts("Error : Target queue is not exist.\n");
		message = NULL;
	}

	return message;
}

//=============================================================================
static GROUPWARE_MESSAGE_STRUCT_PTR send_receive_message(_queue_id queueid, GROUPWARE_MESSAGE_STRUCT_PTR sendmsg, uint32_t timeout)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	receivemsg = NULL;	/* received message */
	MQX_TICK_STRUCT					currenttime;		/* current tick time */
	MQX_TICK_STRUCT					starttime;			/* start tick time for timeout count */
	bool							overflow;			/* dummy variable for _time_diff function */
	_queue_id						targetqid;			/* target queue ID */

	assert(queueid != MSGQ_NULL_QUEUE_ID);
	assert(sendmsg);

	targetqid = sendmsg->header.TARGET_QID;

	/* send */
	if (_msgq_send(sendmsg) == false)
	{
		_msg_free(sendmsg);
		puts("Error : Message send failed.\n");
		return NULL;
	}

	_time_get_ticks(&starttime);

	/* receive */
	do
	{
		_time_delay(1);
		receivemsg = _msgq_poll(queueid);
		_time_get_ticks(&currenttime);

	} while((receivemsg == NULL) && (_time_diff_milliseconds(&currenttime, &starttime, &overflow) < timeout));

	/* timeout */
	if(receivemsg == NULL)
	{
		puts("Error : Message timeout.\n");
		return NULL;
	}

	/* check source */
	if (receivemsg->header.SOURCE_QID != targetqid)
	{
		_msg_free(receivemsg);
		puts("Error : Message from unknown source.\n");
		receivemsg = NULL;
	}

	return receivemsg;
}
