#include "../Tasks.h"
#include "web.h"
#include "rtc.h"
#include "mqx.h"
#include "rtcs.h"
#include "dns.h"
#include "httpsrv.h"


//#if DEMOCFG_ENABLE_WEBSERVER
#include "html.h"
#include "cgi.h"

#include <string.h>
#include <stdlib.h>


const HTTPSRV_CGI_LINK_STRUCT cgi_lnk_tbl[] = {
  { "cgi_test",       cgi_test       },
  { "test/cgi_test",  cgi_test       },
  { "cgi_time",       cgi_time       },
  { "cgi_oneshot",    cgi_oneshot    },
  { "cgi_form",       cgi_form       },
  { "cgi_data",       cgi_data       },
  { "cgi_DtoI",       cgi_DtoI       },
  { "cgi_ItoD",       cgi_ItoD       },
  { "cgi_GMaC",       cgi_GMaC       },
  { 0, 0 }    // DO NOT REMOVE - last item - end of table
};

_mqx_int cgi_test(HTTPSRV_CGI_REQ_STRUCT* param)
{

  //cgi test_test
  HTTPSRV_CGI_RES_STRUCT cgi_send_test;
  uint32_t length = 0;
  char str[150];

  cgi_send_test.ses_handle = param->ses_handle;
  cgi_send_test.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_test.status_code = 200;

  length = sprintf(str,"<!DOCTYPE html>\n<html>\n<head>\n\t<title>MQX HTTP Server Test</title>\n</head>\n<body>\n\t<h1>test_OK</h1>\n</body>\n</html>\n");

  cgi_send_test.data = str;
  cgi_send_test.data_length = length;
  cgi_send_test.content_length = cgi_send_test.data_length;

  HTTPSRV_cgi_write(&cgi_send_test);

  return (cgi_send_test.content_length);

}



_mqx_int cgi_time(HTTPSRV_CGI_REQ_STRUCT* param)
{
  DATE_STRUCT cgi_date;
  TIME_STRUCT cgi_Unix_time;
  char cUNIX_time[15],cDate[30];

  HTTPSRV_CGI_RES_STRUCT cgi_send_time;
  uint32_t length[4] = {0,0,0,0},length_tol = 0;
  char str_0[150];
  char str_1[150];
  char str_2[150];
  char str_3[150];

  _time_get(&cgi_Unix_time);
  cgi_Unix_time.SECONDS += 32400;
  _time_to_date(&cgi_Unix_time,&cgi_date);

  sprintf(cUNIX_time,"%d",cgi_Unix_time.SECONDS);
  sprintf(cDate,"JP_Time %d年%2d月%2d日  %2d:%2d:%2d",cgi_date.YEAR,cgi_date.MONTH,cgi_date.DAY,cgi_date.HOUR,cgi_date.MINUTE,cgi_date.SECOND);


  cgi_send_time.ses_handle = param->ses_handle;
  cgi_send_time.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_time.status_code = 200;


  length_tol += length[0] = sprintf(str_0,"<!DOCTYPE html>\n<html>\n<head>\n\t<title>MQX HTTP Server NTP Time Test</title>\n</head>\n<body>\n\t<h1>NTP_Time</h1>\n");

  length_tol += length[1] = sprintf(str_1,"\t<font size=\"4\">Unix time</f><br>\n\t<font size=\"6\">%s</f><br>\n",cUNIX_time);

  length_tol += length[2] = sprintf(str_2,"\t<font size=\"4\">date</f><br>\n\t<font size=\"6\">%s</f><br>\n",cDate);

  length_tol += length[3] = sprintf(str_3,"<a href=\"http:cgi_oneshot.cgi\">OneShot</a></body>\n</html>");


  cgi_send_time.data = str_0;
  cgi_send_time.data_length = length[0];
  //cgi_send_time.content_length = length_tol;
  cgi_send_time.content_length = length_tol;

  HTTPSRV_cgi_write(&cgi_send_time);

  cgi_send_time.data = str_1;
  cgi_send_time.data_length = length[1];
  //cgi_send_time.content_length = cgi_send_time.content_length + cgi_send_time.data_length;

  HTTPSRV_cgi_write(&cgi_send_time);


  cgi_send_time.data = str_2;
  cgi_send_time.data_length = length[2];
  //cgi_send_time.content_length = cgi_send_time.content_length + cgi_send_time.data_length;

  HTTPSRV_cgi_write(&cgi_send_time);

  cgi_send_time.data = str_3;
  cgi_send_time.data_length = length[3];
  //cgi_send_time.content_length = cgi_send_time.content_length + cgi_send_time.data_length;

  HTTPSRV_cgi_write(&cgi_send_time);


  return (cgi_send_time.content_length);
}

_mqx_int cgi_oneshot(HTTPSRV_CGI_REQ_STRUCT* param)
{
  DATE_STRUCT cgi_date;
  TIME_STRUCT cgi_Unix_time;
  char cUNIX_time[15],cDate[30];

  HTTPSRV_CGI_RES_STRUCT cgi_send_time;
  uint32_t length[4] = {0,0,0,0},length_tol = 0,error = 0;
  char str_0[150];


  error = SNTP_oneshot(0xC0A8F0AB,1);
  //error = SNTP_oneshot(0x85F3EEA3,1);


  cgi_send_time.ses_handle = param->ses_handle;
  cgi_send_time.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_time.status_code = 200;


  if(error == RTCS_OK){
  	length_tol += length[0] = sprintf(str_0,"<html>\n<head>\n<title>aute jump</title>\n<meta http-equiv=\"REFRESH\" content=\"0;URL=http:cgi_time.cgi\">\n</head>\n</body>\n</html>");
  }else if(error == RTCSERR_TIMEOUT){
  	length_tol += length[0] = sprintf(str_0,"<html><body>TimeOut</body></html>");
  }else{
  	length_tol += length[0] = sprintf(str_0,"<html><body>error</body></html>");
  }



  cgi_send_time.data = str_0;
  cgi_send_time.data_length = length[0];
  //cgi_send_time.content_length = length_tol;
  cgi_send_time.content_length = length_tol;

  HTTPSRV_cgi_write(&cgi_send_time);



  return (cgi_send_time.content_length);
}


_mqx_int cgi_ItoD(HTTPSRV_CGI_REQ_STRUCT* param)
{

  HTTPSRV_CGI_RES_STRUCT cgi_send_ItoD;
  HOSTENT_STRUCT *ItoD_data;

  char 	cAddr_x[50],
  			cgi_post[200],
			*cName,
			*cDomein;
  uint32_t 	return_Number	= 0,
  			html_count		= 0,
			addr_count		= 0,
			i				= 0,
			input_IP[4]		= {0,0,0,0};
  in_addr	IPdata;

  for(i=0;i<200;i++){
	cgi_post[i]='\0';
  }


//クライアントの入力したデータを受け取る
  return_Number = HTTPSRV_cgi_read(param->ses_handle,cgi_post,sizeof(cgi_post));
//取得したデータを分解
  cName = strtok(cgi_post,"=");
  for(i=0;i<4;i++){
	input_IP[i] = atoi(strtok(NULL,"&"));
    cName = strtok(NULL,"=");
  }
  IPdata.s_addr = IPADDR(input_IP[3],input_IP[2],input_IP[1],input_IP[0]);

//DNSリゾルバ設定
  char DNS_Lnn[] = ".";
  char DNS_Lsn[] = "";

  DNS_First_Local_server[0].NAME_PTR = DNS_Lsn;

  DNS_First_Local_server[0].IPADDR = ENET_IPGATEWAY;
//  DNS_First_Local_server[0].IPADDR = 0xC0A8F0AB;
//IPアドレス->ドメイン名
  ItoD_data = gethostbyaddr((char *)&IPdata,sizeof(IPdata),AF_INET);

  if(ItoD_data != NULL){
	//変数リセット
    for(i=0;i<200;i++){
      cgi_post[i]='\0';
    }

	addr_count += snprintf(cgi_post,199,"Domein : %s<br>\n",ItoD_data->h_name);


	//レスポンスデータ準備
	  cgi_send_ItoD.ses_handle = param->ses_handle;
	  cgi_send_ItoD.content_type = HTTPSRV_CONTENT_TYPE_HTML;
	  cgi_send_ItoD.status_code = 200;

	  cgi_send_ItoD.data = cgi_post;
	  cgi_send_ItoD.data_length = addr_count;
	  cgi_send_ItoD.content_length = cgi_send_ItoD.data_length;

	//レスポンスとしてHTMLデータ出力
	  HTTPSRV_cgi_write(&cgi_send_ItoD);
  }else{
      addr_count = sprintf(cgi_post,"Not Found");

	  //レスポンスデータ準備
	  cgi_send_ItoD.ses_handle = param->ses_handle;
	  cgi_send_ItoD.content_type = HTTPSRV_CONTENT_TYPE_HTML;
	  cgi_send_ItoD.status_code = 200;

	  cgi_send_ItoD.data = cgi_post;
	  cgi_send_ItoD.data_length = addr_count;
	  cgi_send_ItoD.content_length = cgi_send_ItoD.data_length;

	//レスポンスとしてHTMLデータ出力
	  HTTPSRV_cgi_write(&cgi_send_ItoD);
  }

  return (cgi_send_ItoD.content_length);
}


_mqx_int cgi_DtoI(HTTPSRV_CGI_REQ_STRUCT* param)
{

  HTTPSRV_CGI_RES_STRUCT cgi_send_DtoI;
  HOSTENT_STRUCT *DtoI_data;
  char 		cAddr_x[50],
  			cgi_post[200],
			*cName,
			*cDomein;
  uint32_t 	return_Number	= 0,
  			html_count		= 0,
			addr_count		= 0,
			i				= 0;


  for(i=0;i<200;i++){
	cgi_post[i]='\0';
  }


//クライアントの入力したデータを受け取る
  return_Number = HTTPSRV_cgi_read(param->ses_handle,cgi_post,sizeof(cgi_post));
//取得したデータを分解
  cName = strtok(cgi_post,"=");
  cDomein = strtok(NULL,"&");

//DNSリゾルバ設定
  char DNS_Lnn[] = ".";
  char DNS_Lsn[] = "";

  DNS_First_Local_server[0].NAME_PTR = DNS_Lsn;
  DNS_First_Local_server[0].IPADDR = ENET_IPGATEWAY;
//  DNS_First_Local_server[0].IPADDR = 0xC0A8F0AB;

//ドメイン名->IPアドレスリスト
  DtoI_data = gethostbyname(cDomein);

  if(DtoI_data != NULL){
	//変数リセット
	  for(i=0;i<200;i++){
		cgi_post[i]='\0';
	  }
	  i=0;

	//取得データ文字列化
	  while((DtoI_data->h_addr_list[i] != NULL) && (i < 5)){
		addr_count += sprintf(cAddr_x,
							 "%i : %d.%d.%d.%d<br>\n",
							 i,
							 ((*(uint32_t*)DtoI_data->h_addr_list[i]) >> 24) & 0xFF,
							 ((*(uint32_t*)DtoI_data->h_addr_list[i]) >> 16) & 0xFF,
							 ((*(uint32_t*)DtoI_data->h_addr_list[i]) >>  8) & 0xFF,
							  (*(uint32_t*)DtoI_data->h_addr_list[i])        & 0xFF);

		strcat(cgi_post,cAddr_x);
		i++;

	  }

	//レスポンスデータ準備
	  cgi_send_DtoI.ses_handle = param->ses_handle;
	  cgi_send_DtoI.content_type = HTTPSRV_CONTENT_TYPE_HTML;
	  cgi_send_DtoI.status_code = 200;

	  cgi_send_DtoI.data = cgi_post;
	  cgi_send_DtoI.data_length = addr_count;
	  cgi_send_DtoI.content_length = cgi_send_DtoI.data_length;

	//レスポンスとしてHTMLデータ出力
	  HTTPSRV_cgi_write(&cgi_send_DtoI);
  }else{
      addr_count = sprintf(cgi_post,"Not Found");

	  //レスポンスデータ準備
	  cgi_send_DtoI.ses_handle = param->ses_handle;
	  cgi_send_DtoI.content_type = HTTPSRV_CONTENT_TYPE_HTML;
	  cgi_send_DtoI.status_code = 200;

	  cgi_send_DtoI.data = cgi_post;
	  cgi_send_DtoI.data_length = addr_count;
	  cgi_send_DtoI.content_length = cgi_send_DtoI.data_length;

	//レスポンスとしてHTMLデータ出力
	  HTTPSRV_cgi_write(&cgi_send_DtoI);
  }

  return (cgi_send_DtoI.content_length);
}

_mqx_int cgi_form(HTTPSRV_CGI_REQ_STRUCT* param)
{
  char cgi_post[100];
  uint32_t return_Number = 0,i;

  for(i=0;i<100;i++){
	cgi_post[i]='\0';
  }

  return_Number = HTTPSRV_cgi_read(param->ses_handle,cgi_post,sizeof(cgi_post));


  //cgi test_test
  HTTPSRV_CGI_RES_STRUCT cgi_send_test;
  uint32_t length = 0;
  char str[150];

  cgi_send_test.ses_handle = param->ses_handle;
  cgi_send_test.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_test.status_code = 200;

  length = sprintf(str,"<!DOCTYPE html>\n<html>\n<head>\n\t<title>MQX HTTP Server Test</title>\n</head>\n<body>\n\t<h1>%s</h1><br>\n%d</body>\n</html>\n",cgi_post,return_Number);

  cgi_send_test.data = str;
  cgi_send_test.data_length = length;
  cgi_send_test.content_length = cgi_send_test.data_length;

  HTTPSRV_cgi_write(&cgi_send_test);

  return (cgi_send_test.content_length);

}


_mqx_int cgi_data(HTTPSRV_CGI_REQ_STRUCT* param)
{

  //cgi test_test
  HTTPSRV_CGI_RES_STRUCT cgi_send_data;
  uint32_t length = 0;
  char str[300];

  cgi_send_data.ses_handle = param->ses_handle;
  cgi_send_data.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_data.status_code = 200;

  param->ses_handle;
  param->request_method;
  param->content_type;
  param->content_length;
  param->server_port;
  param->remote_addr;
  param->server_name;
  param->script_name;
  param->server_protocol;
  param->server_software;
  param->query_string;
  param->gateway_interface;
  param->remote_user;
  param->auth_type;


  length = snprintf(str,300,"<http><body>rA:%s<br>svN:%s<br>scN:%s<br>svP:%s<br>svS:%s<br>qS:%s<br>gI:%s<br>rU:%s<br></body></html>",param->remote_addr,param->server_name,param->script_name,param->server_protocol,param->server_software,param->query_string,param->gateway_interface,param->remote_user);

  cgi_send_data.data = str;
  cgi_send_data.data_length = length;
  cgi_send_data.content_length = cgi_send_data.data_length;

  HTTPSRV_cgi_write(&cgi_send_data);

  return (cgi_send_data.content_length);

}

_mqx_int cgi_GMaC(HTTPSRV_CGI_REQ_STRUCT* param)
{

  //cgi test_test
  HTTPSRV_CGI_RES_STRUCT cgi_send_test;
  uint32_t length = 0;
  char str[150];

  cgi_send_test.ses_handle = param->ses_handle;
  cgi_send_test.content_type = HTTPSRV_CONTENT_TYPE_HTML;
  cgi_send_test.status_code = 418;

  length = sprintf(str,"<!DOCTYPE html>\n<html>\n<head>\n\t<title>MQX HTTP Server Test</title>\n</head>\n<body>\n\t<h1>Page not Found</h1>\n</body>\n</html>\n");

  cgi_send_test.data = str;
  cgi_send_test.data_length = length;
  cgi_send_test.content_length = cgi_send_test.data_length;

  HTTPSRV_cgi_write(&cgi_send_test);

  return (cgi_send_test.content_length);

}
