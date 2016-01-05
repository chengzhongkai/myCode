#ifndef _WEB_H
#define _WEB_H

#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include  <ipcfg.h>

#include "dns.h"
#include "httpsrv.h"

#define DEMOCFG_ENABLE_RTCS			1	/* enable RTCS operation */
#define DEMOCFG_ENABLE_DHCP			1	/* enable DHCP Client */

uint32_t initialize_HTTPServer(void);

#endif /* !_WEB_H */
