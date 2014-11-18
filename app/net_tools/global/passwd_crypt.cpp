#include "StdAfx.h"
#include "polarssl/des.h"
#include "passwd_crypt.h"

static const unsigned char des3_test_keys[24] =
{
	0x11, 0x43, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
	0x13, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
	0x25, 0x47, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23
};

static const unsigned char des3_test_iv[8] =
{
	0x02, 0x04, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
};

char* passwd_crypt(const char* in)
{
	const char* myname = "passwd_crypt";
	des3_context ctx;
	unsigned char iv[8];
	unsigned char* data, *result;
	size_t len;

	if (des3_set3key_enc(&ctx, des3_test_keys) != 0)
	{
		acl_msg_error("%s: des3_set3key_enc error!", myname);
		return NULL;
	}

	len = strlen(in);
	len += 8 - len % 8 + 8;
	data = (unsigned char*) acl_mycalloc(1, len);
	memcpy(data, in, len);

	memcpy(iv, des3_test_iv, 8);

	if (des3_crypt_cbc(&ctx, DES_ENCRYPT, len, iv, data, data) != 0)
	{
		acl_myfree(data);
		acl_msg_error("%s: des3_crypt_cbc error!", myname);
		return NULL;
	} 
	result = acl_base64_encode((const char*) data, (int) len + 1);
	acl_myfree(data);
	return (char*) result;
}

char* passwd_decrypt(const char* in)
{
	const char* myname = "passwd_decrypt";
	des3_context ctx;
	unsigned char iv[8];
	unsigned char* buf;
	char* data;
	int   n;

	if (des3_set3key_dec(&ctx, des3_test_keys) != 0)
	{
		acl_msg_error("%s: des3_set3key_dec error!", myname);
		return NULL;
	}

	n = acl_base64_decode(in, &data);
	if (n < 0)
	{
		acl_msg_error("%s: acl_base64_decode error!", myname);
		return NULL;
	}

	memcpy(iv, des3_test_iv, 8);

	n += 8 - n % 8 + 8;
	buf = (unsigned char*) acl_mycalloc(1, n);
	memcpy(buf, data, n);
	acl_myfree(data);

	if (des3_crypt_cbc(&ctx, DES_DECRYPT, n, iv, buf, buf) != 0)
	{
		acl_myfree(buf);
		acl_msg_error("%s: des3_crypt_cbc error!", myname);
		printf("n: %d\n", (int) n);
		return NULL;
	}

	return (char*) buf;
}
