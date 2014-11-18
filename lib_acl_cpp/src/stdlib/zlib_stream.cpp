#include "acl_stdafx.hpp"
#include "zlib.h"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/zlib_stream.hpp"

#define HAVE_H_ZLIB

#ifdef HAVE_H_ZLIB
# if defined(WIN32) || defined(USE_DYNAMIC)

typedef int     (*deflateInit_fn)(z_stream*, int, const char*, int);
typedef int     (*deflate_fn)(z_stream*, int);
typedef int     (*deflateReset_fn)(z_stream*);
typedef int     (*deflateEnd_fn)(z_stream*);

typedef int     (*inflateInit_fn)(z_stream*, int, const char*, int);
typedef int     (*inflate_fn)(z_stream*, int);
typedef int     (*inflateReset_fn)(z_stream*);
typedef int     (*inflateEnd_fn)(z_stream*);

static deflateInit_fn __deflateInit = NULL;
static deflate_fn __deflate = NULL;
static deflateReset_fn __deflateReset = NULL;
static deflateEnd_fn __deflateEnd = NULL;
static inflateInit_fn __inflateInit = NULL;
static inflate_fn __inflate = NULL;
static inflateReset_fn __inflateReset = NULL;
static inflateEnd_fn __inflateEnd = NULL;

static acl_pthread_once_t __zlib_once = ACL_PTHREAD_ONCE_INIT;
static ACL_DLL_HANDLE __zlib_dll = NULL;

// 程序退出时释放动态加载的 zlib.dll 库
static void __zlib_dll_unload(void)
{
	if (__zlib_dll != NULL)
	{
		acl_dlclose(__zlib_dll);
		__zlib_dll = NULL;
		logger("zlib.dll unload ok");
	}
}

// 动态加载 zlib.dll 库
static void __zlib_dll_load(void)
{
	if (__zlib_dll != NULL)
		logger_fatal("__zlib_dll not null");

#ifdef WIN32
	__zlib_dll = acl_dlopen("zlib.dll");
#else
	__zlib_dll = acl_dlopen("libz.so");
#endif
	if (__zlib_dll == NULL)
		logger_fatal("load zlib.dll error: %s", acl_last_serror());

	__deflateInit = (deflateInit_fn) acl_dlsym(__zlib_dll, "deflateInit_");
	if (__deflateInit == NULL)
		logger_fatal("load deflateInit from zlib.dll error: %s",
			acl_last_serror());

	__deflate = (deflate_fn) acl_dlsym(__zlib_dll, "deflate");
	if (__deflate == NULL)
		logger_fatal("load deflate from zlib.dll error: %s",
			acl_last_serror());

	__deflateReset = (deflateReset_fn) acl_dlsym(__zlib_dll, "deflateReset");
	if (__deflateReset == NULL)
		logger_fatal("load deflateReset from zlib.dll error: %s",
			acl_last_serror());

	__deflateEnd = (deflateEnd_fn) acl_dlsym(__zlib_dll, "deflateEnd");
	if (__deflateEnd == NULL)
		logger_fatal("load deflateEnd from zlib.dll error: %s",
			acl_last_serror());

	__inflateInit = (inflateInit_fn) acl_dlsym(__zlib_dll, "inflateInit2_");
	if (__inflateInit == NULL)
		logger_fatal("load inflateInit from zlib.dll error: %s",
			acl_last_serror());

	__inflate = (inflate_fn) acl_dlsym(__zlib_dll, "inflate");
	if (__inflate == NULL)
		logger_fatal("load inflate from zlib.dll error: %s",
			acl_last_serror());

	__inflateReset = (inflateReset_fn) acl_dlsym(__zlib_dll, "inflateReset");
	if (__inflateReset == NULL)
		logger_fatal("load inflateReset from zlib.dll error: %s",
			acl_last_serror());

	__inflateEnd = (inflateEnd_fn) acl_dlsym(__zlib_dll, "inflateEnd");
	if (__inflateEnd == NULL)
		logger_fatal("load inflateEnd from zlib.dll error: %s",
			acl_last_serror());

	logger("zlib.dll loaded");
	atexit(__zlib_dll_unload);
}
# else
#  define __deflateInit         deflateInit_
#  define __deflate             deflate
#  define __deflateReset	deflateReset
#  define __deflateEnd          deflateEnd
#  define __inflateInit         inflateInit2_
#  define __inflate             inflate
#  define __inflateReset	inflateReset
#  define __inflateEnd          inflateEnd
# endif
#endif

namespace acl
{

static void* __zlib_calloc(void* ctx acl_unused,
	unsigned int nitem, unsigned int size)
{
	return (acl_mymalloc(nitem * size));
}

static void __zlib_free(void* ctx acl_unused, void* ptr)
{
	acl_myfree(ptr);
}

zlib_stream::zlib_stream()
{
	finished_ = false;
	zstream_ = (z_stream*) acl_mycalloc(1, sizeof(z_stream));
	zstream_->zalloc = __zlib_calloc;
	zstream_->zfree = __zlib_free;
	zstream_->opaque = (void*) this;

	is_compress_ = true;  // 默认为压缩状态
	flush_ = zlib_flush_off;

#ifdef  HAVE_H_ZLIB
#if defined(WIN32) || defined(USE_DYNAMIC)
	acl_pthread_once(&__zlib_once, __zlib_dll_load);
# endif
#endif
}

zlib_stream::~zlib_stream()
{
	acl_myfree(zstream_);
}

bool zlib_stream::zlib_compress(const char* in, int len, string* out,
	zlib_level_t level /* = zlib_default */)
{
	bool ret = zip_begin(level);
	if (ret == false)
	{
		zip_reset();
		return (false);
	}

	ret = zip_update(in, len, out, zlib_flush_sync);
	if (ret == false)
	{
		zip_reset();
		return (false);
	}

	return (zip_finish(out));
}

bool zlib_stream::zlib_uncompress(const char* in, int len, string* out,
	bool have_zlib_header /* = true */, int wsize /* = 15 */)
{
	bool ret = unzip_begin(have_zlib_header, wsize);
	if (ret == false)
	{
		unzip_reset();
		return (false);
	}

	ret = unzip_update(in, len, out, zlib_flush_sync);
	if (ret == false)
	{
		unzip_reset();
		return (false);
	}

	return (unzip_finish(out));
}

#define BUF_MIN	4000

bool zlib_stream::update(int (*func)(z_stream*, int),
	zlib_flush_t flag, const char* in, int len, string* out)
{
	if (finished_)
		return (true);

	acl_assert(in);
	acl_assert(len >= 0);
	acl_assert(out);

	int   pos = 0;
	int   dlen, nbuf, ret;

	zstream_->avail_out = 0;

	while (true)
	{
		acl_assert(len >= 0);

		nbuf = (int) (out->capacity() - out->length());

		// 需要保证输出缓冲区的可用空间
		if (nbuf < BUF_MIN)
		{
			nbuf = (int) out->length() + BUF_MIN;
			out->space(nbuf);
		}

		dlen = (int) out->length();
		nbuf = (int) out->capacity() - dlen;
		acl_assert(nbuf >= BUF_MIN);

		zstream_->next_in = (unsigned char*) in + pos;
		zstream_->avail_in = (unsigned int) len;
		zstream_->next_out = (unsigned char*) out->c_str() + dlen;
		zstream_->avail_out = (unsigned int) nbuf;

		ret = func(zstream_, flag);
		if (ret == Z_STREAM_END)
		{
			acl_assert(flag == Z_FINISH || func == __inflate);
			finished_ = true;

			// 修改输出缓冲区的指针位置
			acl_assert(nbuf >= (int) zstream_->avail_out);
			dlen += nbuf - zstream_->avail_out;
			out->set_offset((ssize_t) dlen);

			if (zstream_->avail_in == 0)
				zstream_->next_in = NULL;

			return (true);
		}
		else if (ret != Z_OK)
		{
			logger_error("update(%s) error",
				func == __deflate ? "deflate" : "inflate");
			return (false);
		}

		// 修改输出缓冲区的指针位置
		acl_assert(nbuf >= (int) zstream_->avail_out);
		dlen += nbuf - zstream_->avail_out;
		out->set_offset((ssize_t) dlen);

		// 如输入数据完成则退出循环
		if (zstream_->avail_in == 0)
		{
			zstream_->next_in = NULL;
			break;
		}

		// 更新输入数据的下一个位置
		acl_assert(len >= (int) zstream_->avail_in);
		pos += len - zstream_->avail_in;

		// 更新剩余数据长度
		len = zstream_->avail_in;
	}

	return (true);
}

bool zlib_stream::flush_out(int (*func)(z_stream*, int),
	zlib_flush_t flag, string* out)
{
	if (finished_)
		return (true);

	acl_assert(zstream_->avail_in == 0);
	acl_assert(zstream_->next_in == NULL);

	int   dlen, nbuf, ret;

	nbuf = 0;
	while (true)
	{
		nbuf = (int) (out->capacity() - out->length());

		// 需要保证输出缓冲区的可用空间
		if (nbuf < BUF_MIN)
		{
			nbuf = (int) out->length() + BUF_MIN;
			out->space(nbuf);
		}

		dlen = (int) out->length();
		nbuf = (int) out->capacity() - dlen;
		acl_assert(nbuf >= BUF_MIN);

		zstream_->next_out = (unsigned char*) out->c_str() + dlen;
		zstream_->avail_out = (unsigned int) nbuf;

		ret = func(zstream_, flag);
		if (ret == Z_STREAM_END)
		{
			acl_assert(flag == Z_FINISH || func == __inflate);
			finished_ = true;

			// 修改输出缓冲区的指针位置
			acl_assert(nbuf >= (int) zstream_->avail_out);
			dlen += nbuf - zstream_->avail_out;
			out->set_offset((ssize_t) dlen);

			if (zstream_->avail_in == 0)
				zstream_->next_in = NULL;
			break;
		}
		else if (ret == Z_BUF_ERROR)
		{
			if (zstream_->avail_out > 0)
			{
				logger_error("flush_out(%s) error",
					func == __deflate ?
					"deflate" : "inflate");
				return (false);
			}

			// 修改输出缓冲区的指针位置
			acl_assert(nbuf >= (int) zstream_->avail_out);
			dlen += nbuf - zstream_->avail_out;
			out->set_offset((ssize_t) dlen);
		}
		else if (ret != Z_OK)
		{
			logger_error("update(%s) error", func == __deflate ?
				"deflate" : "inflate");
			return (false);
		}
		else if (zstream_->avail_out == 0)
		{
			// 修改输出缓冲区的指针位置
			acl_assert(nbuf >= (int) zstream_->avail_out);
			dlen += nbuf - zstream_->avail_out;
			out->set_offset((size_t) dlen);
		}
		else
			break;
	}

	return (true);
}

bool zlib_stream::zip_begin(zlib_level_t level /* = zlib_default */)
{
	is_compress_ = true;
	int   ret = __deflateInit(zstream_, level,
		ZLIB_VERSION, sizeof(z_stream));
	if (ret != Z_OK)
	{
		logger_error("deflateInit error");
		return (false);
	}
	return (true);
}

bool zlib_stream::zip_update(const char* in, int len, string* out,
	zlib_flush_t flag /* = zlib_flush_off */)
{
	return (update(__deflate, flag, in, len, out));
}

bool zlib_stream::zip_finish(string* out)
{
	bool ret = flush_out(__deflate, zlib_flush_finish, out);
	//(void) __deflateReset(zstream_);
	__deflateEnd(zstream_);
	finished_ = false;
	return (ret);
}

bool zlib_stream::zip_reset()
{
	return (__deflateEnd(zstream_) == Z_OK ? true : false);
}

bool zlib_stream::unzip_begin(bool have_zlib_header /* = true */,
	int wsize /* = 15 */)
{
	is_compress_ = false;
	int   ret = __inflateInit(zstream_, have_zlib_header ?
		wsize : -wsize, ZLIB_VERSION, sizeof(z_stream));
	if (ret != Z_OK)
	{
		logger_error("inflateInit error");
		return (false);
	}
	return (true);
}

bool zlib_stream::unzip_update(const char* in, int len, string* out,
	zlib_flush_t flag /* = zlib_flush_off */)
{
	return (update(__inflate, flag, in, len, out));
}

bool zlib_stream::unzip_finish(string* out)
{
	bool ret = flush_out(__inflate, zlib_flush_finish, out);
	//(void) __inflateReset(zstream_);
	__inflateEnd(zstream_);
	finished_ = false;
	return (ret);
}

bool zlib_stream::unzip_reset()
{
	return (__inflateEnd(zstream_) == Z_OK ? true : false);
}

bool zlib_stream::pipe_zip_begin(zlib_level_t level /* = zlib_default */,
	zlib_flush_t flag /* = zlib_flush_off */)
{
	flush_ = flag;
	return (zip_begin(level));
}

bool zlib_stream::pipe_unzip_begin(zlib_flush_t flag /* = zlib_flush_off */)
{
	flush_ = flag;
	return (unzip_begin());
}

int zlib_stream::push_pop(const char* in, size_t len,
	string* out, size_t max /* = 0 */ acl_unused)
{
	if (out == NULL)
		return (0);

	size_t n = out->length();

	if (is_compress_)
	{
		if (zip_update(in, len, out, flush_) == false)
			return (-1);
	}
	else
	{
		if (unzip_update(in, len, out, flush_) == false)
			return (-1);
	}

	return ((int) (out->length() - n));
}

int zlib_stream::pop_end(string* out, size_t max /* = 0 */ acl_unused)
{
	if (out == NULL)
		return (0);

	size_t n = out->length();
	if (is_compress_)
	{
		if (zip_finish(out) == false)
			return (-1);
	}
	else
	{
		if (unzip_finish(out) == false)
			return (-1);
	}

	return ((int) (out->length() - n));
}

void zlib_stream::clear()
{
	if (is_compress_)
		zip_reset();
	else
		unzip_reset();
}

} // namespace acl
