#ifndef _cgi_h_
#define _cgi_h_

_mqx_int cgi_test(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_time(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_form(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_data(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_oneshot(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_DtoI(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_ItoD(HTTPSRV_CGI_REQ_STRUCT* param);
_mqx_int cgi_GMaC(HTTPSRV_CGI_REQ_STRUCT* param);

#endif