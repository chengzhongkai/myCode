/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include <rtcs.h>
#include <ipcfg.h>

#include <ctype.h>

#include "FTPClient.h"
#include "version.h"

#include "Config.h"
#include "Tasks.h"
#include "flash_utils.h"

//=============================================================================
static void usage(void)
{
	printf("Usage: update (<last 2 byte of IP>)\n   Start firmware update (downloading files via ftp)\n");
}

//=============================================================================
static _ip_address resolve_ip_addr(void)
{
	char domain[255] = {0};	/* domain name of FTP server */
	char subdomain[FLASHROM_SUBDOMAIN_SIZE + 1];
	uint8_t ftpid[FLASHROM_FTPID_SIZE + 1];

	HOSTENT_STRUCT_PTR dnsdata;

	// ----------------------------------------------------- Read Subdomain ---
	if (EEPROM_Read(FLASHROM_SUBDOMAIN_ID,
					subdomain, FLASHROM_SUBDOMAIN_SIZE) == 0) {
		puts("update: NG Fail to read subdomain data.\n");
		return (0);
	}
	subdomain[FLASHROM_SUBDOMAIN_SIZE] = '\0';

	if (subdomain[0] != 0xff) {
		strncpy(domain, subdomain, FLASHROM_SUBDOMAIN_SIZE);
		if (strlen(domain) != 0) {
			strcat(domain, ".");
		}
	}

	// --------------------------------------------------- Read Domain Name ---
	if (EEPROM_Read(FLASHROM_FTPID_ID, ftpid, FLASHROM_FTPID_SIZE) == 0) {
		puts("update: NG Fail to read domain ID.\n");
		return (0);
	}
	if (ftpid[0] == 0) {
		strcat(domain, "smk.co.jp");
	} else  {
		puts("update: NG Unregistered domain ID\n");
		return (0);
	}

	// ------------------------------------ Get IP Address from Domain Name ---
	dnsdata = gethostbyname(&domain[0]);
	if (dnsdata == NULL || *(uint32_t *)dnsdata->h_addr_list[0] == 0) {
		puts("update: NG DNS error\n");
		return (0);
	}

	printf("ftp://%s/", domain);

	return (*(uint32_t *)dnsdata->h_addr_list[0]);
}

//=============================================================================
static _ip_address get_ip_addr(char *param)
{
	char *periodpoint;
	IPCFG_IP_ADDRESS_DATA currentip;
	_mqx_int byte[2] = {0x0000FFFF, 0x0000FFFF};
	_mqx_int i;

	_ip_address ip_addr = 0;

	if (strlen(param) > 7) {
		return (0);
	}

	ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &currentip);
	periodpoint = strchr(param, '.');

	if (periodpoint == NULL) {
		byte[0] = (_mqx_int)((currentip.ip & 0x0000FF00) >> 8);
		periodpoint = param;
	}
	else if (periodpoint == strrchr(param, '.')) {
		*periodpoint = 0;
		periodpoint++;

		for (i = 0; i < strlen(param); i++) {
			if(!(isdigit(param[i]))) {
				break;
			}
		}

		if (i == strlen(param)) {
			byte[0] = atoi(param);
		}
	}

	for (i = 0; i < strlen(periodpoint); i++) {
		if (!(isdigit(periodpoint[i]))) {
			break;
		}
	}

	if (i == strlen(periodpoint)) {
		byte[1] = atoi(periodpoint);
	}

	if ((byte[0] >= 0) && (byte[0] <= 255) && (byte[1] > 0) && (byte[1] < 255))
	{
		ip_addr = (currentip.ip & 0xFFFF0000) + (_ip_address)(byte[0] * 256) + (_ip_address)byte[1];
	}

	return (ip_addr);
}

//=============================================================================
int32_t Shell_update(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	// -------------------------------------------------------------- Usage ---
	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("update (<last 2 byte of IP>)\n");
		} else {
			usage();
		}
		return (0);
	}
	if (argc > 2) {
		usage();
		return (0);
	}

	FTP_ACCESS_INFO	ftp_server = {
		.username = "firmup0",
		.password = "test4SMK",
		.path = NULL
	};

	_queue_id queueid;
   	GROUPWARE_MESSAGE_STRUCT_PTR receivemsg;

	char ftppath[FLASHROM_PATH_SIZE + 1] = {0};

	_task_id ftpclient_tid = MQX_NULL_TASK_ID;


	/* FTP server setting */
	if (argc == 1)
	{
		// ------------------ download update file from external FTP server ---
		ftp_server.ftpaddress = resolve_ip_addr();
		if (ftp_server.ftpaddress == 0) {
			return (0);
		}
	}
	else /* (argc == 2) */
	{
	    // ------------------ download update file from internal ftp server ---
		ftp_server.ftpaddress = get_ip_addr(argv[1]);
		if (ftp_server.ftpaddress == 0) {
			puts("update: NG input data error\n");
			return (0);
		}

		printf("ftp://%d.%d.%d.%d/",
			   (uint8_t)((ftp_server.ftpaddress >> 24) & 0xff),
			   (uint8_t)((ftp_server.ftpaddress >> 16) & 0xff),
			   (uint8_t)((ftp_server.ftpaddress >>  8) & 0xff),
			   (uint8_t)(ftp_server.ftpaddress         & 0xff));
	}

	// ---------------------------------------------------------- Read Path ---
	if (EEPROM_Read(FLASHROM_PATH_ID, ftppath, FLASHROM_PATH_SIZE) == 0) {
		puts("update: NG Fail to read FTP path data.\n");
		return (0);
	}
	ftppath[FLASHROM_PATH_SIZE] = '\0';
	ftp_server.path = ftppath;

	if (ftppath[0] == '\0') {
		printf("\n");
	} else {
		printf("%s/\n", ftppath);
	}
	fflush(stdout);

#if 0
	printf("IP Address: %d.%d.%d.%d\n",
		   (uint8_t)((ftp_server.ftpaddress >> 24) & 0xff),
		   (uint8_t)((ftp_server.ftpaddress >> 16) & 0xff),
		   (uint8_t)((ftp_server.ftpaddress >>  8) & 0xff),
		   (uint8_t)(ftp_server.ftpaddress         & 0xff));
#endif

	// --------------------------------------------------------- Open Queue ---
	queueid = _msgq_open_system(MSGQ_FREE_QUEUE, 0, NULL, 0);
	if (queueid ==  MSGQ_NULL_QUEUE_ID) {
		printf("update: Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
		return (0);
	}

	// --------------------------------------------------- Start FTP Client ---
    ftp_server.version = (VERSION_DATA_PTR)&stSoftwareVersion;
    ftp_server.taskQueueNumber = FTP_MESSAGE_QUEUE;
	ftp_server.creatorqid = queueid;
	ftp_server.msg_fd = stdout;
	ftpclient_tid = _task_create(0,
								 TN_FTP_CLIENT_TASK, (uint32_t)(&ftp_server));
	if (ftpclient_tid != MQX_NULL_TASK_ID) {
		puts("update: Download start\n");
		fflush(stdout);

		while((_task_get_index_from_id(ftpclient_tid) == TN_FTP_CLIENT_TASK) && ((receivemsg = _msgq_poll(queueid)) == NULL))
		{
	  		_time_delay(100);
		}

		/* result */
		if (receivemsg != NULL)
		{
			if (receivemsg->header.SOURCE_QID == _msgq_get_id(0, FTP_MESSAGE_QUEUE))
			{
				switch (receivemsg->data[0]) {

					case UPDATE_RESULT_OK:

						puts("update: Download succeed\n");
				  		break;

					case UPDATE_RESULT_ERR_FTP:

						puts("update: FTP server login error\n");
				  		break;

					case UPDATE_RESULT_ERR_SIZE:

						puts("update: Update file size error\n");
				  		break;

					case UPDATE_RESULT_ERR_MD5:

						puts("update: Update file hash error\n");
				  		break;

					case UPDATE_RESULT_ERR_DATE:

						puts("update: Update file version error\n");
				  		break;

					case UPDATE_RESULT_ERR_EEPROM:

						puts("update: EEPROM access error\\n");
				  		break;

					case UPDATE_RESULT_ERR_VERIFY:

						puts("update: Update file verify error\n");
				  		break;

					default:
						puts("update: Unknown error\n");
				}

				_msgq_close(receivemsg->header.SOURCE_QID);
			}
			else
			{
				puts("update: Message from unknown task.\n");
			}

			_msg_free(receivemsg);
			_task_abort(ftpclient_tid);
		}
		else
		{
			puts("update: FTP Client task aborted.\n");
		}
	}
	else
	{
		printf("Fatal Error 0x%X: Unable to create FTPClient task.",
			   _task_get_error());
	}

	_msgq_close(queueid);

	return 0;
}

/******************************** END-OF-FILE ********************************/
