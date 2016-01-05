#include "web.h"
#include "tfs.h"
#include "rtcs.h"
#include "dns.h"
#include "httpsrv.h"
#include "shell.h"
#include "sh_rtcs.h"

#include "NetConnect.h"

#include  <ipcfg.h>
#include <mqx.h>
#include <bsp.h>

uint32_t initialize_HTTPServer(void)
{
	HTTPSRV_PARAM_STRUCT params;
	extern const TFS_DIR_ENTRY	tfs_data[];
	HTTPSRV_CGI_LINK_STRUCT cgi_lnk_tbl[] = {0,0};
	uint32_t server,error;

	/*-- param clear --*/
	_mem_zero(&params,sizeof(HTTPSRV_PARAM_STRUCT));

	/*-- TFS start --*/
	error = _io_tfs_install("tfs:", tfs_data);

	/*-- HTTP Server param set --*/
	params.root_dir = "tfs:";
	params.index_page = "\\index.html";
	params.af = AF_INET;
	//params.cgi_lnk_tbl = (HTTPSRV_CGI_LINK_STRUCT*) cgi_lnk_tbl;
	params.script_stack = 3000;

	/*-- HTTP Server Start --*/
	server = HTTPSRV_init(&params);
	if(!server){
		printf("Errir: HTTP server init error.\n");
	}

	return (MQX_OK);
}
