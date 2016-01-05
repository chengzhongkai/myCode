#ifndef BOOTLOADER_UPDATE_H_
#define BOOTLOADER_UPDATE_H_

/* SPI EEPROM Address */
#define UPDATE_MAGICWORD_ADDRESS	0x80000	/* Magic word */
#define UPDATE_VERSION_ADDRESS		0x80100	/* Version(compile date */
#define UPDATE_LENGTH_ADDRESS		0x80200	/* Program file length(byte) */	
#define UPDATE_FORCED_ADDRESS		0x80300	/* force update flag */
#define UPDATE_PROGRAM_ADDRESS		0x80400	/* program data */

#define UPDATE_PROGRAM_MAXSIZE		(0x80000 - 0xA000)	/* maximum size of program data */

/* struct */
typedef struct version_data
{
	uint8_t 	bMajorVersion;	/* Major version */
	uint8_t		bMinorVersion;	/* Minor version */
	uint16_t	wYear;			/* Year */
  	uint8_t		bMonth;			/* Month */
	uint8_t		bDate;			/* Date */
} VERSION_DATA, * VERSION_DATA_PTR;

extern bool update_check_magicword(void);
extern void update_exec(void);

#endif /* BOOTLOADER_UPDATE_H_ */