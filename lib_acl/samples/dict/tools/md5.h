#ifndef SQUID_MD5_H
#define SQUID_MD5_H

#include <stdlib.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

typedef struct MD5Context {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
} MD5_CTX;


void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, const void *buf, unsigned len);
void MD5Final(uint8_t digest[16], struct MD5Context *context);
void MD5Transform(uint32_t buf[4], uint32_t const in[16]);
const char* MD5Key(const char *s, size_t s_len, const char *k, size_t k_len, void* buf, size_t b_len);
const char* MD5String(const char *s, const char *k, size_t k_len, char* buf, size_t size);
const char* MD5Key2String(const unsigned char *s, size_t s_len, char* buf, size_t size);

#define MD5_DIGEST_CHARS         16

#endif /* SQUID_MD5_H */
