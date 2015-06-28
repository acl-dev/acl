#include "StdAfx.h"
#include "uni2utf8.h"

/**
 * g_unichar_to_utf8:
 * @c: a Unicode character code
 * @buf: output buffer, must have at least 6 bytes of space.
 *       If %NULL, the length will be computed and returned
 *       and nothing will be written to @buf.
 * 
 * Converts a single character to UTF-8.
 * 
 * Return value: number of bytes written
 **/

int uni2utf8(unsigned int c, char *buf, size_t size)
{
	unsigned int len = 0;
	int first;
	int i;

	if (c < 0x80) {
		first = 0;
		len = 1;
	} else if (c < 0x800) {
		first = 0xc0;
		len = 2;
	} else if (c < 0x10000) {
		first = 0xe0;
		len = 3;
	} else if (c < 0x200000) {
		first = 0xf0;
		len = 4;
	} else if (c < 0x4000000) {
		first = 0xf8;
		len = 5;
	} else {
		first = 0xfc;
		len = 6;
	}

	if (buf && size > 0) {
		if (len > size)
			len = (unsigned int) size;
		for (i = len - 1; i > 0; --i) {
			buf[i] = (c & 0x3f) | 0x80;
			c >>= 6;
		}
		buf[0] = c | first;
	}

	return len;
}
