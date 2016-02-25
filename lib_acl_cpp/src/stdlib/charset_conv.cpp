#include "acl_stdafx.hpp"

#ifndef HAVE_H_ICONV
# define HAVE_H_ICONV
#endif

#ifdef	HAVE_H_ICONV
# ifndef USE_WIN_ICONV
#  include <iconv.h>
# endif
#endif

#ifndef ACL_PREPARE_COMPILE
#include <string.h>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#endif

#define SCOPY ACL_SAFE_STRNCPY

static const char UTF8_HEADER[] = { (char) 0xEF, (char) 0xBB, (char) 0xBF, (char) 0x00 };

#ifdef HAVE_H_ICONV
# ifdef USE_WIN_ICONV
#  include "internal/win_iconv.hpp"
#  define __iconv_open    iconv_open
#  define __iconv_close   iconv_close
#  define __iconv         iconv
# elif defined(ACL_WINDOWS)

typedef iconv_t (*iconv_open_fn)(const char*, const char*);
typedef int     (*iconv_close_fn)(iconv_t);
typedef size_t  (*iconv_fn)(iconv_t, const char**, size_t*, char**, size_t*, int*);
typedef int     (*iconvctl_fn)(iconv_t, int, void*);

static iconv_open_fn __iconv_open = NULL;
static iconv_close_fn __iconv_close = NULL;
static iconv_fn __iconv = NULL;
static iconvctl_fn __iconvctl = NULL;

static acl_pthread_once_t __iconv_once = ACL_PTHREAD_ONCE_INIT;
static ACL_DLL_HANDLE __iconv_dll = NULL;

// 程序退出时释放动态加载的 iconv.dll 库
static void __iconv_dll_unload(void)
{
	if (__iconv_dll != NULL)
	{
		acl_dlclose(__iconv_dll);
		__iconv_dll = NULL;
		logger("iconv.dll unload ok");
	}
}

// 动态加载 iconv.dll 库
static void __iconv_dll_load(void)
{
	if (__iconv_dll != NULL)
		logger_fatal("__iconv_dll not null");

	__iconv_dll = acl_dlopen("iconv.dll");
	if (__iconv_dll == NULL)
		logger_fatal("load iconv.dll error: %s", acl_last_serror());

	__iconv_open = (iconv_open_fn) acl_dlsym(__iconv_dll, "libiconv_open");
	if (__iconv_open == NULL)
		logger_fatal("load iconv_open from iconv.dll error: %s",
				acl_last_serror());

	__iconv_close = (iconv_close_fn) acl_dlsym(__iconv_dll, "libiconv_close");
	if (__iconv_close == NULL)
		logger_fatal("load iconv_close from iconv.dll error: %s",
				acl_last_serror());

	__iconv = (iconv_fn) acl_dlsym(__iconv_dll, "libiconv");
	if (__iconv == NULL)
		logger_fatal("load iconv from iconv.dll error: %s",
				acl_last_serror());

	__iconvctl = (iconvctl_fn) acl_dlsym(__iconv_dll, "libiconvctl");
	if (__iconvctl == NULL)
		logger_fatal("load iconvctl from iconv.dll error: %s",
				acl_last_serror());

	logger("iconv.dll loaded");
	atexit(__iconv_dll_unload);
}

# else
#  define __iconv_open    iconv_open
#  define __iconv_close   iconv_close
#  define __iconv         iconv
#  define __iconvctl      iconvctl
# endif
#endif

namespace acl {

#define EQ(x, y) !strcasecmp((x), (y))

charset_conv::charset_conv()
: m_addInvalid(true)
, m_errmsg("ok")
, m_pBuf(NULL)
{
	m_fromCharset[0] = 0;
	m_toCharset[0] = 0;
#ifdef  HAVE_H_ICONV
	m_iconv = (iconv_t) -1;
	m_pInBuf = NULL;
	m_pOutBuf = NULL;
# ifdef ACL_WINDOWS 
#  ifndef USE_WIN_ICONV
	acl_pthread_once(&__iconv_once, __iconv_dll_load);
#  endif
# endif
#endif
}

charset_conv::~charset_conv()
{
#ifdef  HAVE_H_ICONV
	if (m_iconv != (iconv_t) -1)
		__iconv_close(m_iconv);
	if (m_pInBuf)
		acl_vstring_free(m_pInBuf);
	if (m_pOutBuf)
		acl_vstring_free(m_pOutBuf);
#endif
	delete m_pBuf;
}

void charset_conv::set_add_invalid(bool onoff)
{
	m_addInvalid = onoff;
}

const char* charset_conv::serror() const
{
	return (m_errmsg.c_str());
}

void charset_conv::reset()
{
	m_errmsg = "ok";

#ifdef  HAVE_H_ICONV
	if (m_iconv != (iconv_t) -1)
	{
		__iconv_close(m_iconv);
		m_iconv = (iconv_t) - 1;
	}
	m_fromCharset[0] = 0;
	m_toCharset[0] = 0;
#endif
}

bool charset_conv::convert(const char* fromCharset, const char* toCharset,
	const char* in, size_t n, acl::string* out)
{
	if (fromCharset == NULL || toCharset == NULL
		|| in == NULL || n == 0 || out == NULL)
	{
		m_errmsg = "params invalid";
		return (false);
	}

	if (update_begin(fromCharset, toCharset) == false)
	{
		reset();
		return (false);
	}
	if (update(in, n, out) == false)
	{
		reset();
		return (false);
	}
	update_finish(out);
	return (true);
}

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN
#define	SIZE	ACL_VSTRING_SIZE
#define EQ2(x, y) (((x) == NULL && (y) == NULL)  \
		|| ((x) != NULL && (y) != NULL && !strcasecmp((x), (y))))

bool charset_conv::update_begin(const char* fromCharset,
	const char* toCharset)
{
#ifdef  HAVE_H_ICONV
	if (EQ2(fromCharset, toCharset))
		return (true);

	if (fromCharset == NULL || toCharset == NULL)
	{
		if (m_iconv != (iconv_t) -1)
			return (true);

		logger_error("input invalid, from: %s, to: %s, m_conv: %s",
			fromCharset ? fromCharset : "null",
			toCharset ? toCharset : "null",
			m_iconv == (iconv_t) -1 ? "invalid" : "valud");
		m_errmsg = "input invalid";
		return (false);
	}

	// 如果源是 UTF-8 编码，则 m_pTuf8Pre 从 UTF8_HEADER 头部第
	// 一个字节开始进行匹配，否则从最后一个字节 '\0' 开始匹配，
	// 即跳过 UTF-8 头部匹配过程
	if (EQ(fromCharset, "utf-8") || EQ(fromCharset, "utf8"))
		m_pUtf8Pre = UTF8_HEADER;
	else
		m_pUtf8Pre = &UTF8_HEADER[3];

	if (m_iconv != (iconv_t) -1
		&& EQ(m_fromCharset, fromCharset)
		&& EQ(m_toCharset, toCharset))
	{
		return (true);
	}

	SCOPY(m_fromCharset, fromCharset, sizeof(m_fromCharset));
	SCOPY(m_toCharset, toCharset, sizeof(m_toCharset));

	if (m_iconv != (iconv_t) -1)
		__iconv_close(m_iconv);
	m_iconv = __iconv_open(toCharset, fromCharset);
	if (m_iconv == (iconv_t) -1)
	{
		logger_error("iconv_open(%s, %s) error(%s)",
			toCharset, fromCharset, acl_last_serror());
		m_errmsg.format("iconv_open(%s, %s) error(%s)",
			toCharset, fromCharset, acl_last_serror());
		return (false);
	}
	else
	{
#ifdef ACL_WINDOWS
# ifndef USE_WIN_ICONV
		int  n = 1;
		__iconvctl(m_iconv, ICONV_TRIVIALP, &n);

		n = 1;
		__iconvctl(m_iconv, ICONV_SET_DISCARD_ILSEQ, &n);

		n = 1;
		__iconvctl(m_iconv, ICONV_SET_TRANSLITERATE, &n);
# endif // USE_WIN_ICONV
#endif

		char *pNil = NULL;
		size_t zero = 0;
#ifdef	ACL_WINDOWS
# ifdef USE_WIN_ICONV
		__iconv(m_iconv, (const char**) &pNil, &zero, &pNil, &zero);
# else
		__iconv(m_iconv, (const char**) &pNil, &zero, &pNil, &zero, NULL);
# endif // USE_WIN_ICONV
#elif defined(ACL_SUNOS5) || defined(ACL_FREEBSD)
		__iconv(m_iconv, (const char**) &pNil, &zero, &pNil, &zero);
#else
		__iconv(m_iconv, &pNil, &zero, &pNil, &zero);
#endif
		return (true);
	}
#else
	logger_error("no iconv lib");
	m_errmsg = "no iconv lib";
	return (false);
#endif
}

bool charset_conv::update(const char* in, size_t len, acl::string* out)
{
#ifdef  HAVE_H_ICONV
	if (in == NULL)
		logger_fatal("in null");
	if (out == NULL)
		logger_fatal("out null");

	if (EQ(m_fromCharset, m_toCharset))
	{
		out->append(in, len);
		return (true);
	}

	if (m_iconv == (iconv_t) -1)
	{
		logger_error("m_iconv invalid");
		m_errmsg = "m_iconv invalid";
		return (false);
	}

	// 去掉有些 UTF-8 文档中开始的 UTF-8 引导符
	if (*m_pUtf8Pre)
	{
		while (len > 0)
		{
			if (*m_pUtf8Pre == 0x00)
				break;
			else if (*m_pUtf8Pre != *in)
			{
				// 必须使 UTF-8 前缀失效
				m_pUtf8Pre = &UTF8_HEADER[3];
				break;
			}
			m_pUtf8Pre++;
			in++;
			len--;
		}
	}

	if (len == 0)
		return (true);

	if (m_pInBuf == NULL)
		m_pInBuf = acl_vstring_alloc(len);

	if (m_pOutBuf == NULL)
		m_pOutBuf = acl_vstring_alloc(len);
	else
		ACL_VSTRING_SPACE(m_pOutBuf, (int) len);

	// 先将输入数据进行缓冲
	if (*m_pUtf8Pre && m_pUtf8Pre - UTF8_HEADER > 0)
		acl_vstring_memcpy(m_pInBuf, UTF8_HEADER,
			m_pUtf8Pre - UTF8_HEADER);
	acl_vstring_memcat(m_pInBuf, in, len);
	ACL_VSTRING_TERMINATE(m_pInBuf);

	char  *pIn, *pOut;
	size_t ret, nIn, nOut;

	while (true)
	{
		nIn  = LEN(m_pInBuf);
		if (nIn == 0)
			break;
		pIn  = STR(m_pInBuf);
		pOut = STR(m_pOutBuf);
		nOut = SIZE(m_pOutBuf);

#ifdef	ACL_WINDOWS
# ifdef USE_WIN_ICONV
		ret = __iconv(m_iconv, (const char**) &pIn, &nIn,
				&pOut, &nOut);
# else
		int   err;
		ret = __iconv(m_iconv, (const char**) &pIn, &nIn,
				&pOut, &nOut, &err);
		errno = err;
# endif // USE_WIN_ICONV
#elif defined(ACL_SUNOS5) || defined(ACL_FREEBSD)
		ret = __iconv(m_iconv, (const char**) &pIn, &nIn, &pOut, &nOut);
#else
		ret = __iconv(m_iconv, &pIn, &nIn, &pOut, &nOut);
#endif

		if (ret != (size_t) -1)
		{
			if ((ret = SIZE(m_pOutBuf) - nOut) > 0)
				out->append(STR(m_pOutBuf), ret);
			else  // xxx
				out->append(in, len);
			ACL_VSTRING_RESET(m_pInBuf);
			break;
		}
		else if (errno == E2BIG)
		{
			if ((ret = SIZE(m_pOutBuf) - nOut) > 0)
				out->append(STR(m_pOutBuf), ret);
			if (pIn > STR(m_pInBuf) && nIn < LEN(m_pInBuf))
				acl_vstring_memmove(m_pInBuf, pIn, nIn);
			// 扩大内存空间
			ACL_VSTRING_SPACE(m_pOutBuf, SIZE(m_pOutBuf) * 2);
			continue;
		}
		else if (errno == EILSEQ)
		{
			char *pNil = NULL;
			size_t zero = 0;

			// 重置状态, 似乎也没啥用处
#ifdef	ACL_WINDOWS
# ifdef USE_WIN_ICONV
			__iconv(m_iconv, (const char**) &pNil,
				&zero, &pNil, &zero);
# else
			__iconv(m_iconv, (const char**) &pNil,
				&zero, &pNil, &zero, NULL);
# endif
#elif defined(ACL_SUNOS5) || defined(ACL_FREEBSD)
			__iconv(m_iconv, (const char**) &pNil,
				&zero, &pNil, &zero);
#else
			__iconv(m_iconv, &pNil, &zero, &pNil, &zero);
#endif

			// 遇到无效的多字节序列，pIn 指向第一个无效的位置

			// 先拷贝已经转换的数据
			if ((ret = SIZE(m_pOutBuf) - nOut) > 0)
				out->append(STR(m_pOutBuf), ret);

			if (nIn == 0)
			{
				ACL_VSTRING_RESET(m_pInBuf);
				break;
			}

			acl_assert(pIn >= STR(m_pInBuf));

			// 是否跳过无效字节?
			if (m_addInvalid)
				(*out) += (char)(*pIn); // 直接拷贝无效字节
			nIn--;
			pIn++;
			if (nIn > 0)
				acl_vstring_memmove(m_pInBuf, pIn, nIn);
			else
				ACL_VSTRING_RESET(m_pInBuf);
		}
		else if (errno == EINVAL)
		{
			char *pNil = NULL;
			size_t zero = 0;

			// 重置状态, 似乎也没啥用处
#ifdef	ACL_WINDOWS
# ifdef USE_WIN_ICONV
			__iconv(m_iconv, (const char**) &pNil,
				&zero, &pNil, &zero);
# else
			__iconv(m_iconv, (const char**) &pNil,
				&zero, &pNil, &zero, NULL);
# endif // USE_WIN_ICONV
#elif defined(ACL_SUNOS5) || defined(ACL_FREEBSD)
			__iconv(m_iconv, (const char**) &pNil, &zero, &pNil, &zero);
#else
			__iconv(m_iconv, &pNil, &zero, &pNil, &zero);
#endif

			// 输入的多字节序列不完整，pIn 指向该不完整的位置

			// 先拷贝已经转换的数据
			if ((ret = SIZE(m_pOutBuf) - nOut) > 0)
				out->append(STR(m_pOutBuf), ret);

			// 移动数据，将未转换的数据移至缓冲区起始位置
			if (nIn > 0)
				acl_vstring_memmove(m_pInBuf, pIn, nIn);
			else
				ACL_VSTRING_RESET(m_pInBuf);
			break;
		}
		else if (LEN(m_pInBuf) > 0)
		{
			// 如果遇到了无效的字符集，根据设置的标志位
			// 决定是否直接拷贝
			if (m_addInvalid)
			{
				out->append(STR(m_pInBuf), LEN(m_pInBuf));
				ACL_VSTRING_RESET(m_pInBuf);
			}
			break;
		}
		else
			break;
	}

	return (true);
#else
	(void) in;
	(void) len;
	(void) out;
	logger_error("no iconv lib");
	m_errmsg = "no iconv lib";
	return (false);
#endif
}

void charset_conv::update_finish(acl::string* out)
{
#ifdef  HAVE_H_ICONV
	if (m_pInBuf && LEN(m_pInBuf) > 0 && m_addInvalid)
	{
		out->append(STR(m_pInBuf), LEN(m_pInBuf));
		ACL_VSTRING_RESET(m_pInBuf);
	}
#endif
}

int charset_conv::push_pop(const char* in, size_t len,
	acl::string* out, size_t max /* = 0 */)
{
	if (m_pBuf == NULL)
		m_pBuf = NEW acl::string(1024);

	if (in && len > 0 && update(in, len, m_pBuf) == false)
		return (-1);

	len = m_pBuf->length();
	if (len == 0)
		return (0);

	size_t n;
	if (max > 0)
		n = max > len ? len : max;
	else
		n = len;

	out->append(m_pBuf->c_str(), n);

	if (len > n)
		m_pBuf->memmove(m_pBuf->c_str() + n, len - n);
	else
		m_pBuf->clear();

	return (int) (n);
}

int charset_conv::pop_end(acl::string* out, size_t max /* = 0 */)
{
	if (m_pBuf == NULL)
	{
		logger_error("call push_pop first");
		return (-1);
	}

	update_finish(m_pBuf);

	if (out == NULL)
	{
		m_pBuf->clear();
		return (0);
	}

	size_t n = m_pBuf->length();
	if (n == 0)
		return (0);
	if (max > 0 && n > max)
		n = max;
	out->append(m_pBuf->c_str(), m_pBuf->length());
	m_pBuf->clear();
	return (int) (n);
}

void charset_conv::clear()
{
	if (m_pBuf)
		m_pBuf->clear();
}

// 获得字符集转换器
charset_conv* charset_conv::create(const char* fromCharset,
	const char* toCharset)
{
	if (fromCharset == NULL || toCharset == NULL)
		return (NULL);
	if (strcasecmp(fromCharset, toCharset) == 0)
		return (NULL);

	charset_conv* conv = NEW charset_conv();
	if (conv->update_begin(fromCharset, toCharset) == false)
	{
		delete conv;
		return (NULL);
	}
	return (conv);
}

} // namespace acl
