/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "tls.h"

//=============================================================================
#define isPrim(tag) (((tag)[0] & 0x20) == 0)
#define isCons(tag) (((tag)[0] & 0x20) != 0)

#define tagID(tag) ((tag)[0] & 0x1f)

//=============================================================================
enum {
	ASN_BOOLEAN = 1,
	ASN_INTEGER = 2,
	ASN_BIT_STRING = 3,
	ASN_OCTET_STRING = 4,
	ASN_NULL = 5,
	ASN_OBJECT_ID = 6,
	ASN_OBJECT_DESC = 7,

	ASN_SEQUENCE = 16,
	ASN_SET = 17,

	ASN_PRINTABLE_STRING = 19,

	ASN_UTC_TIME = 23,
	ASN_GENERALIZED_TIME = 24
};

//=============================================================================
static uint32_t TLS_tagHeaderLen(uint8_t *tag)
{
	tag ++;
	if ((*tag & 0x80) == 0) {
		return (2);
	} else {
		return ((*tag & 0x7f) + 2);
	}
}

//=============================================================================
static uint32_t TLS_tagLen(uint8_t *tag)
{
	int num_bytes;
	uint32_t len;

	tag ++;
	if ((*tag & 0x80) == 0) {
		return (*tag);
	} else {
		num_bytes = *tag & 0x7f;
		if (num_bytes > 4) {
			// too large
			return (0);
		}
		tag ++;
		len = 0;
		for ( ; num_bytes > 0; num_bytes --) {
			len <<= 8;
			len += *tag;
			tag ++;
		}
		return (len);
	}
}

//=============================================================================
static uint8_t *TLS_tagValue(uint8_t *tag)
{
	int num_bytes;

	tag ++;
	if ((*tag & 0x80) == 0) {
		tag ++;
	} else {
		num_bytes = *tag & 0x7f;
		tag += (num_bytes + 1);
	}
	return (tag);
}

//=============================================================================
static uint8_t *TLS_tagSkip(uint8_t *tag)
{
	int num_bytes;
	uint32_t len;

	len = TLS_tagLen(tag);
	if (len == 0) return (NULL);

	tag = TLS_tagValue(tag);

	tag += len;

	return (tag);
}

//=============================================================================
uint32_t TLS_GetSignedCertificate(const uint8_t *der_buf, uint8_t **ptr)
{
	uint32_t len;
	uint8_t *tag;

	if (der_buf == NULL || ptr == NULL) return (0);

	tag = (uint8_t *)der_buf;

	// root
	if (!isCons(tag)) return (0);

	// root value
	tag = TLS_tagValue(tag);

	// signedCertificate
	if (!isCons(tag)) return (0);

	// signedCertificate length
	len = TLS_tagLen(tag);
	if (len == 0) return (0);

	// return including tag header
	*ptr = tag;

	len += TLS_tagHeaderLen(tag);

	return (len);
}

//=============================================================================
uint32_t TLS_GetEncryptedSignature(const uint8_t *der_buf, uint8_t **ptr)
{
	uint32_t len;
	uint8_t *tag;

	if (der_buf == NULL || ptr == NULL) return (0);

	tag = (uint8_t *)der_buf;

	// root
	if (!isCons(tag)) return (0);

	// root value
	tag = TLS_tagValue(tag);

	// skip value x 2
	tag = TLS_tagSkip(tag); // signedCertificate
	if (tag == NULL) return (0);

	tag = TLS_tagSkip(tag); // encryptAlgorithm
	if (tag == NULL) return (0);

	// encryptedSignature
	if (!isPrim(tag)) return (0);

	// encryptedSignature length
	len = TLS_tagLen(tag);
	if (len == 0) return (0);

	// encryptedSignature value
	tag = TLS_tagValue(tag);
	*ptr = tag;

	return (len);
}

//=============================================================================
uint32_t TLS_GetPublicKey(const uint8_t *der_buf, uint8_t **ptr)
{
	uint32_t len;
	uint8_t *tag;

	if (der_buf == NULL || ptr == NULL) return (0);

	tag = (uint8_t *)der_buf;

	// root
	if (!isCons(tag)) return (0);

	// root value
	tag = TLS_tagValue(tag);

	// signedCertificate
	if (!isCons(tag)) return (0);

	// signedCertificate value
	tag = TLS_tagValue(tag);

	// version?
	if (isPrim(tag)) {
		// no version, but serial number
		tag = TLS_tagSkip(tag); // serial number
		if (tag == NULL) return (0);
	} else {
		// version number
		tag = TLS_tagSkip(tag); // version 
		if (tag == NULL) return (0);
		tag = TLS_tagSkip(tag); // serial number
		if (tag == NULL) return (0);
	}

	tag = TLS_tagSkip(tag); // signature
	if (tag == NULL) return (0);
	tag = TLS_tagSkip(tag); // issuer
	if (tag == NULL) return (0);
	tag = TLS_tagSkip(tag); // validity
	if (tag == NULL) return (0);
	tag = TLS_tagSkip(tag); // subject
	if (tag == NULL) return (0);

	// subjectPublicKeyInfo
	if (!isCons(tag)) return (0);

	// subjectPublicKeyInfo value
	tag = TLS_tagValue(tag);

	tag = TLS_tagSkip(tag); // argorithm
	if (tag == NULL) return (0);

	// subjectPublicKey
	if (!isPrim(tag) || tagID(tag) != ASN_BIT_STRING) return (0);

	// subjectPublicKey length
	len = TLS_tagLen(tag);

	// subjectPublicKey value
	tag = TLS_tagValue(tag);
	*ptr = tag;

	return (len);
}

//=============================================================================
uint32_t TLS_GetModulus(const uint8_t *public_key, uint8_t **ptr)
{
	uint8_t *tag;
	uint32_t len;

	if (public_key == NULL || ptr == NULL) return (0);

	tag = (uint8_t *)public_key;

	if (*tag == 0) tag ++;

	if (!isCons(tag)) return (0);

	tag = TLS_tagValue(tag);

	// modulus
	if (!isPrim(tag) || tagID(tag) != ASN_INTEGER) return (0);

	len = TLS_tagLen(tag);

	tag = TLS_tagValue(tag);

	// skip 0x00 MSB
	if (*tag == 0) {
		tag ++;
		len --;
	}

	*ptr = tag;

	return (len);
}

//=============================================================================
uint32_t TLS_GetExponent(const uint8_t *public_key, uint8_t **ptr)
{
	uint8_t *tag;
	uint32_t len;

	if (public_key == NULL || ptr == NULL) return (0);

	tag = (uint8_t *)public_key;

	if (*tag == 0) tag ++;

	if (!isCons(tag)) return (0);

	tag = TLS_tagValue(tag);

	tag = TLS_tagSkip(tag); // modulus
	if (tag == NULL) return (0);

	// exponent
	if (!isPrim(tag) || tagID(tag) != ASN_INTEGER) return (0);

	len = TLS_tagLen(tag);

	tag = TLS_tagValue(tag);
	*ptr = tag;

	return (len);
}

/******************************** END-OF-FILE ********************************/
