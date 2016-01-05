/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "tls.h"
#include "tls_prv.h"

#define TLS_BLOCK_SIZE 16 // max(aes block size, des3 block size)
#define TLS_MAX_FRAGMENT_SIZE (16384)

//=============================================================================
#define TLS_resetSeqNum(seq_num) do { \
		(seq_num).high = 0;			  \
		(seq_num).low = 0;			  \
	} while (0)

//=============================================================================
static void TLS_incSeqNum(TLS_SeqNum_t *seq_num)
{
	if (seq_num == NULL) return;

	seq_num->low ++;
	if (seq_num->low == 0) {
		seq_num->high ++;
	}
}

//=============================================================================
static void TLS_setSeqNum(TLS_SeqNum_t *seq_num, uint8_t *out)
{
	if (seq_num == NULL || out == NULL) return;

	*out ++ = (seq_num->high >> 24) & 0xff;
	*out ++ = (seq_num->high >> 16) & 0xff;
	*out ++ = (seq_num->high >> 8) & 0xff;
	*out ++ = seq_num->high & 0xff;
	*out ++ = (seq_num->low >> 24) & 0xff;
	*out ++ = (seq_num->low >> 16) & 0xff;
	*out ++ = (seq_num->low >> 8) & 0xff;
	*out = seq_num->low & 0xff;
}

//=============================================================================
bool TLS_SockOpen(TLS_Handle_t *handle)
{
	struct sockaddr_in addr;
	TLS_CipherSpec_t cipher_spec;

	if (handle == NULL) return (FALSE);

	if (!isInvalidSocket(handle->sock)) {
		// Already Opened
		return (FALSE);
	}

	// ----------------------------------- Initialize Connection Parameters ---
	cipher_spec.cipher = TLS_CIPHER_NULL;
	// cipher_spec.compress = TLS_COMPRESS_NULL;
	cipher_spec.mac = TLS_MAC_NULL;

	handle->send_cipher = cipher_spec;
	handle->recv_cipher = cipher_spec;
	handle->pending_cipher = cipher_spec;

	TLS_resetSeqNum(handle->send_seq_num);
	TLS_resetSeqNum(handle->recv_seq_num);

	if (handle->send_cipher_algo != NULL) {
		TLS_free(handle->send_cipher_algo);
		handle->send_cipher_algo = NULL;
	}
	if (handle->recv_cipher_algo != NULL) {
		TLS_free(handle->recv_cipher_algo);
		handle->recv_cipher_algo = NULL;
	}

	if (handle->send_mac_secret != NULL) {
		handle->send_mac_secret = NULL;
	}
	if (handle->recv_mac_secret != NULL) {
		handle->recv_mac_secret = NULL;
	}

    // -------------------------------------------------------- Open Socket ---
	handle->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (isInvalidSocket(handle->sock)) {
		err_printf("[TLS] sock open error\n");
		return (FALSE);
	}

    // ----------------------------------------------------- Connect Server ---
    TLS_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = handle->ipaddr;
    addr.sin_port = SOCK_PORTNO(handle->portno);

	if (SOCK_IsError(connect(handle->sock,
							 (struct sockaddr *)&addr, sizeof(addr)))) {
		SOCK_Close(handle->sock);
		handle->sock = SOCK_INVALID;
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
void TLS_SockClose(TLS_Handle_t *handle)
{
	if (handle == NULL) return;

	if (!isInvalidSocket(handle->sock)) {
		SOCK_Close(handle->sock);
		handle->sock = SOCK_INVALID;
	}
}

//=============================================================================
static bool TLS_SelectCipherSuite(TLS_Handle_t *handle,
								  TLS_CipherSuite_t cipher_suite)
{
	if (handle == NULL) return (FALSE);

	switch (cipher_suite) {
	case TLS_NULL_WITH_NULL_NULL:
		handle->pending_cipher.cipher = TLS_CIPHER_NULL;
		handle->pending_cipher.mac = TLS_MAC_NULL;
		break;

	case TLS_RSA_WITH_NULL_MD5:
	case TLS_RSA_WITH_NULL_SHA:
	case TLS_RSA_WITH_NULL_SHA256:
		/*** not implemented ***/
		return (FALSE);

	case TLS_RSA_WITH_RC4_128_MD5:
	case TLS_RSA_WITH_RC4_128_SHA:
		/*** not implemented ***/
		return (FALSE);

	case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
		handle->pending_cipher.cipher = TLS_CIPHER_3DES_EDE_CBC;
		handle->pending_cipher.mac = TLS_MAC_SHA;
		break;

	case TLS_RSA_WITH_AES_128_CBC_SHA:
		handle->pending_cipher.cipher = TLS_CIPHER_AES_128_CBC;
		handle->pending_cipher.mac = TLS_MAC_SHA;
		break;

	case TLS_RSA_WITH_AES_256_CBC_SHA:
		handle->pending_cipher.cipher = TLS_CIPHER_AES_256_CBC;
		handle->pending_cipher.mac = TLS_MAC_SHA;
		break;

	case TLS_RSA_WITH_AES_128_CBC_SHA256:
		handle->pending_cipher.cipher = TLS_CIPHER_AES_128_CBC;
		handle->pending_cipher.mac = TLS_MAC_SHA256;
		break;

	case TLS_RSA_WITH_AES_256_CBC_SHA256:
		handle->pending_cipher.cipher = TLS_CIPHER_AES_256_CBC;
		handle->pending_cipher.mac = TLS_MAC_SHA256;
		break;

	case TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA:
	case TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA:
	case TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA:
	case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
	case TLS_DH_DSS_WITH_AES_128_CBC_SHA:
	case TLS_DH_RSA_WITH_AES_128_CBC_SHA:
	case TLS_DHE_DSS_WITH_AES_128_CBC_SHA:
	case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
	case TLS_DH_DSS_WITH_AES_256_CBC_SHA:
	case TLS_DH_RSA_WITH_AES_256_CBC_SHA:
	case TLS_DHE_DSS_WITH_AES_256_CBC_SHA:
	case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
    case TLS_DH_DSS_WITH_AES_128_CBC_SHA256:
	case TLS_DH_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_DHE_DSS_WITH_AES_128_CBC_SHA256:
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_DH_DSS_WITH_AES_256_CBC_SHA256:
    case TLS_DH_RSA_WITH_AES_256_CBC_SHA256:
    case TLS_DHE_DSS_WITH_AES_256_CBC_SHA256:
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
		/*** not implemented ***/
		return (FALSE);

    case TLS_DH_anon_WITH_RC4_128_MD5:
    case TLS_DH_anon_WITH_3DES_EDE_CBC_SHA:
    case TLS_DH_anon_WITH_AES_128_CBC_SHA:
    case TLS_DH_anon_WITH_AES_256_CBC_SHA:
    case TLS_DH_anon_WITH_AES_128_CBC_SHA256:
    case TLS_DH_anon_WITH_AES_256_CBC_SHA256:
		/*** not implemented ***/
		return (FALSE);
	}

	switch (handle->pending_cipher.cipher) {
	case TLS_CIPHER_3DES_EDE_CBC:
		handle->key_len = DES3_KEY_SIZE;
		handle->block_size = DES_BLOCK_SIZE;
		break;
	case TLS_CIPHER_AES_128_CBC:
		handle->key_len = AES128_KEY_SIZE;
		handle->block_size = AES_BLOCK_SIZE;
		break;
	case TLS_CIPHER_AES_256_CBC:
		handle->key_len = AES256_KEY_SIZE;
		handle->block_size = AES_BLOCK_SIZE;
		break;
	default:
		handle->key_len = 0;
		handle->block_size = 0;
		break;
	}

	switch (handle->pending_cipher.mac) {
	case TLS_MAC_MD5:
		handle->hash_size = MD5_BYTES_SIZE;
		break;
	case TLS_MAC_SHA:
		handle->hash_size = SHA1_BYTES_SIZE;
		break;
	case TLS_MAC_SHA256:
		handle->hash_size = SHA256_BYTES_SIZE;
		break;
	default:
		handle->hash_size = 0;
		break;
	}

	return (TRUE);
}

//=============================================================================
bool TLS_SetSecrets(TLS_Handle_t *handle, uint8_t *master_secret,
					uint8_t *server_random, uint8_t *client_random)
{
	uint8_t *label_seed;
	uint32_t ls_len;
	const char label[] = "key expansion";
	uint32_t label_len;
	uint8_t *key_block;
	uint32_t key_block_len;

	if (handle == NULL) return (FALSE);

	// ---------------------------------------- Allocate Label & Seed Space ---
	label_len = TLS_strlen(label);
	ls_len = label_len + TLS_SERVER_RANDOM_LEN + TLS_CLIENT_RANDOM_LEN;
	label_seed = TLS_malloc(ls_len);
	if (label_seed == NULL) {
		return (FALSE);
	}

	TLS_memcpy(label_seed, label, label_len);
	TLS_memcpy(label_seed + label_len, server_random, TLS_SERVER_RANDOM_LEN);
	TLS_memcpy(label_seed + label_len + TLS_SERVER_RANDOM_LEN,
			   client_random, TLS_CLIENT_RANDOM_LEN);

	key_block_len = handle->hash_size + handle->key_len + handle->block_size;
	key_block_len *= 2;
	key_block = TLS_malloc(key_block_len);
	if (key_block == NULL) {
		TLS_free(label_seed);
		return (FALSE);
	}

	// --------------------------------------------------- PRF => Key Block ---
	if (!PRF_calc(master_secret, TLS_MASTER_SECRET_LEN, label_seed, ls_len,
				  key_block, key_block_len)) {
		TLS_free(key_block);
		TLS_free(label_seed);
		return (FALSE);
	}

#if 1
	int cnt;
	printf("key_block:\n");
	for (cnt = 0; cnt < key_block_len; cnt ++) {
		printf("%02x ", key_block[cnt]);
	}
	printf("\n");
#endif

	handle->key_block = key_block;

	handle->send_mac_secret = key_block; // Client MAC Secret
	key_block += handle->hash_size;
	handle->recv_mac_secret = key_block; // Server MAC Secret
	key_block += handle->hash_size;

	handle->send_write_key = key_block; // Client Write Key
	key_block += handle->key_len;
	handle->recv_write_key = key_block; // Server Write Key
	key_block += handle->key_len;

	if (handle->block_size > 0) {
		handle->send_iv = key_block; // Client IV
		key_block += handle->block_size;
		handle->recv_iv = key_block; // Server IV
		key_block += handle->block_size;
	} else {
		handle->send_iv = NULL;
		handle->recv_iv = NULL;
	}

	assert((handle->key_block + key_block_len) == key_block);

	switch (handle->pending_cipher.cipher) {
	case TLS_CIPHER_NULL:
		break;
	case TLS_CIPHER_3DES_EDE_CBC:
		handle->send_cipher_algo = TLS_malloc(sizeof(DES_t));
		TLS_assert(handle->send_cipher_algo != NULL);
		DES_Init(handle->send_cipher_algo,
				 DES_3, handle->send_write_key, handle->send_iv);

		handle->recv_cipher_algo = TLS_malloc(sizeof(DES_t));
		TLS_assert(handle->recv_cipher_algo != NULL);
		DES_Init(handle->recv_cipher_algo,
				 DES_3, handle->recv_write_key, handle->recv_iv);
		break;
	case TLS_CIPHER_RC4_128:
		/*** not implemented ***/
		break;
	case TLS_CIPHER_AES_128_CBC:
		handle->send_cipher_algo = TLS_malloc(sizeof(AES_t));
		TLS_assert(handle->send_cipher_algo != NULL);
		AES_Init(handle->send_cipher_algo,
				 AES_128, handle->send_write_key, handle->send_iv);
		handle->recv_cipher_algo = TLS_malloc(sizeof(AES_t));
		TLS_assert(handle->recv_cipher_algo != NULL);
		AES_Init(handle->recv_cipher_algo,
				 AES_128, handle->recv_write_key, handle->recv_iv);
		break;
	case TLS_CIPHER_AES_256_CBC:
		handle->send_cipher_algo = TLS_malloc(sizeof(AES_t));
		TLS_assert(handle->send_cipher_algo != NULL);
		AES_Init(handle->send_cipher_algo,
				 AES_256, handle->send_write_key, handle->send_iv);
		handle->recv_cipher_algo = TLS_malloc(sizeof(AES_t));
		TLS_assert(handle->recv_cipher_algo != NULL);
		AES_Init(handle->recv_cipher_algo,
				 AES_256, handle->recv_write_key, handle->recv_iv);
		break;
	default:
		break;
	}

	TLS_free(label_seed);

	return (TRUE);
}

//=============================================================================
bool TLS_SendChangeCipherSpec(TLS_Handle_t *handle)
{
	uint8_t msg[] = { 0x01 };

	if (handle == NULL) return (FALSE);

	if (!TLS_SendMsg(handle,
					 TLS_MSG_CHANGE_CIPHER_SPEC, handle->ver, msg, 1)) {
		return (FALSE);
	}

	// ------------------------------------------------- Change Cipher Spec ---
	if ((handle->flag & TLS_CHANGE_CIPHER_SPEC) == 0) {
		TLS_SelectCipherSuite(handle, handle->sel_cipher_suite);

		TLS_SetSecrets(handle, handle->session_info.master_secret,
					   handle->handshake->server_random,
					   handle->handshake->client_random);

		handle->flag |= TLS_CHANGE_CIPHER_SPEC;
	}

	// change send cipher
	handle->send_cipher = handle->pending_cipher;

	return (TRUE);
}

//=============================================================================
bool TLS_RecvChangeCipherSpec(TLS_Handle_t *handle)
{
	if (handle == NULL) return (FALSE);

	// ------------------------------------------------- Change Cipher Spec ---
	if ((handle->flag & TLS_CHANGE_CIPHER_SPEC) == 0) {
		TLS_SelectCipherSuite(handle, handle->sel_cipher_suite);

		TLS_SetSecrets(handle, handle->session_info.master_secret,
					   handle->handshake->server_random,
					   handle->handshake->client_random);

		handle->flag |= TLS_CHANGE_CIPHER_SPEC;
	}

	// change recv cipher
	handle->recv_cipher = handle->pending_cipher;

	return (TRUE);
}

//=============================================================================
static void TLS_setHeader(TLS_RecordHeader_t *header, uint32_t len,
						  uint8_t *buf)
{
	buf[0] = header->type;
	buf[1] = (header->ver >> 8) & 0xff;
	buf[2] = header->ver & 0xff;
	buf[3] = (len >> 8) & 0xff;
	buf[4] = len & 0xff;
}

//=============================================================================
static bool TLS_getMAC(TLS_MacAlgorithm_t mac_type,
					   uint8_t *mac_secret, uint32_t ms_len,
					   TLS_SeqNum_t *seq_num, TLS_RecordHeader_t *header,
					   uint8_t *fragment, uint32_t len, uint8_t *mac)
{
	uint8_t msg_header[8 + 5];
	HashType_t hash_type;

	if (mac_secret == NULL || ms_len == 0 || header == NULL
		|| fragment == NULL /* || len == 0 */ || mac == NULL) return (FALSE);

	switch (mac_type) {
	case TLS_MAC_MD5:
		hash_type = HASH_MD5;
		break;
	case TLS_MAC_SHA:
		hash_type = HASH_SHA1;
		break;
	case TLS_MAC_SHA256:
		hash_type = HASH_SHA256;
		break;
	default:
		return (FALSE);
	}

	// put seq_num[8 bytes] and header[5 bytes]
	TLS_setSeqNum(seq_num, &msg_header[0]);
	TLS_setHeader(header, len, &msg_header[8]);

#if 0
	{
		int cnt;

		printf("before HMAC:\n");
		for (cnt = 0; cnt < (8 + 5); cnt ++) {
			printf("%02x ", msg_header[cnt]);
		}
		for (cnt = 0; cnt < len; cnt ++) {
			printf("%02x ", fragment[cnt]);
		}
		printf("\n");
	}
#endif

	return (HMAC_hash(hash_type, mac_secret, ms_len,
					  msg_header, 8 + 5, fragment, len, mac));
}

//=============================================================================
// Create TLSCompressed
//=============================================================================
static uint32_t TLS_createTLSCompressed(TLS_RecordHeader_t *header,
										uint8_t *fragment, uint32_t len,
										uint8_t **created)
{
	uint8_t *record;
	uint32_t record_len;

	if (header == NULL || fragment == NULL || len == 0) return (0);
	if (created == NULL) return (0);
	if (len > TLS_MAX_FRAGMENT_SIZE) return (0);

	record_len = len + 5;

	record = TLS_malloc(record_len);
	if (record == NULL) return (0);

	TLS_setHeader(header, len, record);

	TLS_memcpy(&record[5], fragment, len);

	*created = record;

	return (record_len);
}

//=============================================================================
// Create TLSCiphertext
//=============================================================================
static uint32_t TLS_createTLSCiphertext(TLS_Handle_t *handle,
										TLS_RecordHeader_t *header,
										uint8_t *fragment, uint32_t len,
										uint8_t **created)
{
	uint8_t *record;
	uint32_t m_len;
	uint32_t record_len;
	uint32_t pad_size;
	uint8_t *work;
	uint32_t cnt;

	// --------------------------------------------------- Check Parameters ---
	if (header == NULL || fragment == NULL || len == 0) return (0);
	if (created == NULL) return (0);
	if (len > TLS_MAX_FRAGMENT_SIZE) return (0);

	// ------------------------------------------ Calc TLSCiphertext Length ---
	m_len = len + handle->hash_size; // add MAC size
	if (handle->block_size > 0) {
		// add padding size
		pad_size = handle->block_size - (m_len % handle->block_size);
		m_len += pad_size;
	}

	// --------------------------------------------- Allocate Message Space ---
	record_len = m_len + 5;
	record = TLS_malloc(record_len);
	if (record == NULL) return (0);

	// ------------------------------------------------------ Copy Fragment ---
	TLS_memcpy(&record[5], fragment, len);

	// ------------------------------------------------------------ Add MAC ---
	TLS_getMAC(handle->send_cipher.mac,
			   handle->send_mac_secret, handle->hash_size,
			   &handle->send_seq_num, header, fragment, len, &record[5 + len]);
	TLS_incSeqNum(&handle->send_seq_num);

	// -------------------------------------------------------- Add Padding ---
	work = &record[5 + len + handle->hash_size];
	for (cnt = 0; cnt < pad_size; cnt ++) {
		*work ++ = pad_size - 1;
	}

	// --------------------------------------------------------- Encryption ---
	switch (handle->send_cipher.cipher) {
	case TLS_CIPHER_3DES_EDE_CBC:
		DES_Encrypt(handle->send_cipher_algo, &record[5], &record[5], m_len);
		break;
	case TLS_CIPHER_RC4_128:
		/*** not implemented ***/
		break;
	case TLS_CIPHER_AES_128_CBC:
	case TLS_CIPHER_AES_256_CBC:
		AES_Encrypt(handle->send_cipher_algo, &record[5], &record[5], m_len);
		break;
	default:
		/*** ERROR ***/
		break;
	}

	TLS_setHeader(header, m_len, record);

	*created = record;

	return (record_len);
}

//=============================================================================
// Send Message (at Record Protocol Layer)
//=============================================================================
bool TLS_SendMsg(TLS_Handle_t *handle, TLS_MsgType_t type, TLS_Version_t ver,
				 uint8_t *fragment, uint32_t len)
{
	uint8_t *send_msg = NULL;
	TLS_RecordHeader_t header;
	uint32_t send_len;

	if (handle == NULL || fragment == NULL || len == 0) return (FALSE);

	header.type = type;
	header.ver = ver;

	// -------------------------------------------- Create Sendning Message ---
	if (handle->send_cipher.cipher == TLS_CIPHER_NULL) {
		send_len = TLS_createTLSCompressed(&header, fragment, len, &send_msg);
	} else {
#if 0
		int cnt;
		printf("before ciphertext:\n");
		for (cnt = 0; cnt < len; cnt ++) {
			printf("%02x ", fragment[cnt]);
		}
		printf("\n");
#endif
		send_len = TLS_createTLSCiphertext(handle, &header,
										   fragment, len, &send_msg);
	}
	if (send_len == 0) {
		return (FALSE);
	}

	// ------------------------------------------------------- Send Message ---
	send_len = send(handle->sock, send_msg, send_len, 0);
	TLS_free(send_msg);

	if (SOCK_SendError(send_len)) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
static bool TLS_recvHeader(TLS_Handle_t *handle,
						   TLS_RecordHeader_t *header, uint32_t *m_len)
{
	uint8_t recv_buf[5];
	uint32_t recv_len;

	if (handle == NULL || header == NULL || m_len == NULL) return (FALSE);

	recv_len = recv(handle->sock, recv_buf, 5, 0);
	if (SOCK_RecvError(recv_len)) {
		return (FALSE);
	}

	if (recv_len < 5) {
		// received header is short
		return (FALSE);
	}

	header->type = (TLS_MsgType_t)recv_buf[0];
	header->ver = (TLS_Version_t)(((uint16_t)recv_buf[1] << 8) | (uint16_t)recv_buf[2]);
	*m_len = ((uint32_t)recv_buf[3] << 8) | (uint32_t)recv_buf[4];

	return (TRUE);
}

//=============================================================================
// Check Fragment's MAC
//   MAC follows fragment
//=============================================================================
static bool TLS_checkMAC(TLS_Handle_t *handle, TLS_RecordHeader_t *header,
						 uint8_t *fragment, uint32_t len)
{
	uint8_t *work;
	int result;

	if (handle == NULL || header == NULL
		|| fragment == NULL /* || len == 0 */) return (FALSE);

	// --------------------------- Allocate Memory for SeqNum + Header, MAC ---
	work = TLS_malloc(handle->hash_size);

	TLS_getMAC(handle->recv_cipher.mac,
			   handle->recv_mac_secret, handle->hash_size,
			   &handle->recv_seq_num, header, fragment, len, work);

	result = TLS_memcmp(work, fragment + len, handle->hash_size);

	TLS_free(work);

	TLS_incSeqNum(&handle->recv_seq_num);

	return ((result == 0));
}


//=============================================================================
// Receive Message (at Record Protocol Layer)
//=============================================================================
bool TLS_RecvMsg(TLS_Handle_t *handle, TLS_RecordHeader_t *header,
				 uint8_t **fragment, uint32_t *len)
{
	uint32_t m_len;
	uint8_t *recv_buf;
	uint32_t recv_len, recv_total_len;

	if (handle == NULL || header == NULL || fragment == NULL || len == NULL) {
		return (FALSE);
	}

	// ----------------------------------------------------- Receive Header ---
	if (!TLS_recvHeader(handle, header, &m_len)) {
		return (FALSE);
	}

	if (m_len == 0 || m_len > TLS_MAX_FRAGMENT_SIZE) {
		return (FALSE);
	}

	recv_buf = TLS_malloc(m_len);
	if (recv_buf == NULL) {
		// ------------------------------------------------ Discard Message ---
		recv_buf = TLS_malloc(256); // alloc small work space
		if (recv_buf == NULL) {
			/*** no memory ***/
			return (FALSE);
		}
		recv_total_len = 0;
		while (recv_total_len < m_len) {
			recv_len = recv(handle->sock, recv_buf, 256, 0);
			if (SOCK_RecvError(recv_len)) {
				/*** ERROR ***/
				TLS_free(recv_buf);
				return (FALSE);
			}
			recv_total_len += recv_len;
		}
		TLS_free(recv_buf);
		return (FALSE);
	}

	// ---------------------------------------------------- Receive Message ---
	recv_total_len = 0;
	while (recv_total_len < m_len) {
		recv_len = recv(handle->sock, &recv_buf[recv_total_len],
						m_len - recv_total_len, 0);
		if (SOCK_RecvError(recv_len)) {
			/*** ERROR ***/
			TLS_free(recv_buf);
			return (FALSE);
		}
		recv_total_len += recv_len;
	}

	// ---------------------------------------------------- Decrypt Message ---
	switch (handle->recv_cipher.cipher) {
	case TLS_CIPHER_NULL:
		break;
	case TLS_CIPHER_3DES_EDE_CBC:
		DES_Decrypt(handle->recv_cipher_algo, recv_buf, recv_buf, m_len);
		break;
	case TLS_CIPHER_RC4_128:
		/*** not implemented ***/
		TLS_free(recv_buf);
		return (FALSE);
		break;
	case TLS_CIPHER_AES_128_CBC:
	case TLS_CIPHER_AES_256_CBC:
		AES_Decrypt(handle->recv_cipher_algo, recv_buf, recv_buf, m_len);
		break;
	default:
		/*** ERROR ***/
		TLS_free(recv_buf);
		return (FALSE);
		break;
	}

	// ---------------------------------------------- Strip Padding and MAC ---
	if (handle->recv_cipher.cipher != TLS_CIPHER_NULL) {
		// -------------------------------------------------- Strip Padding ---
		if (handle->block_size > 0) {
			uint32_t pad_size;
			pad_size = recv_buf[m_len - 1];
			pad_size ++;
			if (pad_size > handle->block_size || pad_size >= m_len) {
				/*** ERROR ***/
				TLS_free(recv_buf);
				return (FALSE);
			}
			m_len -= pad_size;
		}
		// ------------------------------------------------------ Strip MAC ---
		if (m_len < handle->hash_size) {
			TLS_free(recv_buf);
			return (FALSE);
		}
		m_len -= handle->hash_size;

		if (!TLS_checkMAC(handle, header, recv_buf, m_len)) {
			TLS_free(recv_buf);
			return (FALSE);
		}
	}

	*fragment = recv_buf;
	*len = m_len;

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
