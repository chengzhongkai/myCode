/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#ifndef _TLS_H
#define _TLS_H

#include <mqx.h>
#include <bsp.h>

#include "hash.h"

//=============================================================================
#define TLS_CHECK_CERTIFICATE 0

//=============================================================================
#ifdef __FREESCALE_MQX__
typedef uint32_t tls_socket_t;
#else
typedef int tls_socket_t;
#endif

#define TLS_SERVER_RANDOM_LEN 32
#define TLS_CLIENT_RANDOM_LEN 32
#define TLS_MASTER_SECRET_LEN 48
#define TLS_PREMASTER_SECRET_LEN 48
#define TLS_SESSION_ID_LEN 32

#define TLS_MASTER_SECRET_LABEL "master secret"
#define TLS_CLIENT_FINISHED_LABEL "client finished"
#define TLS_SERVER_FINISHED_LABEL "server finished"

#define TLS_VERIFY_DATA_LEN 12

#define TLS_DEFAULT_PORTNO (443)

//=============================================================================
#define TLS_SET_HOSTNAME        (0x01)
#define TLS_SET_CIPHER_SUITES   (0x02)
#define TLS_VALID_SESSION_ID    (0x04)
#define TLS_VALID_MASTER_SECRET (0x08)
#define TLS_CHANGE_CIPHER_SPEC  (0x10)
#define TLS_CONNECT_AVAILABLE (TLS_SET_HOSTNAME | TLS_SET_CIPHER_SUITES)

//=============================================================================
typedef enum {
	TLS_VER_NONE = 0,

	TLS_VER_SSL2 = 0x0002, // not used
	TLS_VER_SSL3 = 0x0300, // not used

	TLS_VER_1_0 = 0x0301,
	TLS_VER_1_1 = 0x0302,
	TLS_VER_1_2 = 0x0303
} TLS_Version_t;

//=============================================================================
typedef enum {
	// TLS_*_WITH_*_*
	TLS_NULL_WITH_NULL_NULL             = 0x0000,

    // Server provides RSA certificate
    TLS_RSA_WITH_NULL_MD5               = 0x0001,
    TLS_RSA_WITH_NULL_SHA               = 0x0002,
    TLS_RSA_WITH_NULL_SHA256            = 0x003B,
    TLS_RSA_WITH_RC4_128_MD5            = 0x0004,
    TLS_RSA_WITH_RC4_128_SHA            = 0x0005,
    TLS_RSA_WITH_3DES_EDE_CBC_SHA       = 0x000A,
    TLS_RSA_WITH_AES_128_CBC_SHA        = 0x002F,
    TLS_RSA_WITH_AES_256_CBC_SHA        = 0x0035,
    TLS_RSA_WITH_AES_128_CBC_SHA256     = 0x003C,
    TLS_RSA_WITH_AES_256_CBC_SHA256     = 0x003D,

    // Diffie-Hellman
    TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA    = 0x000D,
    TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA    = 0x0010,
    TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA   = 0x0013,
    TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA   = 0x0016,
    TLS_DH_DSS_WITH_AES_128_CBC_SHA     = 0x0030,
    TLS_DH_RSA_WITH_AES_128_CBC_SHA     = 0x0031,
    TLS_DHE_DSS_WITH_AES_128_CBC_SHA    = 0x0032,
    TLS_DHE_RSA_WITH_AES_128_CBC_SHA    = 0x0033,
    TLS_DH_DSS_WITH_AES_256_CBC_SHA     = 0x0036,
    TLS_DH_RSA_WITH_AES_256_CBC_SHA     = 0x0037,
    TLS_DHE_DSS_WITH_AES_256_CBC_SHA    = 0x0038,
    TLS_DHE_RSA_WITH_AES_256_CBC_SHA    = 0x0039,
    TLS_DH_DSS_WITH_AES_128_CBC_SHA256  = 0x003E,
    TLS_DH_RSA_WITH_AES_128_CBC_SHA256  = 0x003F,
    TLS_DHE_DSS_WITH_AES_128_CBC_SHA256 = 0x0040,
    TLS_DHE_RSA_WITH_AES_128_CBC_SHA256 = 0x0067,
    TLS_DH_DSS_WITH_AES_256_CBC_SHA256  = 0x0068,
    TLS_DH_RSA_WITH_AES_256_CBC_SHA256  = 0x0069,
    TLS_DHE_DSS_WITH_AES_256_CBC_SHA256 = 0x006A,
    TLS_DHE_RSA_WITH_AES_256_CBC_SHA256 = 0x006B,

    // anonymous Diffie-Hellman
    TLS_DH_anon_WITH_RC4_128_MD5        = 0x0018,
    TLS_DH_anon_WITH_3DES_EDE_CBC_SHA   = 0x001B,
    TLS_DH_anon_WITH_AES_128_CBC_SHA    = 0x0034,
    TLS_DH_anon_WITH_AES_256_CBC_SHA    = 0x003A,
    TLS_DH_anon_WITH_AES_128_CBC_SHA256 = 0x006C,
    TLS_DH_anon_WITH_AES_256_CBC_SHA256 = 0x006D
} TLS_CipherSuite_t;

//=============================================================================
typedef enum {
	TLS_CIPHER_NULL = 0,
	TLS_CIPHER_3DES_EDE_CBC,
	TLS_CIPHER_RC4_128,
	TLS_CIPHER_AES_128_CBC,
	TLS_CIPHER_AES_256_CBC
} TLS_CipherAlgorithm_t;

//=============================================================================
typedef enum {
	TLS_COMPRESS_NULL = 0
} TLS_Compression;

//=============================================================================
typedef enum {
	TLS_MAC_NULL = 0,
	TLS_MAC_MD5,
	TLS_MAC_SHA,
	TLS_MAC_SHA256
} TLS_MacAlgorithm_t;

//=============================================================================
typedef struct {
	TLS_CipherAlgorithm_t cipher;
	// TLS_Compression compress; // not used
	TLS_MacAlgorithm_t mac;
} TLS_CipherSpec_t;

//=============================================================================
typedef struct {
	uint32_t high;
	uint32_t low;
} TLS_SeqNum_t;

//=============================================================================
typedef enum {
	TLS_MSG_CHANGE_CIPHER_SPEC = 20,
	TLS_MSG_ALERT = 21,
	TLS_MSG_HANDSHAKE = 22,
	TLS_MSG_APPLICATION_DATA = 23,

	TLS_MSG_ERROR = 255
} TLS_MsgType_t;

//=============================================================================
typedef struct {
	TLS_MsgType_t type;
	TLS_Version_t ver;
} TLS_RecordHeader_t;

//=============================================================================
typedef enum {
	TLS_HANDSHAKE_HELLO_REQUEST = 0x00,
    TLS_HANDSHAKE_CLIENT_HELLO = 0x01,
    TLS_HANDSHAKE_SERVER_HELLO = 0x02,
    TLS_HANDSHAKE_CERTIFICATE = 0x0b,
    TLS_HANDSHAKE_SERVER_KEY_EXCHANGE = 0x0c,
    TLS_HANDSHAKE_CERTIFICATE_REQUEST = 0x0d,
    TLS_HANDSHAKE_SERVER_HELLO_DONE = 0x0e,
    TLS_HANDSHAKE_CERTIFICATE_VERIFY = 0x0f,
    TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE = 0x10,
    TLS_HANDSHAKE_FINISHED = 0x14
} TLS_HandshakeType_t;

//=============================================================================
typedef struct {
	uint8_t client_random[TLS_CLIENT_RANDOM_LEN];
	uint8_t server_random[TLS_SERVER_RANDOM_LEN];
	Hash_t msg_md5;
	Hash_t msg_sha1;
	uint8_t *public_key; // alloc
} TLS_Handshake_t;

//=============================================================================
typedef struct {
	uint32_t session_id_len;
	uint8_t session_id[TLS_SESSION_ID_LEN];
	uint8_t master_secret[TLS_MASTER_SECRET_LEN];
} TLS_SessionInfo_t;

//=============================================================================
typedef struct {
	TLS_Version_t ver;

	TLS_CipherSpec_t send_cipher;
	TLS_CipherSpec_t recv_cipher;
	TLS_CipherSpec_t pending_cipher;

	// for record protocol using crypto
	uint32_t hash_size;
	uint32_t key_len;
	uint32_t block_size;

	TLS_SeqNum_t send_seq_num;
	TLS_SeqNum_t recv_seq_num;

	uint8_t *key_block; // alloc

	uint8_t *send_mac_secret; // pointer from key_block
	uint8_t *recv_mac_secret; // pointer from key_block

	uint8_t *send_write_key; // pointer from key_block
	uint8_t *recv_write_key; // pointer from key_block
	uint8_t *send_iv; // pointer from key_block
	uint8_t *recv_iv; // pointer from key_block

	void *send_cipher_algo; // alloc, AES_t or DES_t
	void *recv_cipher_algo; // alloc, AES_t or DES_t

	// for Handshake Protocol
	TLS_Handshake_t *handshake; // alloc

	uint32_t num_cipher_suites;
	TLS_CipherSuite_t *cipher_suites; // alloc

	TLS_CipherSuite_t sel_cipher_suite;

	TLS_SessionInfo_t session_info;

	// uint8_t *master_secret; // alloc
	// uint32_t session_id_len;
	// uint8_t session_id[TLS_SESSION_ID_LEN];

	// hostname
	uint32_t ipaddr;
	uint32_t portno;

	uint32_t flag;

	// connection
	tls_socket_t sock;
} TLS_Handle_t;

//=============================================================================
TLS_Handle_t *TLS_CreateHandle(TLS_Version_t ver);
void TLS_FreeHandle(TLS_Handle_t *handle);

bool TLS_SetHostname(TLS_Handle_t *handle, uint32_t ipaddr, uint32_t portno);
bool TLS_SetCipherSuites(TLS_Handle_t *handle,
						 const TLS_CipherSuite_t *list, uint32_t num_items);
bool TLS_SetSessionInfo(TLS_Handle_t *handle, TLS_SessionInfo_t *info);
bool TLS_GetSessionInfo(TLS_Handle_t *handle, TLS_SessionInfo_t *info);

bool TLS_Connect(TLS_Handle_t *handle);
uint32_t TLS_Send(TLS_Handle_t *handle, uint8_t *buf, uint32_t len);
uint32_t TLS_Recv(TLS_Handle_t *handle, uint8_t *buf, uint32_t len);
void TLS_Shutdown(TLS_Handle_t *handle);


uint32_t TLS_GetSignedCertificate(const uint8_t *der_buf, uint8_t **ptr);
uint32_t TLS_GetEncryptedSignature(const uint8_t *der_buf, uint8_t **ptr);
uint32_t TLS_GetPublicKey(const uint8_t *der_buf, uint8_t **ptr);
uint32_t TLS_GetModulus(const uint8_t *public_key, uint8_t **ptr);
uint32_t TLS_GetExponent(const uint8_t *public_key, uint8_t **ptr);

bool TLS_SockOpen(TLS_Handle_t *handle);
void TLS_SockClose(TLS_Handle_t *handle);
bool TLS_SetSecrets(TLS_Handle_t *handle, uint8_t *master_secret,
					uint8_t *server_random, uint8_t *client_random);
bool TLS_SendChangeCipherSpec(TLS_Handle_t *handle);
bool TLS_RecvChangeCipherSpec(TLS_Handle_t *handle);
bool TLS_SendMsg(TLS_Handle_t *handle, TLS_MsgType_t type, TLS_Version_t ver,
				 uint8_t *fragment, uint32_t len);
bool TLS_RecvMsg(TLS_Handle_t *handle, TLS_RecordHeader_t *header,
				 uint8_t **fragment, uint32_t *len);

bool TLS_SetupHandshake(TLS_Handle_t *handle);
bool TLS_CleanupHandshake(TLS_Handle_t *handle);
bool TLS_SendClientHello(TLS_Handle_t *handle);
bool TLS_RecvServerHello(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len);
bool TLS_RecvCertificate(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len);
bool TLS_RecvServerHelloDone(TLS_Handle_t *handle,
							 uint8_t *msg, uint32_t m_len);
bool TLS_RecvServerKeyExchange(TLS_Handle_t *handle,
							   uint8_t *msg, uint32_t m_len);
bool TLS_RecvCertificateRequest(TLS_Handle_t *handle,
								uint8_t *msg, uint32_t m_len);
bool TLS_SendCertificate(TLS_Handle_t *handle);
bool TLS_SendClientKeyExchange(TLS_Handle_t *handle);
bool TLS_SendCertificateVerify(TLS_Handle_t *handle);
bool TLS_SendFinished(TLS_Handle_t *handle);
bool TLS_RecvFinished(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len);

#endif /* !_TLS_H */

/******************************** END-OF-FILE ********************************/
