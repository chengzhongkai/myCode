#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <ftpc.h>
#include <io_mem.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#define FREESCALE_MMCAU	1
#include "cau/cau_api.h"

#include "storage_api.h"

#include "FTPClient.h"

#define BUFFER_NAME	"ftpdata:"

#define FTP_FILENAME	"Gateway-firmware.%03d"
#define FTP_SIZEFILE	"Gateway-firmware.size"
#define FTP_MD5FILE		"Gateway-firmware.md5"
#define FTP_VERFILE		"Gateway-firmware.ver"
#define FTP_FORCEFILE	"Gateway-firmware.forced"

#define FILESIZE_ERROR	0xFFFFFFFF
#define FILESIZE_MAX	(0x80000 - 0xA000)
#define MD5_BYTESIZE	64

#define FILEDATE_LATEST	1
#define FILEDATE_SAME	0
#define FILEDATE_OLD	2
#define FILEDATE_ERROR	0xFFFFFFFF

static void *FTPClient_init(const FTP_ACCESS_INFO_PTR ftpinfo);
static void FTPClient_end(void *ftphandle, const char *buffername, _queue_id queueid, uint32_t result, _queue_id creatorqid);

static uint32_t FTPClient_read_size(void *ftphandle, const char *filename, const char *buffername);
static bool FTPClient_get_data(void *ftphandle, const char *filename, const char *buffername);
static uint32_t FTPClient_read_wholefilesize(void *ftphandle, const char *buffername);
static bool FTPClient_check_md5(void *ftphandle, const char *buffername, uint32_t filelength);
static uint32_t FTPClient_check_version(void *ftphandle, const VERSION_DATA_PTR saveddatadate, VERSION_DATA_PTR orgVersion, const char *buffername);

static uint8_t *read_from_exmem(MQX_FILE *fd, uint32_t startaddr, uint32_t length);
static bool write_to_exmem(MQX_FILE *fd, uint32_t startaddr, uint32_t length, const uint8_t *data);

static bool erase_exmem_all(MQX_FILE *fd);
static uint32_t write_and_verify_exmem(MQX_FILE *fd,
									   uint32_t startaddr, uint32_t length,
									   const uint8_t *data);

static uint8_t *FTPClient_get_buffer_base_addr(const char *buffername);
static uint32_t FTPClient_update_date_to_value(const VERSION_DATA_PTR datedata);
static uint32_t FTPClient_version_to_value(const VERSION_DATA_PTR datedata);
static void FTPClient_abort(void);

static uint32_t dwFtpHandleabort = 0;	/* handle number for FTP socket(to abort) */
static _queue_id gFtpQID = MSGQ_NULL_QUEUE_ID;

static MQX_FILE *gMsgFD = NULL;

//=============================================================================
static void ftp_printf(const char *format, ...)
{
	if (gMsgFD != NULL) {
		va_list args;
		va_start(args, format);
		vfprintf(gMsgFD, format, args);
		va_end(args);

		fflush(gMsgFD);
	}
}

//=============================================================================
void FTPClient_task(uint32_t ftpaccess)
{
	void *ftphandle;								/* handle for ftp task */
	uint32_t filelength;							/* byte length of update file */
	uint32_t blocknumber = 0;						/* current downloading block number */
	uint32_t blocklength;							/* byte length of current downloading block */
	uint8_t buffer[BLOCK_SIZE] = {0};				/* download buffer */

	int i;											/* counter for roop */
	uint8_t *eepromversion;							/* compile date information of software in SPI EEPROM */
	const char magicword[] = UPDATE_MAGIC_WORD;	/* Magic word */
	bool forcedupdate = false;						/* forced update flag */
	uint32_t datecomp;								/* result of compile date comparation */
	char ftpfilename[24] = "";						/* filename character of downloading file */
    FTP_ACCESS_INFO_PTR info = (FTP_ACCESS_INFO_PTR)ftpaccess;

	uint32_t result;

	int new_progress, cur_progress;

	gMsgFD = info->msg_fd;

	/* set exit handler */
	_task_set_exit_handler(_task_get_id(), FTPClient_abort);

	// ------------------------------------------- Initialize message queue ---
	gFtpQID = _msgq_open_system(info->taskQueueNumber, 0, NULL, 0);
	if (gFtpQID == MSGQ_NULL_QUEUE_ID) {
		err_printf("FTPClient_task: Fatal Error 0x%X: System message queue initialization failed.\n", _task_set_error(MQX_OK));
		_task_destroy(MQX_NULL_TASK_ID);
	}

	/* Initialize buffer */
	_io_mem_install(BUFFER_NAME, &buffer[0], BLOCK_SIZE);

	// ---------------------------------------------- Connect to FTP server ---
	ftphandle = FTPClient_init((FTP_ACCESS_INFO_PTR)ftpaccess);
	if (ftphandle == NULL)
	{
		FTPClient_end(ftphandle, NULL, gFtpQID, UPDATE_RESULT_ERR_FTP, info->creatorqid);
	}

	// ------------------------------------------------- check force update ---
	if (FTPClient_read_size(ftphandle,
							FTP_FORCEFILE, BUFFER_NAME) != FILESIZE_ERROR) {
		ftp_printf("Forced Update Enabled\n");
		forcedupdate = true;
	}

	// ---------------------------------------------- Open External Storage ---
	MQX_FILE *exmem_fd;
	exmem_fd = Storage_Open();
	if (exmem_fd == NULL) {
		err_printf("FTPClient_task: EEPROM open failed\n");
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_EEPROM, info->creatorqid);
	}

	// ---------------------------------------------------- Compare Version ---
	eepromversion = read_from_exmem(exmem_fd,
									VERSION_ADDR, sizeof(VERSION_DATA));
	if (eepromversion == NULL) {
		eepromversion = _mem_alloc_system_zero(sizeof(VERSION_DATA));
	}

	datecomp = FTPClient_check_version(ftphandle,
									   (VERSION_DATA_PTR)eepromversion,
									   info->version, BUFFER_NAME);
	_mem_free(eepromversion);

	if ((datecomp != FILEDATE_LATEST)
		&& ((forcedupdate == false) || (datecomp != FILEDATE_OLD))) {
		err_printf("FTPClient_task: Update file isn't latest\n");
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_DATE, info->creatorqid);
	}

	// -------------------------------------------------- Get Firmware Size ---
	filelength = FTPClient_read_wholefilesize(ftphandle, BUFFER_NAME);
	if (filelength == FILESIZE_ERROR) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_SIZE, info->creatorqid);
	}
	ftp_printf("Firmware Size: %d bytes\n", filelength);

	// ----------------------------------- Download All Files and Check MD5 ---
	if (FTPClient_check_md5(ftphandle,  BUFFER_NAME, filelength) == false) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_MD5, info->creatorqid);
	}

	// ----------------------------------------------------- Erase Old Data ---
	ftp_printf("Prepare Flash Memory...");
	if (erase_exmem_all(exmem_fd) == false) {
		err_printf("FTPClient_task: EEPROM erase failed\n");
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_EEPROM, info->creatorqid);
	}
	ftp_printf("Done\n");

	// ---------------------------------------- Download & save update file ---
	blocknumber = filelength / BLOCK_SIZE + ((filelength % BLOCK_SIZE) ? 1 : 0);
	ftp_printf("Download and Save to Flash Memory with Verifying\n");
	ftp_printf("|---------+---------+---------+---------|\n*");
	cur_progress = 0;
	new_progress = 0;
	for (i = 0; i < blocknumber; i ++) {
		uint32_t exmem_addr;

		blocklength = (i != (blocknumber - 1)) ? BLOCK_SIZE : ((filelength % BLOCK_SIZE) ? (filelength % BLOCK_SIZE) : BLOCK_SIZE);
		sprintf(ftpfilename, FTP_FILENAME, i);

		exmem_addr = UPDATE_PROGRAM_ADDR + BLOCK_SIZE * i;

		// -------------------------------------------------- Download File ---
		if (FTPClient_get_data(ftphandle, (const char *)ftpfilename,
							   BUFFER_NAME) == false) {
			FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
						  UPDATE_RESULT_ERR_READ, info->creatorqid);
		}

		// --------------------------- Write to External Storage and Verify ---
		result = write_and_verify_exmem(exmem_fd,
										exmem_addr, blocklength, &buffer[0]);
		if (result != UPDATE_RESULT_OK) {
			FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
						  result, info->creatorqid);
		}

		new_progress = ((i + 1) * 40) / blocknumber;
		while (new_progress > cur_progress) {
			ftp_printf("*");
			cur_progress ++;
		}
	}
	ftp_printf("\n");

	// ------------------------------------------------------- write length ---
	if (FTPClient_get_data(ftphandle, FTP_SIZEFILE, BUFFER_NAME) == false) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_READ, info->creatorqid);
	}

	result = write_and_verify_exmem(exmem_fd, LENGTH_ADDR, 7, &buffer[0]);
	if (result != UPDATE_RESULT_OK) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  result, info->creatorqid);
	}

	// ------------------------------------------------------ Write Version ---
	if (FTPClient_get_data(ftphandle, FTP_VERFILE, BUFFER_NAME) == false) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  UPDATE_RESULT_ERR_READ, info->creatorqid);
	}

	result = write_and_verify_exmem(exmem_fd, VERSION_ADDR,
									sizeof(VERSION_DATA), &buffer[0]);
	if (result != UPDATE_RESULT_OK) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  result, info->creatorqid);
	}

	// -------------------------------------------------- write forced flag ---
	if (forcedupdate == true) {
		memset(&buffer[0], 0, 4);

		result = write_and_verify_exmem(exmem_fd, UPDATE_FLAG_ADDR, 4, &buffer[0]);
		if (result != UPDATE_RESULT_OK) {
			FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
						  result, info->creatorqid);
		}
	}

	// --------------------------------------------------- write magic word ---
	result = write_and_verify_exmem(exmem_fd, MAGICWORD_ADDR, sizeof(magicword),
									(const uint8_t *)&magicword[0]);
	if (result != UPDATE_RESULT_OK) {
		FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID,
					  result, info->creatorqid);
	}

	FTPClient_end(ftphandle, BUFFER_NAME, gFtpQID, UPDATE_RESULT_OK, info->creatorqid);
}

//=============================================================================
/* Initialize(FTP Connection) */
static void *FTPClient_init(const FTP_ACCESS_INFO_PTR ftpinfo)
{
	void *ftphandle;			/* handle for FTP client task */
	int32_t response;			/* response from FTP client functions */
	char user[FTPCLIENT_USER_LENGTH + 8]     = "USER ";	/* FTP USER Command */
	char pass[FTPCLIENT_PASSWORD_LENGTH + 8] = "PASS ";	/* FTP PASS Command */
	char path[FTPCLIENT_PATH_LENGTH + 7]     = "CWD ";		/* FTP CWD Command */
	const char type[] = "TYPE I\r\n";						/* FTP TYPE Command */

	assert(ftpinfo != NULL);

	if ( (strlen(ftpinfo->username) > FTPCLIENT_USER_LENGTH)     ||
		 (strlen(ftpinfo->password) > FTPCLIENT_PASSWORD_LENGTH) ||
		 (strlen(ftpinfo->path) > FTPCLIENT_PATH_LENGTH)           )
	{
		err_printf("FTPClient_task: Too long string.\n");
		return NULL;
	}

	/* Connect to FTP server */
	if (FTP_open(&ftphandle, ftpinfo->ftpaddress, stdout) == -1)
	{
		err_printf("FTPClient_task: Couldn't open FTP session.\n");
		return NULL;
	}

	/* measures for freeze at login term */
	dwFtpHandleabort = (uint32_t)ftphandle;

	/* Login */
	strcat(user, ftpinfo->username);
	strcat(user, "\r\n");
	response = FTP_command(ftphandle, user, stdout);

	/* response 3xx means Password Required */
	if ((response >= 300) && (response < 400))
	{
		strcat(pass, ftpinfo->password);
		strcat(pass, "\r\n");
		response = FTP_command(ftphandle, pass, stdout);
	}

	if ((response < 200) || (response >= 300))
	{
		err_printf("FTPClient_task: Login information mismatch\n");
		FTP_close(ftphandle, stdout);
		dwFtpHandleabort = 0;
		return NULL;
	}

	/* change mode */
	response = FTP_command(ftphandle, (char *)type, stdout);
	if ((response < 200) || (response >= 300))
	{
		err_printf("FTPClient_task: Transfer mode error.\n");
		FTP_close(ftphandle, stdout);
		dwFtpHandleabort = 0;
		return NULL;
	}

	/* change directory */
	if (ftpinfo->path != NULL)
	{
		strcat(path, ftpinfo->path);
		strcat(path, "\r\n");
		response = FTP_command(ftphandle, path, stdout);

		if ((response < 200) || (response >= 300))
		{
			err_printf("FTPClient_task: Path error\n");
			FTP_close(ftphandle, stdout);
			ftphandle = NULL;
			dwFtpHandleabort = 0;
		}
	}

	return ftphandle;
}

//=============================================================================
/* end */
static void FTPClient_end(void *ftphandle, const char *buffername, _queue_id queueid, uint32_t result, _queue_id creatorqid)
{
	GROUPWARE_MESSAGE_STRUCT_PTR	resultmsg;

	/* uninstall RAM filsystem driver */
	if (buffername != NULL)
	{
		_io_dev_uninstall(BUFFER_NAME);
	}

	/* close FTP connection */
	if (ftphandle != NULL)
	{
		FTP_close(ftphandle, stdout);
	}
	dwFtpHandleabort = 0;

	/* unable to send message */
	if (queueid == 0)
	{
		err_printf("FTPClient_task: message queue isn't exist\n");
		_task_destroy(MQX_NULL_TASK_ID);
	}

	/* create result message */
	resultmsg = (GROUPWARE_MESSAGE_STRUCT_PTR)_msg_alloc_system(sizeof(GROUPWARE_MESSAGE_STRUCT));
	if (resultmsg == NULL)
	{
		err_printf("FTPClient_task: message allocation error\n");
		_task_destroy(MQX_NULL_TASK_ID);
	}

	resultmsg->header.SIZE       = sizeof(GROUPWARE_MESSAGE_STRUCT);
	resultmsg->header.TARGET_QID = creatorqid;
	resultmsg->header.SOURCE_QID = queueid;
	resultmsg->data[0]           = result;

	/* send result message */
	if ( (creatorqid == MSGQ_NULL_QUEUE_ID)  ||
		 (_msgq_send(resultmsg) == false)  )
	{
		err_printf("FTPClient_task: message send error\n");
		_msg_free(resultmsg);
		_task_destroy(MQX_NULL_TASK_ID);
	}

	_task_block();
}

//=============================================================================
// Get File Size
//=============================================================================
static uint32_t FTPClient_read_size(void *ftphandle,
									const char *filename,
									const char *buffername)
{
	char			ftpcommand[32] = "SIZE ";	/* FTP SIZE command */
	uint32_t		bufferparam[3];				/* buffer file parameter */
	MQX_FILE_PTR	bufferfile;					/* file handle for buffer */
	int32_t			response;					/* response from FTP functions */
	uint32_t		size = FILESIZE_ERROR;		/* file size */

	assert(ftphandle != NULL);
	assert(filename != NULL);
	assert(buffername != NULL);

	bufferfile = fopen(buffername, NULL);

	if (bufferfile != NULL) {
		assert(ioctl(bufferfile, IO_MEM_IOCTL_GET_TOTAL_SIZE, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[0] == BLOCK_SIZE);
		assert(ioctl(bufferfile, IO_IOCTL_DEVICE_IDENTIFY, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[IO_IOCTL_ID_PHY_ELEMENT] == IO_DEV_TYPE_PHYS_MEMIO);

		if (ioctl(bufferfile,
				  IO_MEM_IOCTL_GET_BASE_ADDRESS, &bufferparam[0]) == MQX_OK) {
			memset((char *)bufferparam[0], 0, BLOCK_SIZE);
			strcat(ftpcommand, filename);
			strcat(ftpcommand, "\r\n");

			response = FTP_command(ftphandle, &ftpcommand[0], bufferfile);

			dbg_printf((char *)bufferparam[0]);

			if (response == 213) {
				size = atoi((const char *)(bufferparam[0]+4));

				if (size > BLOCK_SIZE) {
					size = FILESIZE_ERROR;
					err_printf("FTPClient_read_size: File size error\n");
				}
			}
			else {
				err_printf("FTPClient_read_size: Block size read error\n");
			}

			if (size == FILESIZE_ERROR) {
				err_printf("FTPClient_read_size: file : %s\n", filename);
			}
		}
		else {
			err_printf("FTPClient_read_size: filesystem size error\n");
		}

		fclose(bufferfile);
	}
	else {
		err_printf("FTPClient_read_size: filesystem open error\n");
	}

	return (size);
}

//=============================================================================
/* read file data */
static bool FTPClient_get_data(void *ftphandle, const char *filename, const char *buffername)
{
	char			ftpcommand[32] = "RETR ";	/* FTP SIZE command */
	MQX_FILE_PTR	bufferfile;					/* buffer file id */
	bool			result = false;			/* result */
	uint32_t		bufferparam[3];				/* buffer file parameter */

	assert(ftphandle != NULL);
	assert(filename != NULL);
	assert(buffername != NULL);

	bufferfile = fopen(buffername, NULL);

	if (bufferfile != NULL)
	{

		assert(ioctl(bufferfile, IO_MEM_IOCTL_GET_TOTAL_SIZE, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[0] == BLOCK_SIZE);
		assert(ioctl(bufferfile, IO_IOCTL_DEVICE_IDENTIFY, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[IO_IOCTL_ID_PHY_ELEMENT] == IO_DEV_TYPE_PHYS_MEMIO);

		if (ioctl(bufferfile, IO_MEM_IOCTL_GET_BASE_ADDRESS, &bufferparam[0]) == MQX_OK)
		{
			memset((char *)bufferparam[0], 0, BLOCK_SIZE);
			strcat(ftpcommand, filename);
			strcat(ftpcommand, "\r\n");

			if (FTP_command_data(ftphandle, &ftpcommand[0], stdout, bufferfile, FTPMODE_PASV | FTPDIR_RECV) == 226)
			{
				result = true;
			}
			else
			{
				err_printf("FTPClient_task: File read error\n");
				err_printf("FTPClient_task: file : %s\n", filename);
			}
		}
		else
		{
			err_printf("FTPClient_task: filesysten size error\n");
		}

		fclose(bufferfile);
	}
	else
	{
		err_printf("FTPClient_task: filesystem open error\n");
	}

	return (result);
}

//=============================================================================
// Get Firmware Size from FTP Server
//=============================================================================
static uint32_t FTPClient_read_wholefilesize(void *ftphandle, const char *buffername)
{
	uint32_t filesize;
	uint32_t size = FILESIZE_ERROR;	/* whole file size of update files */
	uint8_t *bufferbase;			/* buffer file base address */

	assert(ftphandle != NULL);
	assert(buffername != NULL);

	// -------------------------------------- Download "Firmware Size" File ---
	filesize = FTPClient_read_size(ftphandle, FTP_SIZEFILE, buffername);
	if (filesize > BLOCK_SIZE) {
		return (FILESIZE_ERROR);
	}
	if (!FTPClient_get_data(ftphandle, FTP_SIZEFILE, buffername)) {
		return (FILESIZE_ERROR);
	}

	// ------------------------------------------------- Get Buffer Pointer ---
	bufferbase = FTPClient_get_buffer_base_addr(buffername);

	if (bufferbase != NULL) {
		size = atoi((const char *)bufferbase);

		/* no file or beyond the flash ROM capacity */
		if ((size == 0) || (size > FILESIZE_MAX)) {
			err_printf("FTPClient_task: Update file size error\n");
			size = FILESIZE_ERROR;
		}
	}
	else {
		err_printf("FTPClient_task: buffer error\n");
	}

	return (size);
}

//=============================================================================
static bool FTPClient_check_md5(void *ftphandle, const char *buffername, uint32_t filelength)
{
	uint8_t			md5hash[16] = {0};				/* MD5 hash */
	char			md5calcdata[34] = "";			/* MD5 hash (ascii)*/
	uint8_t			md5padding[MD5_BYTESIZE] = {0};	/* MD5 padding data */
 	uint32_t		fileblocknumber = 0;			/* number of file block */
	uint32_t		blocklength = 0;				/* byte length of file block */
	uint32_t		modlength;						/* reminder */
	char			filename[24] = "";				/* downloading filename */
	uint32_t 		i;								/* roop counter */
	uint8_t			*bufferbase;					/* buffer file base address */

	int num_blocks;
	int new_progress, cur_progress;

	assert(ftphandle != NULL);
	assert(buffername != NULL);
	assert(filelength != 0);

	// ------------------------------------------------- Get Buffer Pointer ---
	bufferbase = FTPClient_get_buffer_base_addr(buffername);
	if (bufferbase == NULL) {
		err_printf("FTPClient_task: buffer error\n");
		return (false);
	}

  	// ---------------------------------------------------- Initialize hash ---
	cau_md5_initialize_output((const unsigned char *)&md5hash[0]);

	num_blocks = filelength / BLOCK_SIZE + ((filelength % BLOCK_SIZE) ? 1 : 0);

	ftp_printf("Check Firmware MD5, Total %d Blocks to Read\n", num_blocks);
	ftp_printf("|---------+---------+---------+---------|\n*");
	cur_progress = 0;
	new_progress = 0;

	do {
		// -------------------------------------------------- Get File Size ---
		sprintf(&filename[0], FTP_FILENAME, fileblocknumber);
		blocklength = FTPClient_read_size(ftphandle,
										  (const char *)&filename[0],
										  buffername);
		if (blocklength > BLOCK_SIZE) {
			return (false);
		}

		// -------------------------------------------------- Download File ---
		if (!FTPClient_get_data(ftphandle,
							   (const char *)&filename[0], buffername)) {
			return (false);
		}

		// ------------------------------------------------------- Calc MD5 ---
		if (blocklength >= MD5_BYTESIZE) {
			cau_md5_hash_n((const unsigned char *)bufferbase,
						   (blocklength / MD5_BYTESIZE),
						   (unsigned char *)&md5hash[0]);
		}

		fileblocknumber ++;

		new_progress = ((fileblocknumber + 1) * 40) / num_blocks;
		while (new_progress > cur_progress) {
			ftp_printf("*");
			cur_progress ++;
		}

	} while (filelength > (fileblocknumber * BLOCK_SIZE));
	ftp_printf("\n");

	/* calc MD5(last block) */
	md5padding[MD5_BYTESIZE-8] = (uint8_t)((filelength << 3)  & 0xFF);
	md5padding[MD5_BYTESIZE-7] = (uint8_t)((filelength >> 5)  & 0xFF);
	md5padding[MD5_BYTESIZE-6] = (uint8_t)((filelength >> 13) & 0xFF);
	md5padding[MD5_BYTESIZE-5] = (uint8_t)((filelength >> 21) & 0xFF);

	modlength = blocklength % MD5_BYTESIZE;
	if (modlength != 0) {
		*(uint8_t *)(bufferbase + blocklength) = 0x80;

		if (modlength <= (MD5_BYTESIZE-9)) {
			memcpy(&md5padding[0],
				   (const unsigned char *)(bufferbase+blocklength-modlength),
				   modlength+1);
		} else {
			cau_md5_hash((const unsigned char *)(bufferbase+blocklength-modlength), (unsigned char *)&md5hash[0]);
		}
	} else {
		md5padding[0] = 0x80;
	}

	cau_md5_hash((const unsigned char *)&md5padding[0],
				 (unsigned char *)&md5hash[0]);

	for (i = 0; i < sizeof(md5hash); i++) {
		sprintf(&md5calcdata[i*2], "%02x", md5hash[i]);
	}

	// -------------------------------------------------- Download MD5 File ---
	if (FTPClient_read_size(ftphandle, FTP_MD5FILE, buffername)
		!= (sizeof(md5hash)*2+1)) {
		err_printf("FTPClient_task: MD5 file size error\n");
		return (false);
	}

	if (!FTPClient_get_data(ftphandle, FTP_MD5FILE, buffername)) {
		err_printf("FTPClient_task: MD5 file read error\n");
		return (false);
	}

	// -------------------------------------------------------- Compare MD5 ---
	dbg_printf("Calculate MD5: %s\n", md5calcdata);
	dbg_printf("Download  MD5: %s\n", (char *)bufferbase);

	if (memcmp(&md5calcdata[0], (void *)bufferbase, sizeof(md5hash)*2) != 0) {
		err_printf("FTPClient_task: MD5 mismatch\n");
		return (false);
	}

	dbg_printf("FTPClient_task: MD5 check OK\n");

	return (true);
}

//=============================================================================
static uint32_t FTPClient_check_version(void *ftphandle, const VERSION_DATA_PTR savedversiondata, VERSION_DATA_PTR orgVersion, const char *buffername)
{
 	uint32_t eepromversion;			/* version of software in external SPI EEPROM */
	uint32_t currentversion;		/* version of current software */
	uint32_t ftpversion;			/* version of software in FTP server */
	uint8_t	 *bufferbase;			/* buffer file base address */
	uint32_t result = FILEDATE_OLD;	/* result */

	assert(ftphandle != NULL);
	assert(savedversiondata != NULL);
	assert(buffername != NULL);

	bufferbase = FTPClient_get_buffer_base_addr(buffername);
	if (bufferbase == NULL)
	{
		err_printf("FTPClient_task: buffer error\n");
		return FILEDATE_ERROR;
	}

	currentversion = FTPClient_version_to_value(orgVersion);
	eepromversion  = FTPClient_version_to_value(savedversiondata);

	// ---------------------------------------------- Download Version File ---
	if ((FTPClient_read_size(ftphandle, FTP_VERFILE, buffername) != sizeof(VERSION_DATA)) ||
		 (FTPClient_get_data(ftphandle, FTP_VERFILE, buffername) == false)                    )
	{
		err_printf("FTPClient_task: date file error\n");
		return FILEDATE_ERROR;
	}

	ftpversion = FTPClient_version_to_value((VERSION_DATA_PTR)bufferbase);

	/* compare date */
#if 0
	dbg_printf("FTP Software\n");
	Version_print_version((VERSION_DATA_PTR)bufferbase);
	Version_print_date((VERSION_DATA_PTR)bufferbase);

	dbg_printf("EEPROM Software\n");
	Version_print_version(savedversiondata);
	Version_print_date(savedversiondata);

	dbg_printf("Current Software\n");
	Version_print_version((VERSION_DATA_PTR)&stSoftwareVersion);
	Version_print_date((VERSION_DATA_PTR)&stSoftwareVersion);
#endif

	if((ftpversion > currentversion) && (ftpversion > eepromversion))
	{
		result = FILEDATE_LATEST;
	}
	else if ((ftpversion == currentversion) || (ftpversion == eepromversion))
	{
		result = FILEDATE_SAME;
	}

	return result;
}

//=============================================================================
static uint8_t *read_from_exmem(MQX_FILE *fd,
								uint32_t startaddr, uint32_t length)
{
	uint8_t *data;
	uint32_t read_size;

	data = _mem_alloc_system(length);
	if (data == NULL) return (NULL);

	read_size = Storage_Read(fd, startaddr, length, data);
	if (read_size != length) {
		err_printf("FTPClient_task: EEPROM read task error\n");
		_mem_free(data);
		return (NULL);
	}

	return (data);
}

//=============================================================================
static bool write_to_exmem(MQX_FILE *fd,
						   uint32_t startaddr, uint32_t length,
						   const uint8_t *data)
{
	uint32_t write_size;

	if (Storage_IsEmpty(fd, startaddr, length) == false) {
		err_printf("FTPClient_task: writing area may not be empty\n");
		return (false);
	}

	write_size = Storage_Write(fd, startaddr, length, data);

	if (write_size != length) {
		err_printf("FTPClient_task: written size(%d) differs from %d\n",
				   write_size, length);
		return (false);
	}

	return (true);
}

//=============================================================================
static bool erase_exmem_all(MQX_FILE *fd)
{
	return (Storage_EraseSector(fd, 8, 15));
}

//=============================================================================
static uint32_t write_and_verify_exmem(MQX_FILE *fd,
									   uint32_t startaddr, uint32_t length,
									   const uint8_t *data)
{
	uint8_t *verifydata;
	uint32_t verifyresult;

	// ------------------------------------------ Write to External Storage ---
	if (write_to_exmem(fd, startaddr, length, data) == false) {
		return (UPDATE_RESULT_ERR_EEPROM);
	}

	// ------------------------------------------------------------- Verify ---
	verifydata = read_from_exmem(fd, startaddr, length);
	if (verifydata == NULL) {
		return (UPDATE_RESULT_ERR_EEPROM);
	}

	verifyresult = memcmp(data, verifydata, length);

	_mem_free(verifydata);

	if (verifyresult != 0) {
		err_printf("FTPClient_task: verify error\n");
		return (UPDATE_RESULT_ERR_VERIFY);
	}

	return (UPDATE_RESULT_OK);
}

//=============================================================================
static uint32_t FTPClient_update_date_to_value(const VERSION_DATA_PTR datedata)
{
	uint32_t result;	/* result */

	result =  ((uint32_t)(datedata->wYear)  << 16)
			+ ((uint32_t)(datedata->bMonth) << 8)
			+ (uint32_t)(datedata->bDate);

	if (result == 0xFFFFFFFF)
	{
		result = 0;
	}

	return result;
}

//=============================================================================
static uint32_t FTPClient_version_to_value(const VERSION_DATA_PTR datedata)
{
	uint32_t result;	/* result */

	result =  ((uint32_t)datedata->bMajorVersion << 8)
			+ (uint32_t)datedata->bMinorVersion;

	if (result == 0xFFFF)
	{
		result = 0;
	}

	return result;
}

//=============================================================================
static uint8_t *FTPClient_get_buffer_base_addr(const char *buffername)
{
	MQX_FILE_PTR	bufferfile;						/* buffer file id */
	uint32_t		bufferparam[3] = {0, 0, 0};		/* buffer file parameter */

	assert(buffername != NULL);

	bufferfile = fopen(buffername, NULL);

	/* get buffer base address */
	if (bufferfile != NULL) {

		assert(ioctl(bufferfile, IO_MEM_IOCTL_GET_TOTAL_SIZE, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[0] == BLOCK_SIZE);
		assert(ioctl(bufferfile, IO_IOCTL_DEVICE_IDENTIFY, &bufferparam[0]) == MQX_OK);
		assert(bufferparam[IO_IOCTL_ID_PHY_ELEMENT] == IO_DEV_TYPE_PHYS_MEMIO);

		if (ioctl(bufferfile,
				  IO_MEM_IOCTL_GET_BASE_ADDRESS, &bufferparam[0]) != MQX_OK) {
			err_printf("FTPClient_task: filesysten size error\n");
			bufferparam[0] = 0;
		}

		fclose(bufferfile);
	}
	else
	{
		err_printf("FTPClient_task: filesystem open error\n");
	}

	return (uint8_t *)bufferparam[0];
}

//=============================================================================
static void FTPClient_abort(void)
{
	if (dwFtpHandleabort != 0)
	{
		shutdown(dwFtpHandleabort, FLAG_ABORT_CONNECTION);
		_io_dev_uninstall(BUFFER_NAME);
	}

	if (gFtpQID != MSGQ_NULL_QUEUE_ID)
	{
		_msgq_close(gFtpQID);
	}
}

/******************************** END-OF-FILE ********************************/
