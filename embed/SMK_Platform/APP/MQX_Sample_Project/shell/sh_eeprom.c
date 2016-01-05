/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include <ctype.h>

#include "flash_utils.h"

//=============================================================================
static const char sEepromCmdHelp[]  = "eeprom [mac|code|serial|ftpid|ftpsubd|ftppath|dhcp|ip|netmask|gateway|sgwhost|sgwport|sgwpath] (<data>)\\n\n";

static const char *sReadError = "Read error\n";
static const char *sDataError = "Data error\n";

//=============================================================================
static void write_eeprom(uint32_t id, uint8_t *buf, uint32_t size)
{
	if (EEPROM_Write(id, buf, size) > 0) {
		puts("Write succeed.\n");
	} else {
		puts("Write error.\n");
	}
}

//=============================================================================
static void write_eeprom_str(uint32_t id,
							 const char *str, uint8_t *buf, uint32_t size)
{
	if (strlen(str) > size) {
		puts(sDataError);
		return;
	}

	_mem_zero(buf, size);
	strncpy((char *)buf, str, size);

	write_eeprom(id, buf, size);
}

//=============================================================================
static void write_eeprom_ip(uint32_t id, const char *str, uint32_t size)
{
	in_addr temp;

	if (inet_pton(AF_INET, str, &temp, sizeof(temp)) == RTCS_ERROR) {
		puts(sDataError);
		return;
	}

	write_eeprom(id, (uint8_t *)&temp.s_addr, size);
}

//=============================================================================
static void write_eeprom_portno(uint32_t id, uint16_t portno)
{
	uint8_t buf[2];

	buf[0] = (portno >> 8) & 0xff;
	buf[1] = portno & 0xff;

	write_eeprom(id, buf, 2);
}

//=============================================================================
static bool read_eeprom(uint32_t id, uint8_t *buf, uint32_t size)
{
	if (EEPROM_Read(id, buf, size) > 0) {
		return (TRUE);
	} else {
		printf(sReadError);
		return (FALSE);
	}
}

//=============================================================================
static void read_eeprom_str(const char *prefix,
							uint32_t id, uint8_t *buf, uint32_t size)
{
	int cnt;

	if (EEPROM_Read(id, buf, size) > 0) {
		puts(prefix);
		for (cnt = 0; cnt < size && buf[cnt] != '\0'; cnt ++) {
			putchar(buf[cnt]);
		}
		putchar('\n');
	} else {
		puts(sReadError);
	}
}

//=============================================================================
static void read_eeprom_ip(const char *prefix, uint32_t id, uint32_t size)
{
	uint8_t buf[4];
	char ip_str[20];

	if (size != 4) {
		puts(sDataError);
		return;
	}

	if (read_eeprom(id, buf, size)) {
		inet_ntop(AF_INET, buf, ip_str, sizeof(ip_str));
		puts(prefix);
		puts(ip_str);
		putchar('\n');
	}
}

//=============================================================================
static void read_eeprom_portno(const char *prefix, uint32_t id)
{
	uint8_t buf[2];
	uint16_t portno;

	if (read_eeprom(id, buf, 2)) {
		puts(prefix);
		portno = ((uint16_t)buf[0] << 8) + (uint16_t)buf[1];
		printf("%d\n", portno);
	}
}

//=============================================================================
int32_t Shell_eeprom(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	char *type_str, *param;

	print_usage = Shell_check_help_request(argc, argv, &short_help);

	if (print_usage) {
		if (short_help) {
			printf("eeprom <type> [<data>]\n");
		} else {
			printf("Usage: %s", sEepromCmdHelp);
		}
		return (0);
	}

	if ((argc > 3) || (argc < 2)) {
		printf("Usage: %s", sEepromCmdHelp);
		return (0);
	}

	type_str = argv[1];
	if (strcmp(type_str, "mac") == 0) {
		// ---------------------------------------------------- MAC Address ---
		uint8_t mac_addr[FLASHROM_MACADDR_SIZE];
		if (argc == 2) {
			if (read_eeprom(FLASHROM_MACADDR_ID,
							mac_addr, FLASHROM_MACADDR_SIZE)) {
				printf("MAC address : %02x-%02x-%02x-%02x-%02x-%02x\n",
					   mac_addr[0], mac_addr[1], mac_addr[2], 
					   mac_addr[3], mac_addr[4], mac_addr[5]);
			}
		} else {
			int i;
			char *temppoint;
			char *errorpoint;
			unsigned long tempvalue;

			param = argv[2];

			temppoint = strtok(param, "-");
			for(i=1; i<=FLASHROM_MACADDR_SIZE; i++)
			{
				tempvalue = strtoul(temppoint, &errorpoint, 16);

				if ((errorpoint != (temppoint + 2)) || (tempvalue > 255))
				{
  					break;
				}
  				*(mac_addr+i-1) = (uint8_t)tempvalue;
				temppoint   = strtok(NULL, "-");
			}

			if ((i <= FLASHROM_MACADDR_SIZE) || (temppoint != NULL))
			{
				puts(sDataError);
				return (0);
			}

			write_eeprom(FLASHROM_MACADDR_ID, mac_addr, FLASHROM_MACADDR_SIZE);
		}
	}
	else if (strcmp(type_str, "code") == 0) {
		// --------------------------------------------------- Machine Code ---
		uint8_t buf[FLASHROM_MACHINE_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("Machine Number : ",
							FLASHROM_MACHINE_ID, buf, FLASHROM_MACHINE_SIZE);
		} else {
			write_eeprom_str(FLASHROM_MACHINE_ID,
							 argv[2], buf, FLASHROM_MACHINE_SIZE);
		}
	}
	else if (strcmp(type_str, "serial") == 0) {
		// -------------------------------------------------- Serial Number ---
		uint8_t buf[FLASHROM_SERIAL_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("Serial Number : ",
							FLASHROM_SERIAL_ID, buf, FLASHROM_SERIAL_SIZE);
		} else {
			write_eeprom_str(FLASHROM_SERIAL_ID,
							 argv[2], buf, FLASHROM_SERIAL_SIZE);
		}
	}
	else if (strcmp(type_str, "ftpid") == 0) {
		// --------------------------------------------------------- FTP ID ---
		uint8_t buf[FLASHROM_FTPID_SIZE + 1];
		if (argc == 2) {
			if (read_eeprom(FLASHROM_FTPID_ID, buf, FLASHROM_FTPID_SIZE)) {
				printf("FTP domain ID : 0x%02x ", buf[0]);
				if(buf[0] == 0) {
					puts("smk.co.jp\n");
				} else {
					puts("Reserved\n");
				}
			}
		} else {
			uint32_t temp;
			temp = atoi(argv[2]);
			if (temp <= 0xff) {
				buf[0] = (uint8_t)temp;
				write_eeprom(FLASHROM_FTPID_ID, buf, FLASHROM_FTPID_SIZE);
			} else {
				puts(sDataError);
			}
		}
	}
	else if (strcmp(type_str, "ftpsubd") == 0) {
		// -------------------------------------------------- FTP Subdomain ---
		uint8_t buf[FLASHROM_SUBDOMAIN_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("FTP server subdomain : ",
							FLASHROM_SUBDOMAIN_ID, buf, FLASHROM_SUBDOMAIN_SIZE);
		} else {
			write_eeprom_str(FLASHROM_SUBDOMAIN_ID,
							 argv[2], buf, FLASHROM_SUBDOMAIN_SIZE);
		}
	}		
	else if (strcmp(type_str, "ftppath") == 0) {
		// ------------------------------------------------------- FTP Path ---
		uint8_t buf[FLASHROM_PATH_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("FTP server path : ",
							FLASHROM_PATH_ID, buf, FLASHROM_PATH_SIZE);
		} else {
			write_eeprom_str(FLASHROM_PATH_ID,
							 argv[2], buf, FLASHROM_PATH_SIZE);
		}
	}
	else if (strcmp(type_str, "dhcp") == 0) {
		// ----------------------------------------------------------- DHCP ---
		uint8_t buf[FLASHROM_DHCP_SIZE + 1];
		if (argc == 2) {
			if (read_eeprom(FLASHROM_DHCP_ID, buf, FLASHROM_DHCP_SIZE)) {
				puts("DHCP setting : ");
				if (buf[0] == 0) {
					puts("Disable\n");
				} else {
					puts("Enable\n");
				}
			}
		} else {
			if (strcmp(argv[2], "enable") == 0) {
				buf[0] = (uint8_t)true;
				write_eeprom(FLASHROM_DHCP_ID, buf, FLASHROM_DHCP_SIZE);
			}
			else if (strcmp(argv[2], "disable") == 0) {
				buf[0] = (uint8_t)false;
				write_eeprom(FLASHROM_DHCP_ID, buf, FLASHROM_DHCP_SIZE);
			}
			else {
				puts(sDataError);
			}
		}
	}
	else if (strcmp(type_str, "ip") == 0) {
		// ---------------------------------------------- Static IP Address ---
		if (argc == 2) {
			read_eeprom_ip("Static IP address setting : ",
						   FLASHROM_STATICIP_ID, FLASHROM_STATICIP_SIZE);
		} else {
			write_eeprom_ip(FLASHROM_STATICIP_ID,
							(const char *)argv[2], FLASHROM_STATICIP_SIZE);
		}
	}
	else if (strcmp(type_str, "netmask") == 0) {
		// -------------------------------------------------------- Netmask ---
		if (argc == 2) {
			read_eeprom_ip("Static subnet mask setting : ",
						   FLASHROM_SUBNET_ID, FLASHROM_SUBNET_SIZE);
		} else {
			write_eeprom_ip(FLASHROM_SUBNET_ID,
							(const char *)argv[2], FLASHROM_SUBNET_SIZE);
		}
	}
	else if (strcmp(type_str, "gateway") == 0) {
		// -------------------------------------------------------- Gateway ---
		if (argc == 2) {
			read_eeprom_ip("Static gateway address setting : ",
						   FLASHROM_GATEWAY_ID, FLASHROM_GATEWAY_SIZE);
		} else {
			write_eeprom_ip(FLASHROM_GATEWAY_ID,
							(const char *)argv[2], FLASHROM_GATEWAY_SIZE);
		}
	}
	// ------------------------------------------------- SGW WebSocket Host ---
	else if (strcmp(type_str, "sgwhost") == 0) {
		uint8_t buf[FLASHROM_SGWS_HOST_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("SGW Host : ", FLASHROM_SGWS_HOST_ID,
							buf, FLASHROM_SGWS_HOST_SIZE);
		} else {
			write_eeprom_str(FLASHROM_SGWS_HOST_ID,
							 argv[2], buf, FLASHROM_SGWS_HOST_SIZE);
		}
	}
	// ------------------------------------------------- SGW WebSocket Path ---
	else if (strcmp(type_str, "sgwport") == 0) {
		if (argc == 2) {
			read_eeprom_portno("SGW PortNo : ", FLASHROM_SGWS_PORTNO_ID);
		} else {
			uint16_t portno;
			portno = atoi(argv[2]);
			write_eeprom_portno(FLASHROM_SGWS_PORTNO_ID, portno);
		}
	}
	// ------------------------------------------------- SGW WebSocket Path ---
	else if (strcmp(type_str, "sgwpath") == 0) {
		uint8_t buf[FLASHROM_SGWS_PATH_SIZE + 1];
		if (argc == 2) {
			read_eeprom_str("SGW Path : ", FLASHROM_SGWS_PATH_ID,
							buf, FLASHROM_SGWS_PATH_SIZE);
		} else {
			write_eeprom_str(FLASHROM_SGWS_PATH_ID,
							 argv[2], buf, FLASHROM_SGWS_PATH_SIZE);
		}
	}

	else {
		printf("Usage: %s", sEepromCmdHelp);
		return (0);
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
