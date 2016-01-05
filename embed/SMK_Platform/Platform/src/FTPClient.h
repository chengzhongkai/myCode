#ifndef FTPCLIENT_H_
#define FTPCLIENT_H_

#include "platform.h"

#define UPDATE_MAGIC_WORD	"SMK-Gateway999"		/* Magic word */
#define BLOCK_SIZE	4096							/* block size of download binary */
#define FTPCLIENT_USER_LENGTH		12				/* maximum length of FTP user name */
#define FTPCLIENT_PASSWORD_LENGTH	12				/* maximum length of FTP password */
#define FTPCLIENT_PATH_LENGTH		32				/* maximum length of Path */

/* address on EEPROM */
#define MAGICWORD_ADDR		0x80000	/* Update data:Magic word */
#define VERSION_ADDR		0x80100	/* Update data:Version */
#define LENGTH_ADDR			0x80200	/* Update data:Byte length of update program */
#define UPDATE_FLAG_ADDR	0x80300	/* Update data:Flag for automatic update */
#define UPDATE_PROGRAM_ADDR	0x80400	/* Update data:Program */

/* result */
#define	UPDATE_RESULT_OK			0	/* OK */
#define	UPDATE_RESULT_ERR_FTP		1	/* FTP server login error */
#define	UPDATE_RESULT_ERR_SIZE		2	/* file size error */
#define	UPDATE_RESULT_ERR_READ		3	/* file read error */
#define	UPDATE_RESULT_ERR_MD5		4	/* MD5 compare error */
#define	UPDATE_RESULT_ERR_DATE		5	/* file date error */
#define	UPDATE_RESULT_ERR_EEPROM	6	/* EEPROM error */
#define	UPDATE_RESULT_ERR_VERIFY	7	/* verify error */

/* FTP Access Information */
typedef struct ftp_access_info
{
	_ip_address		ftpaddress;
	char			*username;
	char			*password;
	char			*path;
    VERSION_DATA_PTR version;
    _queue_number   taskQueueNumber;
	_queue_id		creatorqid;
	MQX_FILE		*msg_fd;
}
FTP_ACCESS_INFO, * FTP_ACCESS_INFO_PTR;

/* Task */
extern void FTPClient_task(uint32_t ftpaccess);

#endif /* FTPCLIENT_H_ */
