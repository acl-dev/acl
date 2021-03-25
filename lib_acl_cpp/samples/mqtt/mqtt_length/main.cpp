/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 24 Mar 2021 06:49:23 PM CST
 */

#include <stdio.h>
#include <stdlib.h>

static unsigned decode(const unsigned char* buf, unsigned len) {
	unsigned n = 0;
	for (unsigned i = 0; i < len; i++) {
		n |= (unsigned) ((buf[i] & 0x7f) << (7 * i));
	}
	return n;
}

static int encode(unsigned n, unsigned char *buf) {
	int len = 0;
	if (n < 128) {
		buf[len++] = n & 0x7f;
	} else if (n < 16384) {
		buf[len++] = ((unsigned char) n & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 7) & 0x7f);
	} else if (n < 2097152) {
		buf[len++] = ((unsigned char) n & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 7 ) & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 14) & 0x7f);
	} else if (n < 268435456) {
		buf[len++] = ((unsigned char) n & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 7 ) & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 14) & 0x7f) | 0x80;
		buf[len++] = ((unsigned char) (n >> 21) & 0x7f);
	} else {
		printf("invalid dlen_=%u\n", n);
		return -1;
	}

	return len;
}

int main(void) {
	unsigned char buf[4];

	unsigned i, max= 268435455;
	for (i = 0; i < max; i++) {
		int len = encode(i, buf);
		if (len == -1) {
			printf("encode i=%u error\r\n", i);
			break;
		}

		unsigned n = decode(buf, (unsigned) len);
		if (n != i) {
			printf("decode error, n=%u, i=%u\n", n, i);
			break;
		}
		if (i < 10) {
			printf("ok, i=%u, n=%u\n", i, n);
		}
	}

	printf("i=%u, max=%u, status=%s\r\n", i, max, i == max ? "ok" : "error");
	return 0;
}
