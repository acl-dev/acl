/* MS VisualStudio Projects are monolithic, so we need the following
 * #if to exclude the MD5 code from compile process when we are
 * building the SSL support.
 */
#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <string.h>
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/md5.hpp"
#endif

#ifdef WORDS_BIGENDIAN
static void byteSwap(uint32_t * buf, unsigned words)
{
	uint8_t *p = (uint8_t *) buf;

	do {
		*buf++ = (uint32_t) ((unsigned) p[3] << 8 | p[2]) << 16 |
			((unsigned) p[1] << 8 | p[0]);
		p += 4;
	} while (--words);
}
#else
#define byteSwap(buf,words)
#endif

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
	(w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void transform(unsigned int buf[4], unsigned int const in[16])
{
	register unsigned int a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

namespace acl
{

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
md5::md5()
{
	reset();
}

md5::~md5()
{

}

md5& md5::reset()
{
	buf_[0] = 0x67452301;
	buf_[1] = 0xefcdab89;
	buf_[2] = 0x98badcfe;
	buf_[3] = 0x10325476;
	bytes_[0] = 0;
	bytes_[1] = 0;
	memset(in_, 0, sizeof(in_));

	digest_[0] = 0;
	digest_s_[0] = 0;
	return *this;
}

/*
 * update context to reflect the concatenation of another buffer full
 * of bytes.
 */
md5& md5::update(const void *in, size_t len)
{
	const unsigned char *buf = (const unsigned char *) in;
	unsigned int t;

	/* Update byte count */

	t = bytes_[0];
	if ((bytes_[0] = t + (unsigned int) len) < t)
		bytes_[1]++;	/* Carry from low to high */

	t = 64 - (t & 0x3f);	/* Space available in in_ (at least 1) */
	if ((size_t) t > len)
	{
		memcpy((unsigned char *) in_ + 64 - t, buf, len);
		return *this;
	}
	/* First chunk is an odd size */
	memcpy((unsigned char *) in_ + 64 - t, buf, t);
	byteSwap(in_, 16);
	transform(buf_, in_);
	buf += t;
	len -= t;

	/* Process data in 64-byte chunks */
	while (len >= 64)
	{
		memcpy(in_, buf, 64);
		byteSwap(in_, 16);
		transform(buf_, in_);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */
	memcpy(in_, buf, len);

	return *this;
}

/*
 * finish wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
md5& md5::finish()
{
	int count = bytes_[0] & 0x3f;	/* Number of bytes in ctx->in */
	unsigned char *p = (unsigned char *) in_ + count;

	/* Set the first char of padding to 0x80.  There is always room. */
	*p++ = 0x80;

	/* Bytes of padding needed to make 56 bytes (-8..55) */
	count = 56 - 1 - count;

	if (count < 0)
	{
		/* Padding forces an extra block */
		memset(p, 0, count + 8);
		byteSwap(in_, 16);
		transform(buf_, in_);
		p = (unsigned char *) in_;
		count = 56;
	}
	memset(p, 0, count);
	byteSwap(ctx->in, 14);

	/* Append length in bits and transform */
	in_[14] = bytes_[0] << 3;
	in_[15] = bytes_[1] << 3 | bytes_[0] >> 29;
	transform(buf_, in_);

	byteSwap(buf_, 4);
	memcpy(digest_, buf_, 16);

	/* In case it's sensitive */
	memset(buf_, 0, sizeof(buf_));
	memset(bytes_, 0, sizeof(bytes_));
	memset(in_, 0, sizeof(in_));

	return *this;
}

const char* md5::get_digest() const
{
	return (const char*) digest_;
}

const char* md5::get_string() const
{
	const_cast<md5*>(this)->hex_encode(digest_,
		(char*) digest_s_, sizeof(digest_s_));
	return (const char*) digest_s_;
}

const char* md5::md5_digest(const void *dat, size_t dlen,
	const void *key, size_t klen, void* out, size_t size)
{
	md5 md5;

	size = size > 16 ? 16 : size;

	if (key != NULL && klen > 0)
		md5.update(key, klen);
	md5.update(dat, dlen);
	md5.finish();
	memcpy(out, md5.get_digest(), size);
	return ((const char*) out);
}

const char* md5::md5_string(const void *dat, size_t dlen,
	const void *key, size_t klen, char* out, size_t size)
{
	md5 md5;

	if (key != NULL && klen > 0)
		md5.update(key, klen);
	md5.update(dat, dlen);
	md5.finish();

	const unsigned char* d = (const unsigned char*) md5.get_digest();
	hex_encode(d, out, size);
	return (out);
}

acl_int64 md5::md5_file(const char* path, const void *key, size_t klen,
	char* out, size_t size)
{
	ifstream in;

	if (in.open_read(path) == false)
	{
		logger_error("open file: %s error: %s", path, last_serror());
		return -1;
	}
	return md5_file(in, key, klen, out, size);
}

acl_int64 md5::md5_file(istream& in, const void *key, size_t klen,
	char* out, size_t size)
{
	acl_int64 n = 0;
	char buf[8192];
	int  ret;
	md5 md5;

	if (size < 33)
	{
		logger_error("size(%d) < 33", (int) size);
		return -1;
	}

	if (key && klen > 0)
		md5.update(key, klen);
	while (true)
	{
		ret = in.read(buf, sizeof(buf), false);
		if (ret < 0)
			break;
		n += ret;
		md5.update(buf, ret);
	}
	
	if (n == 0)
		return -1;
	md5.finish();

	const char* ptr = md5.get_digest();
	hex_encode(ptr, out, size);
	return n;
}

const char* md5::hex_encode(const void *in, char* out, size_t size)
{
	size_t i;
	char  buf[34];  // xxx: 必须是 34 个字节
	char *ptr;
	unsigned char digest[16];

	acl_assert(size >= 33);

	memcpy(digest, in, 16);

	for (i = 0; i < 16; i++)
	{
#if _MSC_VER >= 1500
		sprintf_s(&(buf[2 * i]), 3, "%02x", (unsigned char) digest[i]);
		sprintf_s(&(buf[2 * i + 1]), 3, "%02x",
			(unsigned char) (digest[i] << 4));
#else
		sprintf(&(buf[2 * i]), "%02x", (unsigned char) digest[i]);
		sprintf(&(buf[2 * i + 1]), "%02x",
			(unsigned char) (digest[i] << 4));
#endif
	}

	ptr = out;

	for (i = 0; i < 32; i++)
		*ptr++ = buf[i];
	*ptr = '\0';

	return (out);
}

} // namespace acl
