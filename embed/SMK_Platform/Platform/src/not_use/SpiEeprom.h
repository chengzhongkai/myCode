#ifndef SPIMEM_H_
#define SPIMEM_H_

/* SPI Memory Product Number */
#define SPI_MEMORY_NAME					"M25P80"	/* Product Name:Micron M25P80 */

#define SPI_MEMORY_ADDRESS_BYTES		3

/* Memory page size - maximum bytes per write */
#define SPI_MEMORY_PAGE_SIZE			0x100
#define SPI_MEMORY_SECTOR_SIZE			0x10000
#define SPI_MEMORY_SECTOR_NUMBER		16

/* The SPI serial memory instructions */
#define SPI_MEMORY_WRITE_STATUS			0x01		/* WRITE STATUS REGISTER */
#define SPI_MEMORY_WRITE_DATA			0x02		/* PAGE PROGRAM */
#define SPI_MEMORY_READ_DATA			0x0B		/* READ DATA BYTE by HIGH SPEED */
#define SPI_MEMORY_WRITE_LATCH_DISABLE	0x04		/* WRITE DISABLE */
#define SPI_MEMORY_READ_STATUS			0x05		/* READ STATUS REGISTER */
#define SPI_MEMORY_WRITE_LATCH_ENABLE	0x06		/* WRITE ENABLE */
#define SPI_MEMORY_SECTOR_ERASE			0xD8		/* SECTOR ELASE */
#define SPI_MEMORY_CHIP_ERASE			0xC7		/* BULK ELASE */
#define SPI_MEMORY_READ_ID				0x9E		/* READ IDENTIFICATION */
#define SPI_MEMORY_SET_POWERDOWN		0xB9		/* DEEP POWER-DOWN */
#define SPI_MEMORY_RELEASE_POWERDOWN	0xAB		/* RELEASE from DEEP POWER-DOWN */

/* Status */
#define SPI_MEMORY_STATUS_BUSY	0x01	/* WIP:write in progress */
#define SPI_MEMORY_STATUS_WEL	0x02	/* WEL:write eneble */
#define SPI_MEMORY_STATUS_BP0	0x04	/* BP0:block protect bit 0 */
#define SPI_MEMORY_STATUS_BP1	0x08	/* BP0:block protect bit 1 */
#define SPI_MEMORY_STATUS_BP2	0x10	/* BP0:block protect bit 2 */
#define SPI_MEMORY_STATUS_BPL	0x80	/* SRWD:status register write protect */

#define SPI_MEMORY_DELAY_CHIPERASE		10000	/* typical chip erase time(ms) */
#define SPI_MEMORY_DELAY_SECTORERASE	3000	/* typical sector erase time(ms) */
#define SPI_MEMORY_DELAY_BYTEPRG		5		/* maximum write data time(ms) */
#define SPI_MEMORY_DELAY_WRITESTATUS	15		/* maximum write status register time(ms) */

/* Funtion prototypes */
extern void    memory_chip_erase (MQX_FILE_PTR spifd);
extern void    memory_set_write_latch (MQX_FILE_PTR spifd, bool enable);
extern void    memory_set_protection (MQX_FILE_PTR spifd, bool protect);
extern uint8_t  memory_read_status (MQX_FILE_PTR spifd);
extern void    memory_write_byte (MQX_FILE_PTR spifd, uint32_t addr, unsigned char data);
extern uint8_t  memory_read_byte (MQX_FILE_PTR spifd, uint32_t addr);
extern uint32_t memory_write_data (MQX_FILE_PTR spifd, uint32_t addr, uint32_t size, unsigned char *data);
extern uint32_t memory_read_data (MQX_FILE_PTR spifd, uint32_t addr, uint32_t size, uint8_t *data);
extern bool    memory_sector_erase (MQX_FILE_PTR spifd, uint32_t sector);

#endif
