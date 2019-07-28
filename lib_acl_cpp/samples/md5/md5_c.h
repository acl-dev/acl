#pragma once

#include <stdlib.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

typedef struct md5_ctx {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
} md5_ctx;

void md5_init(md5_ctx *context);
void mf5_update(md5_ctx *context, const void *buf, unsigned len);
void md5_final(uint8_t digest[16], md5_ctx *context);
void md5_transform(uint32_t buf[4], uint32_t const in[16]);
const char* md5_key(const char *s, size_t s_len, const char *k, size_t len,
	void* buf, size_t size);
const char* md5_string(const char *s, const char *k, size_t len,
	char* buf, size_t size);
const char* md5_key2string(const unsigned char *s, size_t len,
	char* buf, size_t size);
