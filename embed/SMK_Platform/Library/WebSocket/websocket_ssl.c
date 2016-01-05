/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"
#include "websocket.h"
#include "websocket_prv.h"

#include "tls.h"

//=============================================================================
static void *WS_OpenSSL(void)
{
	TLS_Handle_t *handle;

	const TLS_CipherSuite_t cipher_suites[] = {
		TLS_RSA_WITH_AES_128_CBC_SHA,
		TLS_RSA_WITH_AES_256_CBC_SHA,
		TLS_RSA_WITH_3DES_EDE_CBC_SHA
	};

	handle = TLS_CreateHandle(TLS_VER_1_0);
	if (handle == NULL) return (NULL);

	TLS_SetCipherSuites(handle, cipher_suites, 3);

	return (handle);
}

//=============================================================================
static bool WS_ConnectSSL(void *handle, uint32_t ipaddr, uint32_t portno)
{
	TLS_Handle_t *tls = handle;

	if (tls == NULL) return (FALSE);

	TLS_SetHostname(tls, ipaddr, portno);

	return (TLS_Connect(tls));
}

//=============================================================================
static void WS_CloseSSL(void *handle)
{
	TLS_Handle_t *tls = handle;

	if (tls == NULL) return;

	TLS_Shutdown(tls);
	TLS_FreeHandle(tls);
}

//=============================================================================
static uint32_t WS_SendSSL(void *handle, uint8_t *buf, uint32_t len)
{
	TLS_Handle_t *tls = handle;

	if (tls == NULL) return (0);

	return (TLS_Send(tls, buf, len));
}

//=============================================================================
static uint32_t WS_RecvSSL(void *handle, uint8_t *buf, uint32_t len)
{
	TLS_Handle_t *tls = handle;

	if (tls == NULL) return (0);

	return (TLS_Recv(tls, buf, len));
}

//=============================================================================
void WS_SetNetIF_SSL(WS_NetIF_t *netif)
{
	netif->open = WS_OpenSSL;
	netif->connect = WS_ConnectSSL;
	netif->close = WS_CloseSSL;
	netif->send = WS_SendSSL;
	netif->recv = WS_RecvSSL;
}

/******************************** END-OF-FILE ********************************/
