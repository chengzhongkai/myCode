/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "platform.h"

#include "tls.h"
#include "tls_prv.h"

//=============================================================================
typedef enum {
	TLS_KEYEX_NULL,
	TLS_KEYEX_RSA,
	TLS_KEYEX_DHE,
	TLS_KEYEX_DH,
	TLS_KEYEX_DH_anon
} TLS_KeyExType_t;

//=============================================================================
bool TLS_SetupHandshake(TLS_Handle_t *handle)
{
	if (handle == NULL) return (FALSE);

	// ---------------------------------------------------- Allocate Memory ---
	handle->handshake = TLS_malloc(sizeof(TLS_Handshake_t));
	if (handle->handshake == NULL) {
		return (FALSE);
	}
	TLS_memset(handle->handshake, 0, sizeof(TLS_Handshake_t));

	// --------------------------------------- Initialize Hash MD5 and SHA1 ---
	if (!Hash_Init(&handle->handshake->msg_md5, HASH_MD5)) {
		TLS_free(handle->handshake);
		handle->handshake = NULL;
		return (FALSE);
	}
	if (!Hash_Init(&handle->handshake->msg_sha1, HASH_SHA1)) {
		TLS_free(handle->handshake);
		handle->handshake = NULL;
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
bool TLS_CleanupHandshake(TLS_Handle_t *handle)
{
	if (handle == NULL) return (FALSE);

	if (handle->handshake != NULL) {
		if (handle->handshake->public_key != NULL) {
			TLS_free(handle->handshake->public_key);
		}
		TLS_free(handle->handshake);
		handle->handshake = NULL;
	}

	return (TRUE);
}

//=============================================================================
static void TLS_getRandomBytes(uint8_t *buf, uint32_t len)
{
	uint32_t rand_data;
	int cnt;

	if (buf == NULL || len == 0) return;

	while (len > 0) {
		rand_data = FSL_GetRandom();
		for (cnt = 0; cnt < 4 && len > 0; cnt ++) {
			*buf = rand_data & 0xff;
			buf ++;
			len --;
			rand_data >>= 8;
		}
	}
}

//=============================================================================
static void TLS_getRandomBytesNoZero(uint8_t *buf, uint32_t len)
{
	uint32_t rand_data;
	int cnt;

	if (buf == NULL || len == 0) return;

	while (len > 0) {
		rand_data = FSL_GetRandom();
		for (cnt = 0; cnt < 4 && len > 0; cnt ++) {
			if ((rand_data & 0xff) != 0) {
				*buf = rand_data & 0xff;
				buf ++;
				len --;
			}
			rand_data >>= 8;
		}
	}
}

//=============================================================================
static void TLS_setClientRandom(uint8_t *crandom)
{
	// GMT Time (4 bytes)
	TLS_getRandomBytes(crandom, 4);

	// Random Bytes (28 bytes)
	TLS_getRandomBytes(crandom + 4, TLS_CLIENT_RANDOM_LEN - 4);
}

//=============================================================================
// Send ClientHello
//=============================================================================
bool TLS_SendClientHello(TLS_Handle_t *handle)
{
	uint8_t *msg;
	uint32_t m_len;
	uint32_t idx;
	int cnt;
	bool result;

	if (handle == NULL) return (FALSE);

	// ------------------------------------------------- Make Client Random ---
	TLS_setClientRandom(handle->handshake->client_random);

#if 1
	printf("client random:\n");
	for (cnt = 0; cnt < TLS_CLIENT_RANDOM_LEN; cnt ++) {
		printf("%02x ", handle->handshake->client_random[cnt]);
	}
	printf("\n");
#endif

	// header(4), ver(2), client_random(32), session_id(1..33), 
	// cipher_suites((N+1)x2), compression(2)
	m_len = 4 + 2 + 32 + (handle->num_cipher_suites + 1) * 2 + 2;
	m_len += handle->session_info.session_id_len + 1;

	msg = TLS_malloc(m_len);
	if (msg == NULL) {
		return (FALSE);
	}

	// header(4) (type(1), len(3))
	msg[0] = TLS_HANDSHAKE_CLIENT_HELLO;
	TLS_setUint24(&msg[1], m_len - 4);

	// version(2)
	TLS_setUint16(&msg[4], handle->ver);

	// client_random(32)
	TLS_memcpy(&msg[6],
			   handle->handshake->client_random, TLS_CLIENT_RANDOM_LEN);
	idx = 6 + TLS_CLIENT_RANDOM_LEN;

	// session_id(1..33)
	msg[idx] = handle->session_info.session_id_len;
	idx ++;
	if (handle->session_info.session_id_len > 0) {
		TLS_memcpy(&msg[idx],
				   handle->session_info.session_id,
				   handle->session_info.session_id_len);
		idx += handle->session_info.session_id_len;
	}

	// cipher_suites((N+1)x2)
	TLS_setUint16(&msg[idx], handle->num_cipher_suites * 2);
	idx += 2;
	for (cnt = 0; cnt < handle->num_cipher_suites; cnt ++) {
		TLS_setUint16(&msg[idx], handle->cipher_suites[cnt]);
		idx += 2;
	}

	// compression_method(2)
	msg[idx] = 1;
	idx ++;
	msg[idx] = 0;
	idx ++;

	TLS_assert(idx == m_len);

	// ------------------------------------------------------- Send Message ---
	result = TLS_SendMsg(handle, TLS_MSG_HANDSHAKE, handle->ver, msg, m_len);
	if (result) {
		Hash_Update(&handle->handshake->msg_md5, msg, m_len);
		Hash_Update(&handle->handshake->msg_sha1, msg, m_len);
	}
	TLS_free(msg);

	return (result);
}

//=============================================================================
// Receive ServerHello
//=============================================================================
bool TLS_RecvServerHello(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	uint32_t c_len;

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_SERVER_HELLO) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	// version(2)
	// msg[4], msg[5] => ver

	// server_random(32)
	TLS_memcpy(handle->handshake->server_random,
			   &msg[6], TLS_SERVER_RANDOM_LEN);
	idx = 6 + TLS_SERVER_RANDOM_LEN;

#if 1
	int cnt;
	printf("server random:\n");
	for (cnt = 0; cnt < TLS_SERVER_RANDOM_LEN; cnt ++) {
		printf("%02x ", handle->handshake->server_random[cnt]);
	}
	printf("\n");
#endif

	// session_id(N), Compare Session ID
	if ((handle->flag & TLS_VALID_SESSION_ID) != 0
		&& msg[idx] == handle->session_info.session_id_len
		&& TLS_memcmp(&msg[idx + 1], handle->session_info.session_id,
					  handle->session_info.session_id_len) == 0) {
		idx ++;
		idx += handle->session_info.session_id_len;
	} else {
		handle->session_info.session_id_len = msg[idx];
		idx ++;
		if (handle->session_info.session_id_len > TLS_SESSION_ID_LEN) {
			return (FALSE);
		}
		if (handle->session_info.session_id_len > 0) {
			TLS_memcpy(handle->session_info.session_id,
					   &msg[idx], handle->session_info.session_id_len);
			idx += handle->session_info.session_id_len;
		}
		handle->flag |= TLS_VALID_SESSION_ID;
		handle->flag &= ~TLS_VALID_MASTER_SECRET;
	}

	// cipher_suite(2)
	handle->sel_cipher_suite = (TLS_CipherSuite_t)TLS_getUint16(&msg[idx]);
	idx += 2;

	// compression_method(1)
	// msg[idx]
	idx ++;

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

//=============================================================================
// RSA Encryption
//=============================================================================
static uint32_t TLS_encryptRSA(uint8_t *src, uint32_t src_len,
							   uint8_t *public_key, uint8_t **rsa)
{
	uint8_t *mod_buf;
	uint32_t mod_len;
	uint8_t *exp_buf;
	uint32_t exp_len;

	uint8_t *rsa_buf;
	uint32_t rsa_len;

	BigNum_t mod, exp, enc, dec;

	if (src == NULL || src_len == 0 || public_key == NULL) return (0);

	mod_len = TLS_GetModulus(public_key, &mod_buf);
	if (mod_len == 0) return (0);

	exp_len = TLS_GetExponent(public_key, &exp_buf);
	if (exp_len == 0) return (0);

	if (!BigNum_InitFromBuffer(&mod, mod_buf, mod_len)) return (0);
	rsa_len = BigNum_NumBytes(&mod);
	if (!BigNum_InitFromBuffer(&exp, exp_buf, exp_len)) {
		BigNum_Free(&mod);
		return (0);
	}
	if (!BigNum_InitFromBuffer(&enc, src, src_len)) {
		BigNum_Free(&mod);
		BigNum_Free(&exp);
		return (0);
	}

	if (!BigNum_PowMod(&dec, &enc, &exp, &mod)) {
		BigNum_Free(&mod);
		BigNum_Free(&exp);
		BigNum_Free(&enc);
	}

	BigNum_Free(&mod);
	BigNum_Free(&exp);
	BigNum_Free(&enc);

	rsa_buf = TLS_malloc(rsa_len);
	if (rsa_buf == NULL) {
		BigNum_Free(&dec);
		return (0);
	}

	BigNum_GetBytes(&dec, rsa_buf, rsa_len);

	BigNum_Free(&dec);

	*rsa = rsa_buf;

	return (rsa_len);
}

//=============================================================================
// Check Certificate Signature
//=============================================================================
static bool TLS_checkCertificateSignature(uint8_t *check_cert,
										  uint8_t *root_cert)
{
	uint8_t *public_key;
	uint32_t public_key_len;
	uint8_t *encrypted;
	uint32_t encrypted_len;
	uint8_t *cert;
	uint32_t cert_len;

	uint8_t *rsa;
	uint32_t rsa_len;

	Hash_t hash;
	uint8_t hash_digest[SHA1_BYTES_SIZE];

	bool check = FALSE;

	if (check_cert == NULL || root_cert == NULL) return (FALSE);

	public_key_len = TLS_GetPublicKey(root_cert, &public_key);
	if (public_key_len == 0) return (FALSE);

	encrypted_len = TLS_GetEncryptedSignature(check_cert, &encrypted);
	if (encrypted_len == 0) return (FALSE);

	cert_len = TLS_GetSignedCertificate(check_cert, &cert);
	if (cert_len == 0) return (FALSE);

	// decrypt RSA
	rsa_len = TLS_encryptRSA(encrypted, encrypted_len, public_key, &rsa);
	if (rsa_len == 0) return (FALSE);

	// calc hash
	if (Hash_Init(&hash, HASH_SHA1)) {
		if (Hash_Update(&hash, cert, cert_len)) {
			if (Hash_GetDigest(&hash, hash_digest)) {
				if (TLS_memcmp(hash_digest,
							   &rsa[rsa_len - SHA1_BYTES_SIZE],
							   SHA1_BYTES_SIZE) == 0) {
					check = TRUE;
				}
			}
		}
	}

	TLS_free(rsa);

	return (check);
}

//=============================================================================
// Receive Certificate
//=============================================================================
bool TLS_RecvCertificate(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	int32_t c_len, all_cert_len, cert_len;
	uint8_t *certificate;
	uint8_t *root_cert, *check_cert;
	bool sign_ok;

	uint8_t *public_key;
	uint32_t public_key_len;

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_CERTIFICATE) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	// all_cert_len(3)
	idx = 4;
	all_cert_len = TLS_getUint24(&msg[idx]);
	idx += 3;
	if (c_len != (all_cert_len + 3)) return (FALSE);

	// certificate (N)
	cert_len = TLS_getUint24(&msg[idx]);
	idx += 3;
	certificate = &msg[idx];
	idx += cert_len;
	all_cert_len -= (cert_len + 3);

	// check certification
#if TLS_CHECK_CERTIFICATE
	sign_ok = FALSE;
	if (all_cert_len == 0) {
		// self-signatured
		sign_ok = TLS_checkCertificateSignature(certificate, certificate);
	} else {
		// use root certification
		check_cert = certificate;
		while (all_cert_len > 0) {
			cert_len = TLS_getUint24(&msg[idx]);
			idx += 3;
			root_cert = &msg[idx];
			idx += cert_len;
			all_cert_len -= (cert_len + 3);

			sign_ok = TLS_checkCertificateSignature(check_cert, root_cert);
			if (!sign_ok) {
				break;
			}

			check_cert = root_cert;
		}
	}
#else
	sign_ok = TRUE;
#endif

	// store public key
	public_key_len = TLS_GetPublicKey(certificate, &public_key);
	handle->handshake->public_key = TLS_malloc(public_key_len);
	if (handle->handshake->public_key == 0) {
		return (FALSE);
	}
	TLS_memcpy(handle->handshake->public_key, public_key, public_key_len);

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

//=============================================================================
// Receive ServerHelloDone
//=============================================================================
bool TLS_RecvServerHelloDone(TLS_Handle_t *handle,
							 uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	uint32_t c_len;

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_SERVER_HELLO_DONE) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	/*** nothing to do ***/

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

//=============================================================================
// Receive ServerKeyExchange
//=============================================================================
bool TLS_RecvServerKeyExchange(TLS_Handle_t *handle,
							   uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	uint32_t c_len;

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_SERVER_KEY_EXCHANGE) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	/*** not implemented ***/

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

//=============================================================================
// Receive CertificateRequest
//=============================================================================
bool TLS_RecvCertificateRequest(TLS_Handle_t *handle,
								uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	uint32_t c_len;

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_CERTIFICATE_REQUEST) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	/*** not implemented ***/

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

//=============================================================================
// Send Certificate (if CertificateRequest received)
//=============================================================================
bool TLS_SendCertificate(TLS_Handle_t *handle)
{
	/*** not implemented ***/

	return (FALSE);
}

//=============================================================================
static TLS_KeyExType_t TLS_keyExType(TLS_CipherSuite_t cipher_suite)
{
	switch (cipher_suite) {
	case TLS_NULL_WITH_NULL_NULL:
		return (TLS_KEYEX_NULL);

	case TLS_RSA_WITH_NULL_MD5:
	case TLS_RSA_WITH_NULL_SHA:
	case TLS_RSA_WITH_NULL_SHA256:
	case TLS_RSA_WITH_RC4_128_MD5:
	case TLS_RSA_WITH_RC4_128_SHA:
	case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
	case TLS_RSA_WITH_AES_128_CBC_SHA:
	case TLS_RSA_WITH_AES_256_CBC_SHA:
	case TLS_RSA_WITH_AES_128_CBC_SHA256:
	case TLS_RSA_WITH_AES_256_CBC_SHA256:
		return (TLS_KEYEX_RSA);

	case TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA:
	case TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA:
	case TLS_DH_DSS_WITH_AES_128_CBC_SHA:
	case TLS_DH_RSA_WITH_AES_128_CBC_SHA:
	case TLS_DH_DSS_WITH_AES_256_CBC_SHA:
	case TLS_DH_RSA_WITH_AES_256_CBC_SHA:
    case TLS_DH_DSS_WITH_AES_128_CBC_SHA256:
	case TLS_DH_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_DH_DSS_WITH_AES_256_CBC_SHA256:
    case TLS_DH_RSA_WITH_AES_256_CBC_SHA256:
		return (TLS_KEYEX_DH);

	case TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA:
	case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
	case TLS_DHE_DSS_WITH_AES_128_CBC_SHA:
	case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
	case TLS_DHE_DSS_WITH_AES_256_CBC_SHA:
	case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
    case TLS_DHE_DSS_WITH_AES_128_CBC_SHA256:
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
    case TLS_DHE_DSS_WITH_AES_256_CBC_SHA256:
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
		return (TLS_KEYEX_DHE);

    case TLS_DH_anon_WITH_RC4_128_MD5:
    case TLS_DH_anon_WITH_3DES_EDE_CBC_SHA:
    case TLS_DH_anon_WITH_AES_128_CBC_SHA:
    case TLS_DH_anon_WITH_AES_256_CBC_SHA:
    case TLS_DH_anon_WITH_AES_128_CBC_SHA256:
    case TLS_DH_anon_WITH_AES_256_CBC_SHA256:
		return (TLS_KEYEX_DH_anon);
	}

	return (TLS_KEYEX_NULL);
}

//=============================================================================
static uint8_t *TLS_makePremasterSecret(TLS_Version_t ver)
{
	uint8_t *premaster;

	premaster = TLS_malloc(TLS_PREMASTER_SECRET_LEN);
	if (premaster == NULL) {
		return (NULL);
	}

	TLS_setUint16(premaster, ver);
	TLS_getRandomBytes(premaster + 2, TLS_PREMASTER_SECRET_LEN - 2);

	return (premaster);
}

//=============================================================================
static bool TLS_makeMasterSecret(TLS_Handle_t *handle,
								 uint8_t *premaster_secret)
{
	uint8_t *master;
	uint32_t label_len;
	uint8_t *label_seed;
	uint32_t ls_len;
	uint8_t *ptr;

	if (handle == NULL) return (NULL);

	master = handle->session_info.master_secret;

	label_len = TLS_strlen(TLS_MASTER_SECRET_LABEL);
	ls_len = label_len + TLS_CLIENT_RANDOM_LEN + TLS_SERVER_RANDOM_LEN;
	label_seed = TLS_malloc(ls_len);
	if (label_seed == NULL) {
		return (FALSE);
	}
	ptr = label_seed;
	TLS_strcpy((char *)ptr, TLS_MASTER_SECRET_LABEL);
	ptr += label_len;
	TLS_memcpy(ptr, handle->handshake->client_random, TLS_CLIENT_RANDOM_LEN);
	ptr += TLS_CLIENT_RANDOM_LEN;
	TLS_memcpy(ptr, handle->handshake->server_random, TLS_SERVER_RANDOM_LEN);

	if (!PRF_calc(premaster_secret, TLS_PREMASTER_SECRET_LEN,
				  label_seed, ls_len, master, TLS_MASTER_SECRET_LEN)) {
		TLS_free(label_seed);
		return (FALSE);
	}
	TLS_free(label_seed);

	handle->flag |= TLS_VALID_MASTER_SECRET;

	return (TRUE);
}

//=============================================================================
static bool TLS_encryptPremasterSecret(uint8_t *msg, uint32_t m_len,
									   uint8_t *premaster_secret,
									   uint8_t *public_key)
{
	uint8_t *rsa;
	uint32_t rsa_len;

	if (msg == NULL || premaster_secret == NULL || public_key == NULL) {
		return (FALSE);
	}

	msg[0] = 0x00;
	msg[1] = 0x02;
	TLS_getRandomBytesNoZero(&msg[2], m_len - TLS_PREMASTER_SECRET_LEN - 3);
	msg[m_len - TLS_PREMASTER_SECRET_LEN - 1] = 0x00;
	TLS_memcpy(&msg[m_len - TLS_PREMASTER_SECRET_LEN],
			   premaster_secret, TLS_PREMASTER_SECRET_LEN);

	rsa_len = TLS_encryptRSA(msg, m_len, public_key, &rsa);
	if (rsa_len == 0 || rsa_len != m_len) {
		return (FALSE);
	}

	TLS_memcpy(msg, rsa, m_len);

	TLS_free(rsa);

	return (TRUE);
}

//=============================================================================
// Send ClientKeyExchange
//=============================================================================
bool TLS_SendClientKeyExchange(TLS_Handle_t *handle)
{
	uint8_t *msg = NULL;
	uint32_t m_len;
	bool result;

	uint8_t *mod_buf;
	uint32_t mod_len;

	uint8_t *premaster_secret = NULL;

#if 1
	int cnt;
#endif

	if (handle == NULL) return (FALSE);

	switch (TLS_keyExType(handle->sel_cipher_suite)) {
	case TLS_KEYEX_RSA:
		mod_len = TLS_GetModulus(handle->handshake->public_key, &mod_buf);

		// header(4), length(2), premaster_secret(N)
		m_len = 4 + 2 + mod_len;
		msg = TLS_malloc(m_len);
		if (msg == NULL) return (FALSE);

		// make premaster secret
		premaster_secret = TLS_makePremasterSecret(handle->ver);
		if (premaster_secret == NULL) {
			TLS_free(msg);
			return (FALSE);
		}

#if 1
		printf("premaster:\n");
		for (cnt = 0; cnt < TLS_PREMASTER_SECRET_LEN; cnt ++) {
			printf("%02x ", premaster_secret[cnt]);
		}
		printf("\n");
#endif

		// encrypte premaster secret
		if (!TLS_encryptPremasterSecret(&msg[6], mod_len,
										premaster_secret,
										handle->handshake->public_key)) {
			TLS_free(msg);
			TLS_free(premaster_secret);
			return (FALSE);
		}

		msg[0] = TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE;
		TLS_setUint24(&msg[1], mod_len + 2);
		TLS_setUint16(&msg[4], mod_len);
		break;

	default:
		/*** not implemented ***/
		return (FALSE);
	}

	result = TLS_SendMsg(handle, TLS_MSG_HANDSHAKE, handle->ver, msg, m_len);
	if (result) {
		Hash_Update(&handle->handshake->msg_md5, msg, m_len);
		Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

		TLS_makeMasterSecret(handle, premaster_secret);

#if 1
		printf("master secret:\n");
		for (cnt = 0; cnt < TLS_MASTER_SECRET_LEN; cnt ++) {
			printf("%02x ", handle->session_info.master_secret[cnt]);
		}
		printf("\n");
#endif
	}
	if (premaster_secret != NULL) {
		TLS_free(premaster_secret);
	}
	if (msg != NULL) {
		TLS_free(msg);
	}

	return (result);
}

//=============================================================================
// Send CertificateVerify
//=============================================================================
bool TLS_SendCertificateVerify(TLS_Handle_t *handle)
{
	/*** not implemented ***/

	return (FALSE);
}

//=============================================================================
static bool TLS_getVerifyData(TLS_Handle_t *handle, char *label, uint8_t *out)
{
	uint32_t label_len;
	uint8_t *label_seed;
	uint32_t ls_len;
	uint8_t *ptr;
	bool result;
	Hash_t temp_hash;

	if (handle == NULL || label == NULL || out == NULL) return (FALSE);

	// alloc label and seed space
	label_len = TLS_strlen(label);
	ls_len = label_len + Hash_Size(HASH_MD5) + Hash_Size(HASH_SHA1);
	label_seed = TLS_malloc(ls_len);
	if (label_seed == NULL) {
		return (FALSE);
	}

	ptr = label_seed;
	TLS_strcpy((char *)ptr, label);
	ptr += label_len;
	temp_hash = handle->handshake->msg_md5;
	Hash_GetDigest(&temp_hash, ptr);
	ptr += Hash_Size(HASH_MD5);
	temp_hash = handle->handshake->msg_sha1;
	Hash_GetDigest(&temp_hash, ptr);

	result = PRF_calc(handle->session_info.master_secret,
					  TLS_MASTER_SECRET_LEN,
					  label_seed, ls_len, out, TLS_VERIFY_DATA_LEN);

	TLS_free(label_seed);

	return (result);
}

//=============================================================================
// Send Finished
//=============================================================================
bool TLS_SendFinished(TLS_Handle_t *handle)
{
	uint8_t *msg;
	uint32_t m_len;
	bool result;

	if (handle == NULL) return (FALSE);

	// header(4), verify_data(12)
	m_len = 4 + TLS_VERIFY_DATA_LEN;
	msg = TLS_malloc(m_len);
	if (msg == NULL) return (FALSE);

	// set verify_data
	if (!TLS_getVerifyData(handle, TLS_CLIENT_FINISHED_LABEL, &msg[4])) {
		TLS_free(msg);
		return (FALSE);
	}

	msg[0] = TLS_HANDSHAKE_FINISHED;
	TLS_setUint24(&msg[1], m_len - 4);

	result = TLS_SendMsg(handle, TLS_MSG_HANDSHAKE, handle->ver, msg, m_len);
	if (result) {
		Hash_Update(&handle->handshake->msg_md5, msg, m_len);
		Hash_Update(&handle->handshake->msg_sha1, msg, m_len);
	}
	TLS_free(msg);

	return (result);
}

//=============================================================================
// Receive Finished
//=============================================================================
bool TLS_RecvFinished(TLS_Handle_t *handle, uint8_t *msg, uint32_t m_len)
{
	uint32_t idx;
	uint32_t c_len;
	uint8_t verify_data[TLS_VERIFY_DATA_LEN];

	if (handle == NULL || msg == NULL || m_len == 0) return (FALSE);

	// type(1)
	if (msg[0] != TLS_HANDSHAKE_FINISHED) return (FALSE);

	// length(3)
	c_len = TLS_getUint24(&msg[1]);
	if (m_len != (c_len + 4)) return (FALSE);

	if (c_len != TLS_VERIFY_DATA_LEN) return (FALSE);

	// compare verify data
	if (!TLS_getVerifyData(handle, TLS_SERVER_FINISHED_LABEL, verify_data)) {
		return (FALSE);
	}
	if (TLS_memcmp(verify_data, &msg[4], TLS_VERIFY_DATA_LEN) != 0) {
		return (FALSE);
	}

	Hash_Update(&handle->handshake->msg_md5, msg, m_len);
	Hash_Update(&handle->handshake->msg_sha1, msg, m_len);

	return (TRUE);
}

/******************************** END-OF-FILE ********************************/
