/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>

#include <rtcs.h>
#include <ipcfg.h>

#include <ctype.h>

//=============================================================================
static void usage(void)
{
	printf("Usage: dns <domain name>\n   Loopup DNS Address\n");
}

//=============================================================================
int32_t Shell_dns(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	// -------------------------------------------------------------- Usage ---
	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("dns <domain name>\n");
		} else {
			usage();
		}
		return (0);
	}
	if (argc != 2) {
		usage();
		return (0);
	}

	HOSTENT_STRUCT_PTR dnsdata;
	_ip_address ipaddr;

	// ------------------------------------ Get IP Address from Domain Name ---
	dnsdata = gethostbyname(argv[1]);
	if (dnsdata == NULL || *(uint32_t *)dnsdata->h_addr_list[0] == 0) {
		puts("dns: error\n");
		return (0);
	}
	ipaddr = *(uint32_t *)dnsdata->h_addr_list[0];

	printf("%s: %d.%d.%d.%d\n", argv[1],
		   (uint8_t)((ipaddr >> 24) & 0xff), (uint8_t)((ipaddr >> 16) & 0xff),
		   (uint8_t)((ipaddr >>  8) & 0xff), (uint8_t)( ipaddr        & 0xff));

	return (0);
}

/******************************** END-OF-FILE ********************************/
