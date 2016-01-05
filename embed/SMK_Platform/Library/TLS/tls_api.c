/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "tls.h"
#include "tls_prv.h"

//=============================================================================
TLS_Handle_t *TLS_CreateHandle(TLS_Version_t ver)
{
	TLS_Handle_t *handle;

	// ------------------------------------ Implementation for Only TLS 1.0 ---
	if (ver != TLS_VER_1_0) {
		return (NULL);
	}

	// ---------------------------------------------------- Allocate Memory ---
	handle = TLS_malloc(sizeof(TLS_Handle_t));
	if (handle == NULL) {
		return (NULL);
	}

	// --------------------------------------------------------- Initialize ---
	TLS_memset(handle, 0, sizeof(TLS_Handle_t));
	handle->ver = ver;
	handle->sock = SOCK_INVALID;

	return (handle);
}

//=============================================================================
void TLS_FreeHandle(TLS_Handle_t *handle)
{
	if (handle == NULL) return;

	TLS_SockClose(handle);

	if (handle->key_block != NULL) {
		TLS_free(handle->key_block);
	}
	if (handle->cipher_suites != NULL) {
		TLS_free(handle->cipher_suites);
	}
	if (handle->send_cipher_algo != NULL) {
		TLS_free(handle->send_cipher_algo);
	}
	if (handle->recv_cipher_algo != NULL) {
		TLS_free(handle->recv_cipher_algo);
	}


	TLS_free(handle);
}

//=============================================================================
bool TLS_SetHostname(TLS_Handle_t *handle, uint32_t ipaddr, uint32_t portno)
{
	if (handle == NULL) return (FALSE);

	handle->ipaddr = ipaddr;
	handle->portno = portno;

	handle->flag |= TLS_SET_HOSTNAME;

	return (TRUE);
}

//=============================================================================
bool TLS_SetCipherSuites(TLS_Handle_t *handle,
						 const TLS_CipherSuite_t *list, uint32_t num_items)
{
	uint32_t size;

	if (handle == NULL || list == NULL || num_items == 0) return (FALSE);

	size = sizeof(TLS_CipherSuite_t) * num_items;
	handle->cipher_suites = TLS_malloc(size);
	if (handle->cipher_suites == NULL) {
		return (FALSE);
	}

	TLS_memcpy(handle->cipher_suites, list, size);
	handle->num_cipher_suites = num_items;

	handle->flag |= TLS_SET_CIPHER_SUITES;

	return (TRUE);
}

//=============================================================================
bool TLS_SetSessionInfo(TLS_Handle_t *handle, TLS_SessionInfo_t *info)
{
	if (handle == NULL || info == NULL) return (FALSE);

	TLS_memcpy(&handle->session_info, info, sizeof(TLS_SessionInfo_t));

	handle->flag |= (TLS_VALID_SESSION_ID | TLS_VALID_MASTER_SECRET);

	return (TRUE);
}						

//=============================================================================
bool TLS_GetSessionInfo(TLS_Handle_t *handle, TLS_SessionInfo_t *info)
{
	if (handle == NULL || info == NULL) return (FALSE);

	if ((handle->flag & (TLS_VALID_SESSION_ID | TLS_VALID_MASTER_SECRET))
		== (TLS_VALID_SESSION_ID | TLS_VALID_MASTER_SECRET)) {
		TLS_memcpy(info, &handle->session_info, sizeof(TLS_SessionInfo_t));
	} else {
		TLS_memset(info, 0, sizeof(TLS_SessionInfo_t));
	}

	return (TRUE);
}

//=============================================================================
static bool TLS_WaitChangeCipherSpec(TLS_Handle_t *handle)
{
	TLS_RecordHeader_t header;
	uint8_t *msg = NULL;
	uint32_t m_len;
	bool ret = FALSE;

	do {
		if (!TLS_RecvMsg(handle, &header, &msg, &m_len)) {
			break;
		}
		if (header.type != TLS_MSG_CHANGE_CIPHER_SPEC) {
			break;
		}
		if (!TLS_RecvChangeCipherSpec(handle)) {
			break;
		}
		ret = TRUE;
	} while (0);

	if (msg != NULL) {
		TLS_free(msg);
	}

	return (ret);
}

//=============================================================================
static bool TLS_WaitFinished(TLS_Handle_t *handle)
{
	TLS_RecordHeader_t header;
	uint8_t *msg = NULL;
	uint32_t m_len;
	bool ret = FALSE;

	do {
		if (!TLS_RecvMsg(handle, &header, &msg, &m_len)) {
			break;
		}
		if (header.type != TLS_MSG_HANDSHAKE) {
			break;
		}
		if (!TLS_RecvFinished(handle, msg, m_len)) {
			break;
		}
		ret = TRUE;
	} while (0);

	if (msg != NULL) {
		TLS_free(msg);
	}

	return (ret);
}

//=============================================================================
bool TLS_Connect(TLS_Handle_t *handle)
{
	TLS_RecordHeader_t header;
	uint8_t *msg = NULL;
	uint32_t m_len;
	bool loop;

	if (handle == NULL) return (FALSE);

	if ((handle->flag & TLS_CONNECT_AVAILABLE) != TLS_CONNECT_AVAILABLE) {
		return (FALSE);
	}

	// -------------------------------------------- Open Socket and Connect ---
	if (!TLS_SockOpen(handle)) return (FALSE);

	// ---------------------------------------------------------- Handshake ---
	if (!TLS_SetupHandshake(handle)) {
		TLS_SockClose(handle);
		return (FALSE);
	}

	// --------------------------------------------------- Send ClientHello ---
	if (!TLS_SendClientHello(handle)) {
		goto TLS_Connect_error;
	}

	// --------------------------------------------------- Wait ServerHello ---
	msg = NULL;
	if (!TLS_RecvMsg(handle, &header, &msg, &m_len)) {
		goto TLS_Connect_error;
	}
	if (header.type != TLS_MSG_HANDSHAKE) {
		goto TLS_Connect_error;
	}
	if (!TLS_RecvServerHello(handle, msg, m_len)) {
		goto TLS_Connect_error;
	}
	if (msg != NULL) {
		TLS_free(msg);
		msg = NULL;
	}

	// --------------------------- When Reconnection, Wait ChangeCipherSpec ---
	if ((handle->flag & TLS_VALID_MASTER_SECRET) != 0) {
		// ------------------------------------------ Wait ChangeCipherSpec ---
		if (!TLS_WaitChangeCipherSpec(handle)) {
			goto TLS_Connect_error;
		}

		// -------------------------------------------------- Wait Finished ---
		if (!TLS_WaitFinished(handle)) {
			goto TLS_Connect_error;
		}

		// ------------------------------------------ Send ChangeCipherSpec ---
		if (!TLS_SendChangeCipherSpec(handle)) {
			goto TLS_Connect_error;
		}

		// -------------------------------------------------- Send Finished ---
		if (!TLS_SendFinished(handle)) {
			goto TLS_Connect_error;
		}

		// ------------------------------------------------------- Clean up ---
		TLS_CleanupHandshake(handle);

		return (TRUE);
	}

	// ------------------------- Wait Server Messsage Until ServerHelloDone ---
	loop = TRUE;
	while (loop) {
		msg = NULL;
		if (!TLS_RecvMsg(handle, &header, &msg, &m_len)) {
			goto TLS_Connect_error;
		}
		if (header.type != TLS_MSG_HANDSHAKE) {
			goto TLS_Connect_error;
		}
		switch ((TLS_HandshakeType_t)msg[0]) {
		case TLS_HANDSHAKE_CERTIFICATE:
			if (!TLS_RecvCertificate(handle, msg, m_len)) {
				goto TLS_Connect_error;
			}
			break;
		case TLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
			if (!TLS_RecvServerKeyExchange(handle, msg, m_len)) {
				goto TLS_Connect_error;
			}
			break;
		case TLS_HANDSHAKE_CERTIFICATE_REQUEST:
			if (!TLS_RecvCertificateRequest(handle, msg, m_len)) {
				goto TLS_Connect_error;
			}
			break;
		case TLS_HANDSHAKE_SERVER_HELLO_DONE:
			if (!TLS_RecvServerHelloDone(handle, msg, m_len)) {
				goto TLS_Connect_error;
			}
			loop = FALSE;
			break;
		default:
			goto TLS_Connect_error;
			break;
		}
		if (msg != NULL) {
			TLS_free(msg);
			msg = NULL;
		}
	}

	// ---------------------------------- Send ClientCertificate, if needed ---
	// TLS_SendCertificate(handle);

	// --------------------------------------------- Send ClientKeyExchange ---
	if (!TLS_SendClientKeyExchange(handle)) {
		goto TLS_Connect_error;
	}

	// ---------------------------------- Send CertificateVerify, if needed ---
	// TLS_SendCertificateVerify(handle);

	// ---------------------------------------------- Send ChangeCipherSpec ---
	if (!TLS_SendChangeCipherSpec(handle)) {
		goto TLS_Connect_error;
	}

	// ------------------------------------------------------ Send Finished ---
	if (!TLS_SendFinished(handle)) {
		goto TLS_Connect_error;
	}

	// ---------------------------------------------- Wait ChangeCipherSpec ---
	if (!TLS_WaitChangeCipherSpec(handle)) {
		goto TLS_Connect_error;
	}

	// ------------------------------------------------------ Wait Finished ---
	if (!TLS_WaitFinished(handle)) {
		goto TLS_Connect_error;
	}

	// ----------------------------------------------------------- Clean up ---
	TLS_CleanupHandshake(handle);

	return (TRUE);

TLS_Connect_error:
	if (msg != NULL) {
		TLS_free(msg);
	}

	TLS_CleanupHandshake(handle);

	TLS_SockClose(handle);

	return (FALSE);
}

//=============================================================================
uint32_t TLS_Send(TLS_Handle_t *handle, uint8_t *buf, uint32_t len)
{
	if (!TLS_SendMsg(handle,
					 TLS_MSG_APPLICATION_DATA, handle->ver, buf, len)) {
		return (0);
	}

	return (len);
}

//=============================================================================
uint32_t TLS_Recv(TLS_Handle_t *handle, uint8_t *buf, uint32_t len)
{
	TLS_RecordHeader_t header;
	uint8_t *msg = NULL;
	uint32_t m_len;

	if (handle == NULL || buf == NULL || len == 0) {
#ifdef __FREESCALE_MQX__
		return (RTCS_ERROR);
#else
		return (0);
#endif
	}

	while (1) {
		if (!TLS_RecvMsg(handle, &header, &msg, &m_len)) {
			printf("TLS Recv error or Connection Closed\n");
#ifdef __FREESCALE_MQX__
			return (RTCS_ERROR);
#else
			return (0);
#endif
		}

		if (header.type == TLS_MSG_APPLICATION_DATA && m_len > 0) break;

		TLS_free(msg);
		msg = NULL;
	}

	if (m_len <= len) {
		TLS_memcpy(buf, msg, m_len);
	} else {
		TLS_memcpy(buf, msg, len);
		m_len = len;

		/*** need to store remained bytes, for next recv ***/
	}
	TLS_free(msg);

	printf("TLS Recv %d bytes\n", m_len);

	return (m_len);
}

//=============================================================================
void TLS_Shutdown(TLS_Handle_t *handle)
{
	TLS_SockClose(handle);
}

/******************************** END-OF-FILE ********************************/
