#include "acl_stdafx.hpp"
#include <utility>
#include <stdarg.h>
#include "acl_cpp/stdlib/string.hpp"

#define ALLOC(n) acl_vstring_alloc((n))
#define FREE(x) acl_vstring_free((x))
#define STR(x)	acl_vstring_str((x))
#define LEN(x)	ACL_VSTRING_LEN((x))
#define ADDCH(x, ch) ACL_VSTRING_ADDCH((x), (ch))
#define MCP(x, from, n) acl_vstring_memcpy((x), (from), (n))
#define MCAT(x, from, n) acl_vstring_memcat((x), (from), (n))
#define SCP(x, from) acl_vstring_strcpy((x), (from))
#define SCAT(x, from) acl_vstring_strcat((x), (from))
#define	RSET(x)	ACL_VSTRING_RESET((x))
#define TERM(x) ACL_VSTRING_TERMINATE((x))
#define AT(x, n) acl_vstring_charat((x), (n))

namespace acl {

void string::init(size_t len)
{
	if (len < 1)
		len = 1;
	m_pVbf = ALLOC(len);
	m_psList = NULL;
	m_psList2 = NULL;
	m_psPair = NULL;
	m_ptr = NULL;
}

string::string(size_t len /* = 64 */, bool bin /* = false */)
: m_bin(bin)
{
	init(len);
}

string::string(const string& s) : m_bin(false)
{
	init(s.length() + 1);
	MCP(m_pVbf, STR(s.m_pVbf), LEN(s.m_pVbf));
	TERM(m_pVbf);
}

string::string(const char* s) : m_bin(false)
{
	size_t len = strlen(s);
	init(len + 1);
	MCP(m_pVbf, s, len);
	TERM(m_pVbf);
}

string::string(const void* s, size_t n) : m_bin(false)
{
	init(n + 1);
	if (n > 0)
		MCP(m_pVbf, (const char*) s, n);
	TERM(m_pVbf);
}

string::~string()
{
	FREE(m_pVbf);
	if (m_psList)
		delete m_psList;
	if (m_psList2)
		delete m_psList2;
	if (m_psPair)
		delete m_psPair;
}

void string::set_bin(bool bin)
{
	m_bin = bin;
}

bool string::get_bin() const
{
	return (m_bin);
}

char string::operator [](size_t n)
{
	acl_assert(n < LEN(m_pVbf));
	return (AT(m_pVbf, n));
}

char string::operator [](int n)
{
	acl_assert(n < (int) LEN(m_pVbf) && n >= 0);
	return (AT(m_pVbf, n));
}

string& string::operator =(const char* s)
{
	SCP(m_pVbf, s);
	return (*this);
}

string& string::operator =(const string& s)
{
	MCP(m_pVbf, STR(s.m_pVbf), LEN(s.m_pVbf));
	TERM(m_pVbf);
	return (*this);
}

string& string::operator =(const string* s)
{
	MCP(m_pVbf, STR(s->m_pVbf), LEN(s->m_pVbf));
	TERM(m_pVbf);
	return (*this);
}

string& string::operator =(acl_int64 n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%lld", n));
}

string& string::operator =(acl_uint64 n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%llu", n));
}

string& string::operator =(long n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%ld", n));
}

string& string::operator =(unsigned long n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%lu", n));
}

string& string::operator =(int n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%d", n));
}

string& string::operator =(unsigned int n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%u", n));
}

string& string::operator =(short n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%d", n));
}

string& string::operator =(unsigned short n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%d", n));
}

string& string::operator =(char n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%c", n));
}

string& string::operator =(unsigned char n)
{
	if (m_bin)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return (format("%c", n));
}

string& string::operator +=(const char* s)
{
	SCAT(m_pVbf, s);
	return (*this);
}

string& string::operator +=(const string& s)
{
	MCAT(m_pVbf, STR(s.m_pVbf), LEN(s.m_pVbf));
	TERM(m_pVbf);
	return (*this);
}

string& string::operator +=(const string* s)
{
	MCAT(m_pVbf, STR(s->m_pVbf), LEN(s->m_pVbf));
	TERM(m_pVbf);
	return (*this);
}

string& string::operator +=(acl_int64 n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%lld", n));
}

string& string::operator +=(acl_uint64 n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%llu", n));
}

string& string::operator +=(long n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%l", n));
}

string& string::operator +=(unsigned long n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%lu", n));
}

string& string::operator +=(int n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%d", n));
}

string& string::operator +=(unsigned int n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%u", n));
}

string& string::operator +=(short n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%d", n));
}

string& string::operator +=(unsigned short n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%u", n));
}

string& string::operator +=(unsigned char n)
{
	if (m_bin)
	{
		append(&n, sizeof(n));
		return (*this);
	}
	else
		return (format_append("%c", n));
}

string& string::operator +=(char ch)
{
	*this += (unsigned char) ch;
	return (*this);
}

string& string::operator <<(const string& s)
{
	*this += s;
	return (*this);
}

string& string::operator <<(const string* s)
{
	*this += s;
	return (*this);
}

string& string::operator <<(const char* s)
{
	*this += s;
	return (*this);
}

string& string::operator <<(acl_int64 n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(acl_uint64 n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(int n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(unsigned int n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(long n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(unsigned long n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(short n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(unsigned short n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(char n)
{
	*this += n;
	return (*this);
}

string& string::operator <<(unsigned char n)
{
	*this += n;
	return (*this);
}

string& string::operator >>(string* s)
{
	*s = this;
	clear();
	return (*this);
}

string& string::push_back(char ch)
{
	return (append(&ch, sizeof(ch)));
}

char* string::buf_end()
{
	if (m_ptr == NULL)
		m_ptr = STR(m_pVbf);
	char *pEnd = acl_vstring_end(m_pVbf);
	if (m_ptr >= pEnd) {
		if (!empty())
			clear();
		return (NULL);
	}
	return (pEnd);
}

string& string::scan_buf(void* pbuf, size_t n)
{
	if (pbuf == NULL || n == 0)
		return (*this);

	const char *pEnd = buf_end();
	if (pEnd == NULL)
		return (*this);

	size_t len = pEnd - m_ptr;
	if (len > n)
		len = n;
	memcpy(pbuf, m_ptr, len);
	m_ptr += len;
	return (*this);
}

string& string::operator >>(acl_int64& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(acl_uint64& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(int& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(unsigned int& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(short& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(unsigned short& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(char& n)
{
	return (scan_buf(&n, sizeof(n)));
}

string& string::operator >>(unsigned char& n)
{
	return (scan_buf(&n, sizeof(n)));
}

bool string::operator ==(const string& s) const
{
	return (compare(s) == 0 ? true : false);
}

bool string::operator==(const string* s) const
{
	return (compare(s) == 0 ? true : false);
}

bool string::operator ==(const char* s) const
{
	return (compare(s) == 0 ? true : false);
}

bool string::operator !=(const string& s) const
{
	return (compare(s) != 0 ? true : false);
}

bool string::operator !=(const string* s) const
{
	return (compare(s) != 0 ? true : false);
}

bool string::operator !=(const char* s) const
{
	return (compare(s) != 0 ? true : false);
}

bool string::operator <(const string& s) const
{
	size_t nLeft = LEN(m_pVbf);
	size_t nRight = LEN(s.m_pVbf);
	size_t n = nLeft > nRight ? nLeft : nRight;
	int   ret = memcmp(STR(m_pVbf), STR(s.m_pVbf), n);
	if (ret < 0)
		return (true);
	else if (ret > 0)
		return (false);
	if (nLeft < nRight)
		return (true);
	return (false);
}

bool string::operator >(const string& s) const
{
	size_t nLeft = LEN(m_pVbf);
	size_t nRight = LEN(s.m_pVbf);
	size_t n = nLeft > nRight ? nLeft : nRight;
	int   ret = memcmp(STR(m_pVbf), STR(s.m_pVbf), n);
	if (ret > 0)
		return (true);
	else if (ret < 0)
		return (false);
	if (nLeft > nRight)
		return (true);
	return (false);
}

string::operator const char *() const
{
	return (STR(m_pVbf));
}

string::operator const void *() const
{
	return ((void*) STR(m_pVbf));
}

int string::compare(const string& s) const
{
	size_t n = LEN(m_pVbf) > LEN(s.m_pVbf) ? LEN(s.m_pVbf) : LEN(m_pVbf);
	int  ret;

	ret = memcmp(STR(m_pVbf), STR(s.m_pVbf), n);
	if (ret != 0)
		return (ret);
	return ((int) (LEN(m_pVbf) - LEN(s.m_pVbf)));
}

int string::compare(const string* s) const
{
	size_t n = LEN(m_pVbf) > LEN(s->m_pVbf) ? LEN(s->m_pVbf) : LEN(m_pVbf);
	int  ret;

	ret = memcmp(STR(m_pVbf), STR(s->m_pVbf), n);
	if (ret != 0)
		return (ret);
	return ((int) (LEN(m_pVbf) - LEN(s->m_pVbf)));
}

int string::compare(const char* s, bool case_sensitive) const
{
	if (case_sensitive)
		return (compare((const void*) s, (size_t) strlen(s)));		
	else
		return (acl_strcasecmp(STR(m_pVbf), s));
}

int string::compare(const void* ptr, size_t len) const
{
	size_t n = LEN(m_pVbf) > len ? len : LEN(m_pVbf);
	int  ret;

	ret = memcmp(STR(m_pVbf), ptr, n);
	if (ret != 0)
		return (ret);
	return ((int) (LEN(m_pVbf) - len));
}

int string::ncompare(const char* s, size_t len, bool case_sensitive/* =true */) const
{
	if (case_sensitive)
		return (strncmp(STR(m_pVbf), s, len));		
	else
		return (acl_strncasecmp(STR(m_pVbf), s, len));
}

int string::rncompare(const char* s, size_t len, bool case_sensitive/* =true */) const
{
	if (case_sensitive)
		return (acl_strrncmp(STR(m_pVbf), s, len));		
	else
		return (acl_strrncasecmp(STR(m_pVbf), s, len));
}

char* string::c_str() const
{
	return (STR(m_pVbf));
}

void* string::buf() const
{
	return (STR(m_pVbf));
}

size_t string::length() const
{
	return (LEN(m_pVbf));
}

size_t string::size() const
{
	return (LEN(m_pVbf));
}

size_t string::capacity() const
{
	return (ACL_VSTRING_SIZE(m_pVbf));
}

string& string::set_offset(size_t n)
{
	RSET(m_pVbf);
	ACL_VSTRING_SPACE(m_pVbf, (int) n);
	ACL_VSTRING_AT_OFFSET(m_pVbf, (int) n);
	TERM(m_pVbf);
	return (*this);
}

string& string::space(size_t n)
{
	ACL_VSTRING_SPACE(m_pVbf, (int) n);
	return (*this);
}

bool string::empty() const
{
	return (LEN(m_pVbf) > 0 ? false : true);
}

ACL_VSTRING* string::vstring() const
{
	return (m_pVbf);
}

int string::find(char ch) const
{
	char *ptr = acl_vstring_memchr(m_pVbf, ch);
	if (ptr == NULL)
		return (-1);
	return ((int)(ptr - STR(m_pVbf)));
}

const char* string::find(const char* needle, bool case_sensitive) const
{
	if (case_sensitive)
		return (acl_vstring_strstr(m_pVbf, needle));
	else
		return (acl_vstring_strcasestr(m_pVbf, needle));
}

const char* string::rfind(const char* needle, bool case_sensitive) const
{
	if (case_sensitive)
		return (acl_vstring_rstrstr(m_pVbf, needle));
	else
		return (acl_vstring_rstrcasestr(m_pVbf, needle));
}

const string string::left(size_t npos)
{
	if (npos >= LEN(m_pVbf))
		return (*this);
	return (string(STR(m_pVbf), npos));
}

const string string::right(size_t npos)
{
	npos++;
	if (npos >= LEN(m_pVbf))
		return string(1);
	size_t nLeft = LEN(m_pVbf) - npos;
	return (string(STR(m_pVbf) + npos, nLeft));
}

const std::list<acl::string>& string::split(const char* sep)
{
	ACL_ARGV *argv = acl_argv_split(STR(m_pVbf), sep);
	ACL_ITER it;

	if (m_psList == NULL)
		m_psList = NEW std::list<acl::string>;
	else
		m_psList->clear();
	acl_foreach(it, argv)
	{
		char* ptr = (char*) it.data;
		m_psList->push_back(ptr);
	}
	acl_argv_free(argv);
	return (*m_psList);
}

const std::vector<acl::string>& string::split2(const char* sep)
{
	ACL_ARGV *argv = acl_argv_split(STR(m_pVbf), sep);
	ACL_ITER it;

	if (m_psList2 == NULL)
		m_psList2 = NEW std::vector<acl::string>;
	else
		m_psList2->clear();
	acl_foreach(it, argv)
	{
		char* ptr = (char*) it.data;
		m_psList2->push_back(ptr);
	}
	acl_argv_free(argv);
	return (*m_psList2);
}

const std::pair<acl::string, acl::string>& string::split_nameval()
{
	char *name, *value;

	if (m_psPair == NULL)
		m_psPair = NEW std::pair<acl::string, acl::string>;

	if (acl_split_nameval(STR(m_pVbf), &name, &value) != NULL) {
		m_psPair->first = "";
		m_psPair->second = "";
		return (*m_psPair);
	}
	m_psPair->first = name;
	m_psPair->second = value;
	return (*m_psPair);
}

string& string::copy(const char* ptr)
{
	SCP(m_pVbf, ptr);
	return (*this);
}

string& string::copy(const void* ptr, size_t len)
{
	MCP(m_pVbf, (const char*) ptr, len);
	TERM(m_pVbf);
	return (*this);
}

string& string::memmove(const char* ptr)
{
	return (memmove(ptr, strlen(ptr)));
}

string& string::memmove(const char* ptr, size_t len)
{
	acl_vstring_memmove(m_pVbf, ptr, len);
	return (*this);
}

string& string::append(const string& s)
{
	return (append(s.c_str(), s.length()));
}

string& string::append(const string* s)
{
	return (append(s->c_str(), s->length()));
}

string& string::append(const char* ptr)
{
	SCAT(m_pVbf, ptr);
	return (*this);
}

string& string::append(const void* ptr, size_t len)
{
	MCAT(m_pVbf, (const char*) ptr, len);
	TERM(m_pVbf);
	return (*this);
}

string& string::prepend(const char* s)
{
	acl_vstring_prepend(m_pVbf, s, strlen(s));
	return (*this);
}

string& string::prepend(const void* ptr, size_t len)
{
	acl_vstring_prepend(m_pVbf, (const char*) ptr, len);
	return (*this);
}


string& string::insert(size_t start, const void* ptr, size_t len)
{
	acl_vstring_insert(m_pVbf, start, (const char*) ptr, len);
	return (*this);
}

string& string::format(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vformat(fmt, ap);
	va_end(ap);
	return (*this);
}

string& string::vformat(const char* fmt, va_list ap)
{
	acl_vstring_vsprintf(m_pVbf, fmt, ap);
	return (*this);
}

string& string::format_append(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vformat_append(fmt, ap);
	va_end(ap);
	return (*this);
}

string& string::vformat_append(const char* fmt, va_list ap)
{
	acl_vstring_vsprintf_append(m_pVbf, fmt, ap);
	return (*this);
}

string& string::replace(char from, char to)
{
	char* ptr = STR(m_pVbf);
	size_t i, n = LEN(m_pVbf);

	for (i = 0; i < n; i++) {
		if (*ptr == from)
			*ptr = to;
		ptr++;
	}

	return (*this);
}

string& string::truncate(size_t n)
{
	acl_vstring_truncate(m_pVbf, n);
	return (*this);
}

string& string::strip(const char* needle, bool each /* false */)
{
	char* src = STR(m_pVbf);
	char* ptr;
	ACL_VSTRING* pVbf = NULL;

	if (each)
	{
		const char* last;

		while (true)
		{
			last = src;
			if ((ptr = acl_mystrtok(&src, needle)) == NULL)
			{
				if (*last == 0)
					break;
				if (pVbf != NULL)
					SCAT(pVbf, last);
				break;
			}
			// 写时分配
			if (pVbf == NULL)
				pVbf = acl_vstring_alloc(LEN(m_pVbf));
			SCAT(pVbf, ptr);
		}
		
		if (pVbf != NULL)
		{
			FREE(m_pVbf);
			m_pVbf = pVbf;
		}
		return (*this);
	}

	size_t len = strlen(needle), n;

	while (true)
	{
		ptr = strstr(src, needle);
		if (ptr == NULL)
		{
			if (pVbf != NULL)
				SCAT(pVbf, src);
			break;
		}

		// 采用写时延迟分配策略
		if (pVbf == NULL)
			pVbf = acl_vstring_alloc(LEN(m_pVbf));
		n = ptr - src;
		MCP(pVbf, src, n);
		TERM(pVbf);
		src += n + len;
	}

	if (pVbf != NULL)
	{
		FREE(m_pVbf);
		m_pVbf = pVbf;
	}
	return (*this);
}

string& string::clear(void)
{
	RSET(m_pVbf);
	TERM(m_pVbf);
	m_ptr = NULL;
	return (*this);
}

string& string::lower()
{
	acl_lowercase(STR(m_pVbf));
	return (*this);
}

string& string::upper()
{
	acl_uppercase(STR(m_pVbf));
	return (*this);
}

string& string::base64_encode(void)
{
	size_t dlen = length();
	if (dlen == 0)
		return (*this);
	size_t n = (dlen * 4) / 3;
	ACL_VSTRING *s = ALLOC(n) ;
	acl_vstring_base64_encode(s, c_str(), (int) dlen);
	FREE(m_pVbf);
	m_pVbf = s;
	return (*this);
}

string& string::base64_encode(const void* ptr, size_t len)
{
	acl_vstring_base64_encode(m_pVbf,
		(const char*) ptr, (int) len);
	return (*this);
}

string& string::base64_decode(void)
{
	size_t dlen = length();
	if (dlen == 0)
		return (*this);
	size_t n = (dlen * 3) / 4;
	ACL_VSTRING *s = ALLOC(n) ;
	if (acl_vstring_base64_decode(s, c_str(), (int) dlen) == NULL)
		RSET(s);
	FREE(m_pVbf);
	m_pVbf = s;
	return (*this);
}

string& string::base64_decode(const char* s)
{
	if (acl_vstring_base64_decode(m_pVbf,
		s, (int) strlen(s)) == NULL)
	{
		RSET(m_pVbf);
	}
	return (*this);
}

string& string::base64_decode(const void* ptr, size_t len)
{
	if (acl_vstring_base64_decode(m_pVbf,
		(const char*) ptr, (int) len) == NULL)
	{
		RSET(m_pVbf);
	}
	return (*this);
}

string& string::url_encode(const char* s)
{
	char *ptr = acl_url_encode(s);
	(*this) = ptr;
	acl_myfree(ptr);
	return (*this);
}

string& string::url_decode(const char* s)
{
	char *ptr = acl_url_decode(s);

	(*this) = ptr;
	acl_myfree(ptr);
	return (*this);
}

string& string::hex_encode(const void* s, size_t len)
{
	(void) acl_hex_encode(m_pVbf, (const char*) s, (int) len);
	return (*this);
}

string& string::hex_decode(const char* s, size_t len)
{
	(void) acl_hex_decode(m_pVbf, s, (int) len);
	return (*this);
}

static void dummy_free(void*)
{
}

static void buf_free(void* arg)
{
	string* s = (string*) arg;
	delete s;
}

static string* __main_buf = NULL;

static void main_buf_free(void)
{
	if (__main_buf)
		delete __main_buf;
}

static acl_pthread_key_t __buf_key;

static void prepare_once(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
	{
		acl_pthread_key_create(&__buf_key, dummy_free);
		atexit(main_buf_free);
	} else
		acl_pthread_key_create(&__buf_key, buf_free);
}

static acl_pthread_once_t __buf_once = ACL_PTHREAD_ONCE_INIT;

static string& get_buf(void)
{
	acl_pthread_once(&__buf_once, prepare_once);
	string* s = (string*) acl_pthread_getspecific(__buf_key);
	if (s == NULL)
	{
		s = NEW string(21);
		acl_pthread_setspecific(__buf_key, s);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__main_buf = s;
	}
	return *s;
}

const string& string::parse_int(int n)
{
	string& s = get_buf();
	s.format("%d", n);
	return (s);
}

const string& string::parse_int(unsigned int n)
{
	string& s = get_buf();
	s.format("%u", n);
	return (s);
}

const string& string::parse_int64(acl_int64 n)
{
	string& s = get_buf();
	s.format("%lld", n);
	return (s);
}

const string& string::parse_int64(acl_uint64 n)
{
	string& s = get_buf();
	s.format("%llu", n);
	return (s);
}

} // namespace acl
