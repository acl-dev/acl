#include "acl_stdafx.hpp"
#include <utility>
#include <stdarg.h>
#include "acl_cpp/stdlib/string.hpp"

#define ALLOC(n) acl_vstring_alloc((n))
#define FREE(x) acl_vstring_free((x))
#define STR(x)	acl_vstring_str((x))
#define LEN(x)	ACL_VSTRING_LEN((x))
#define	CAP(x)	ACL_VSTRING_SIZE((x))
#define ADDCH(x, ch) ACL_VSTRING_ADDCH((x), (ch))
#define MCP(x, from, n) acl_vstring_memcpy((x), (from), (n))
#define MCAT(x, from, n) acl_vstring_memcat((x), (from), (n))
#define SCP(x, from) acl_vstring_strcpy((x), (from))
#define SCAT(x, from) acl_vstring_strcat((x), (from))
#define	RSET(x)	ACL_VSTRING_RESET((x))
#define TERM(x) ACL_VSTRING_TERMINATE((x))
#define AT(x, n) acl_vstring_charat((x), (n))
#define	END(x) acl_vstring_end((x))

namespace acl {

void string::init(size_t len)
{
	if (len < 1)
		len = 1;
	vbf_ = ALLOC(len);
	list_tmp_ = NULL;
	vector_tmp_ = NULL;
	pair_tmp_ = NULL;
	scan_ptr_ = NULL;
}

string::string(size_t len /* = 64 */, bool bin /* = false */)
: use_bin_(bin)
{
	init(len);
}

string::string(const string& s) : use_bin_(false)
{
	init(s.length() + 1);
	MCP(vbf_, STR(s.vbf_), LEN(s.vbf_));
	TERM(vbf_);
}

string::string(const char* s) : use_bin_(false)
{
	size_t len = strlen(s);
	init(len + 1);
	MCP(vbf_, s, len);
	TERM(vbf_);
}

string::string(const void* s, size_t n) : use_bin_(false)
{
	init(n + 1);
	if (n > 0)
		MCP(vbf_, (const char*) s, n);
	TERM(vbf_);
}

string::~string()
{
	FREE(vbf_);
	delete list_tmp_;
	delete vector_tmp_;
	delete pair_tmp_;
}

string& string::set_bin(bool bin)
{
	use_bin_ = bin;
	return *this;
}

bool string::get_bin() const
{
	return use_bin_;
}

string& string::set_max(int max)
{
	vbf_->maxlen = max;
	return *this;
}

int string::get_max(void) const
{
	return vbf_->maxlen;
}

char string::operator [](size_t n) const
{
	acl_assert(n < LEN(vbf_));
	return AT(vbf_, n);
}

char string::operator [](int n) const
{
	acl_assert(n < (int) LEN(vbf_) && n >= 0);
	return AT(vbf_, n);
}

char& string::operator [](size_t n)
{
	// 当偏移位置大于所分配空间的最大位置，需要重新分配内存
	if (n >= (size_t) CAP(vbf_))
	{
		int  len = CAP(vbf_);
		space(n + 1);
		int  new_len = CAP(vbf_);

		// 初始化新分配的内存
		if (new_len > len)
			memset(END(vbf_), 0, new_len - len);

		// 将 vbf_->vbuf.ptr 指向 n 字节处，同时修改 vbf_->vbuf.cnt 值
		ACL_VSTRING_AT_OFFSET(vbf_, n);
	}
	// 当偏移位置大于当前数据长度时，通过重置指针位置以修改数据长度，
	// 这样当调用 length() 方法时可以获得长度
	else if (n >= LEN(vbf_))
	{
		ACL_VSTRING_AT_OFFSET(vbf_, n);
		// 因为本函数返回了偏移位置 n 处的地址引用后，调用者对此引用
		// 地址赋值，但并不会使缓冲区数据长度增加，所以此处在函数返回
		// 前调用 ADDCH 相当于调用者外部赋值后将数据长度加 1
		ADDCH(vbf_, '\0');
		// 保证最后一个字节为 \0
		TERM(vbf_);
	}

	return (char&) (vbf_->vbuf.data[n]);
}

char& string::operator [](int n)
{
	acl_assert(n >= 0);
#if 1
	return (*this)[(size_t) n];
#else
	if (n >= CAP(vbf_))
	{
		int  len = CAP(vbf_);
		printf("%d: cap1: %d\n", __LINE__, CAP(vbf_));
		space(n + 1);
		printf("%d: cap2: %d\n", __LINE__, CAP(vbf_));
		int  new_len = CAP(vbf_);

		// 初始化新分配的内存
		if (new_len > len)
			memset(END(vbf_), 0, new_len - len);

		// 将 vbf_->vbuf.ptr 指向 n 字节处，同时修改 vbf_->vbuf.cnt 值
		ACL_VSTRING_AT_OFFSET(vbf_, n);
	}
	else if (n >= (int) LEN(vbf_))
	{
		ACL_VSTRING_AT_OFFSET(vbf_, n);
		ADDCH(vbf_, '\0');
		TERM(vbf_);
	}

	return (char&) (vbf_->vbuf.data[n]);
#endif
}

string& string::operator =(const char* s)
{
	SCP(vbf_, s);
	return *this;
}

string& string::operator =(const string& s)
{
	MCP(vbf_, STR(s.vbf_), LEN(s.vbf_));
	TERM(vbf_);
	return *this;
}

string& string::operator =(const string* s)
{
	MCP(vbf_, STR(s->vbf_), LEN(s->vbf_));
	TERM(vbf_);
	return *this;
}

string& string::operator =(acl_int64 n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%lld", n);
}

string& string::operator =(acl_uint64 n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%llu", n);
}

string& string::operator =(long n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%ld", n);
}

string& string::operator =(unsigned long n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%lu", n);
}

string& string::operator =(int n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%d", n);
}

string& string::operator =(unsigned int n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%u", n);
}

string& string::operator =(short n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%d", n);
}

string& string::operator =(unsigned short n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%d", n);
}

string& string::operator =(char n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return *this;
	}
	else
		return format("%c", n);
}

string& string::operator =(unsigned char n)
{
	if (use_bin_)
	{
		copy(&n, sizeof(n));
		return (*this);
	}
	else
		return format("%c", n);
}

string& string::operator +=(const char* s)
{
	SCAT(vbf_, s);
	return *this;
}

string& string::operator +=(const string& s)
{
	MCAT(vbf_, STR(s.vbf_), LEN(s.vbf_));
	TERM(vbf_);
	return *this;
}

string& string::operator +=(const string* s)
{
	MCAT(vbf_, STR(s->vbf_), LEN(s->vbf_));
	TERM(vbf_);
	return *this;
}

string& string::operator +=(acl_int64 n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%lld", n);
}

string& string::operator +=(acl_uint64 n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%llu", n);
}

string& string::operator +=(long n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%ld", n);
}

string& string::operator +=(unsigned long n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%lu", n);
}

string& string::operator +=(int n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%d", n);
}

string& string::operator +=(unsigned int n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%u", n);
}

string& string::operator +=(short n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%d", n);
}

string& string::operator +=(unsigned short n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%u", n);
}

string& string::operator +=(unsigned char n)
{
	if (use_bin_)
	{
		append(&n, sizeof(n));
		return *this;
	}
	else
		return format_append("%c", n);
}

string& string::operator +=(char ch)
{
	*this += (unsigned char) ch;
	return *this;
}

string& string::operator <<(const string& s)
{
	*this += s;
	return *this;
}

string& string::operator <<(const string* s)
{
	*this += s;
	return *this;
}

string& string::operator <<(const char* s)
{
	*this += s;
	return *this;
}

string& string::operator <<(acl_int64 n)
{
	*this += n;
	return *this;
}

string& string::operator <<(acl_uint64 n)
{
	*this += n;
	return *this;
}

string& string::operator <<(int n)
{
	*this += n;
	return *this;
}

string& string::operator <<(unsigned int n)
{
	*this += n;
	return *this;
}

string& string::operator <<(long n)
{
	*this += n;
	return *this;
}

string& string::operator <<(unsigned long n)
{
	*this += n;
	return *this;
}

string& string::operator <<(short n)
{
	*this += n;
	return *this;
}

string& string::operator <<(unsigned short n)
{
	*this += n;
	return *this;
}

string& string::operator <<(char n)
{
	*this += n;
	return *this;
}

string& string::operator <<(unsigned char n)
{
	*this += n;
	return *this;
}

string& string::push_back(char ch)
{
	return append(&ch, sizeof(ch));
}

char* string::buf_end()
{
	if (scan_ptr_ == NULL)
		scan_ptr_ = STR(vbf_);
	char *pEnd = acl_vstring_end(vbf_);
	if (scan_ptr_ >= pEnd)
	{
		if (!empty())
			clear();
		return NULL;
	}
	return pEnd;
}

bool string::scan_line(string& out, bool nonl /* = true */,
	size_t* n /* = NULL */, bool move /* = false */)
{
	if (n)
		*n = 0;

	char* pEnd = buf_end();
	if (pEnd == NULL)
		return false;
	
	size_t len = pEnd - scan_ptr_;
	char *ln = (char*) memchr(scan_ptr_, '\n', len);
	if (ln == NULL)
		return false;

	char *next = ln + 1;
	len = ln - scan_ptr_ + 1;

	if (nonl)
	{
		ln--;
		len--;
		if (ln >= scan_ptr_ && *ln == '\r')
		{
			ln--;
			len--;
		}
		if (len > 0)
			out.append(scan_ptr_, len);
	}
	else
		out.append(scan_ptr_, len);

	if (move)
	{
		if (pEnd > next)
		{
			acl_vstring_memmove(vbf_, next, pEnd - next);
			TERM(vbf_);
			scan_ptr_ = STR(vbf_);
		}
		else
			clear();
	}
	else
	{
		if (next >= pEnd)
			clear();
		else
			scan_ptr_ = next;
	}

	if (n)
		*n = len;

	return true;
}

size_t string::scan_buf(void* pbuf, size_t n, bool move /* = false */)
{
	if (pbuf == NULL || n == 0)
		return 0;

	const char *pEnd = buf_end();
	if (pEnd == NULL)
		return 0;

	size_t len = pEnd - scan_ptr_;
	if (len > n)
		len = n;
	memcpy(pbuf, scan_ptr_, len);
	if (move)
	{
		acl_vstring_memmove(vbf_, scan_ptr_, len);
		TERM(vbf_);
		scan_ptr_ = STR(vbf_);
	}
	else
		scan_ptr_ += len;
	return len;
}

size_t string::scan_move()
{
	if (scan_ptr_ == NULL)
		return 0;

	char *pEnd = acl_vstring_end(vbf_);
	if (scan_ptr_ >= pEnd)
	{
		clear();
		return 0;
	}

	size_t len = pEnd - scan_ptr_;
	acl_vstring_memmove(vbf_, scan_ptr_, len);
	TERM(vbf_);
	scan_ptr_  = STR(vbf_);

	return len;
}

size_t string::operator >>(string* s)
{
	size_t len = this->length();
	*s = this;
	clear();
	return len;
}

size_t string::operator >>(acl_int64& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(acl_uint64& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(int& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(unsigned int& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(short& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(unsigned short& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(char& n)
{
	return scan_buf(&n, sizeof(n));
}

size_t string::operator >>(unsigned char& n)
{
	return scan_buf(&n, sizeof(n));
}

bool string::operator ==(const string& s) const
{
	return compare(s) == 0 ? true : false;
}

bool string::operator==(const string* s) const
{
	return compare(s) == 0 ? true : false;
}

bool string::operator ==(const char* s) const
{
	return compare(s) == 0 ? true : false;
}

bool string::operator !=(const string& s) const
{
	return compare(s) != 0 ? true : false;
}

bool string::operator !=(const string* s) const
{
	return compare(s) != 0 ? true : false;
}

bool string::operator !=(const char* s) const
{
	return compare(s) != 0 ? true : false;
}

bool string::operator <(const string& s) const
{
	size_t nLeft = LEN(vbf_);
	size_t nRight = LEN(s.vbf_);
	size_t n = nLeft > nRight ? nLeft : nRight;
	int   ret = memcmp(STR(vbf_), STR(s.vbf_), n);
	if (ret < 0)
		return true;
	else if (ret > 0)
		return false;
	if (nLeft < nRight)
		return true;
	return false;
}

bool string::operator >(const string& s) const
{
	size_t nLeft = LEN(vbf_);
	size_t nRight = LEN(s.vbf_);
	size_t n = nLeft > nRight ? nLeft : nRight;
	int   ret = memcmp(STR(vbf_), STR(s.vbf_), n);
	if (ret > 0)
		return true;
	else if (ret < 0)
		return false;
	if (nLeft > nRight)
		return true;
	return false;
}

string::operator const char *() const
{
	return STR(vbf_);
}

string::operator const void *() const
{
	return (void*) STR(vbf_);
}

int string::compare(const string& s) const
{
	size_t n = LEN(vbf_) > LEN(s.vbf_) ? LEN(s.vbf_) : LEN(vbf_);
	int  ret;

	ret = memcmp(STR(vbf_), STR(s.vbf_), n);
	if (ret != 0)
		return ret;
	return (int) (LEN(vbf_) - LEN(s.vbf_));
}

int string::compare(const string* s) const
{
	size_t n = LEN(vbf_) > LEN(s->vbf_) ? LEN(s->vbf_) : LEN(vbf_);
	int  ret;

	ret = memcmp(STR(vbf_), STR(s->vbf_), n);
	if (ret != 0)
		return ret;
	return (int) (LEN(vbf_) - LEN(s->vbf_));
}

int string::compare(const char* s, bool case_sensitive) const
{
	if (case_sensitive)
		return compare((const void*) s, (size_t) strlen(s));		
	else
		return acl_strcasecmp(STR(vbf_), s);
}

int string::compare(const void* ptr, size_t len) const
{
	size_t n = LEN(vbf_) > len ? len : LEN(vbf_);
	int  ret;

	ret = memcmp(STR(vbf_), ptr, n);
	if (ret != 0)
		return ret;
	return (int) (LEN(vbf_) - len);
}

int string::ncompare(const char* s, size_t len, bool case_sensitive/* =true */) const
{
	if (case_sensitive)
		return strncmp(STR(vbf_), s, len);		
	else
		return acl_strncasecmp(STR(vbf_), s, len);
}

int string::rncompare(const char* s, size_t len, bool case_sensitive/* =true */) const
{
	if (case_sensitive)
		return acl_strrncmp(STR(vbf_), s, len);		
	else
		return acl_strrncasecmp(STR(vbf_), s, len);
}

char* string::c_str() const
{
	if (scan_ptr_)
		return scan_ptr_;
	return STR(vbf_);
}

void* string::buf() const
{
	return STR(vbf_);
}

size_t string::length() const
{
	if (scan_ptr_)
		return LEN(vbf_) - (scan_ptr_ - STR(vbf_));
	return LEN(vbf_);
}

size_t string::size() const
{
	return length();
}

size_t string::capacity() const
{
	return CAP(vbf_);
}

string& string::set_offset(size_t n)
{
	RSET(vbf_);
	ACL_VSTRING_SPACE(vbf_, (int) n);
	ACL_VSTRING_AT_OFFSET(vbf_, (int) n);
	TERM(vbf_);
	return *this;
}

string& string::space(size_t n)
{
	ACL_VSTRING_SPACE(vbf_, (int) n);
	return *this;
}

bool string::empty() const
{
	return LEN(vbf_) > 0 ? false : true;
}

ACL_VSTRING* string::vstring() const
{
	return vbf_;
}

int string::find(char ch) const
{
	char *ptr = acl_vstring_memchr(vbf_, ch);
	if (ptr == NULL)
		return -1;
	return (int)(ptr - STR(vbf_));
}

char* string::find(const char* needle, bool case_sensitive) const
{
	if (case_sensitive)
		return acl_vstring_strstr(vbf_, needle);
	else
		return acl_vstring_strcasestr(vbf_, needle);
}

char* string::rfind(const char* needle, bool case_sensitive) const
{
	if (case_sensitive)
		return acl_vstring_rstrstr(vbf_, needle);
	else
		return acl_vstring_rstrcasestr(vbf_, needle);
}

string string::left(size_t npos)
{
	if (npos >= LEN(vbf_))
		return *this;
	return string(STR(vbf_), npos);
}

string string::right(size_t npos)
{
	npos++;
	if (npos >= LEN(vbf_))
		return string(1);
	size_t nLeft = LEN(vbf_) - npos;
	return string(STR(vbf_) + npos, nLeft);
}

std::list<acl::string>& string::split(const char* sep)
{
	ACL_ARGV *argv = acl_argv_split(STR(vbf_), sep);
	ACL_ITER it;

	if (list_tmp_ == NULL)
		list_tmp_ = NEW std::list<acl::string>;
	else
		list_tmp_->clear();
	acl_foreach(it, argv)
	{
		char* ptr = (char*) it.data;
		list_tmp_->push_back(ptr);
	}
	acl_argv_free(argv);
	return *list_tmp_;
}

std::vector<acl::string>& string::split2(const char* sep)
{
	ACL_ARGV *argv = acl_argv_split(STR(vbf_), sep);
	ACL_ITER it;

	if (vector_tmp_ == NULL)
		vector_tmp_ = NEW std::vector<acl::string>;
	else
		vector_tmp_->clear();
	acl_foreach(it, argv)
	{
		char* ptr = (char*) it.data;
		vector_tmp_->push_back(ptr);
	}
	acl_argv_free(argv);
	return *vector_tmp_;
}

std::pair<acl::string, acl::string>& string::split_nameval()
{
	char *name, *value;

	if (pair_tmp_ == NULL)
		pair_tmp_ = NEW std::pair<acl::string, acl::string>;

	if (acl_split_nameval(STR(vbf_), &name, &value) != NULL) {
		pair_tmp_->first = "";
		pair_tmp_->second = "";
		return *pair_tmp_;
	}
	pair_tmp_->first = name;
	pair_tmp_->second = value;
	return *pair_tmp_;
}

string& string::copy(const char* ptr)
{
	SCP(vbf_, ptr);
	return *this;
}

string& string::copy(const void* ptr, size_t len)
{
	MCP(vbf_, (const char*) ptr, len);
	TERM(vbf_);
	return *this;
}

string& string::memmove(const char* ptr)
{
	return memmove(ptr, strlen(ptr));
}

string& string::memmove(const char* ptr, size_t len)
{
	acl_vstring_memmove(vbf_, ptr, len);
	return *this;
}

string& string::append(const string& s)
{
	return append(s.c_str(), s.length());
}

string& string::append(const string* s)
{
	return append(s->c_str(), s->length());
}

string& string::append(const char* ptr)
{
	SCAT(vbf_, ptr);
	return *this;
}

string& string::append(const void* ptr, size_t len)
{
	MCAT(vbf_, (const char*) ptr, len);
	TERM(vbf_);
	return *this;
}

string& string::prepend(const char* s)
{
	acl_vstring_prepend(vbf_, s, strlen(s));
	return *this;
}

string& string::prepend(const void* ptr, size_t len)
{
	acl_vstring_prepend(vbf_, (const char*) ptr, len);
	return *this;
}


string& string::insert(size_t start, const void* ptr, size_t len)
{
	acl_vstring_insert(vbf_, start, (const char*) ptr, len);
	return *this;
}

string& string::format(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vformat(fmt, ap);
	va_end(ap);
	return *this;
}

string& string::vformat(const char* fmt, va_list ap)
{
	acl_vstring_vsprintf(vbf_, fmt, ap);
	return *this;
}

string& string::format_append(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vformat_append(fmt, ap);
	va_end(ap);
	return *this;
}

string& string::vformat_append(const char* fmt, va_list ap)
{
	acl_vstring_vsprintf_append(vbf_, fmt, ap);
	return *this;
}

string& string::replace(char from, char to)
{
	char* ptr = STR(vbf_);
	size_t i, n = LEN(vbf_);

	for (i = 0; i < n; i++) {
		if (*ptr == from)
			*ptr = to;
		ptr++;
	}

	return *this;
}

string& string::truncate(size_t n)
{
	acl_vstring_truncate(vbf_, n);
	TERM(vbf_);
	return *this;
}

string& string::strip(const char* needle, bool each /* false */)
{
	char* src = STR(vbf_);
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
				pVbf = acl_vstring_alloc(LEN(vbf_));
			SCAT(pVbf, ptr);
		}
		
		if (pVbf != NULL)
		{
			FREE(vbf_);
			vbf_ = pVbf;
		}
		return *this;
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
			pVbf = acl_vstring_alloc(LEN(vbf_));
		n = ptr - src;
		MCP(pVbf, src, n);
		TERM(pVbf);
		src += n + len;
	}

	if (pVbf != NULL)
	{
		FREE(vbf_);
		vbf_ = pVbf;
	}
	return *this;
}

string& string::trim_left_space()
{
	char* pBegin = STR(vbf_);
	char* pEnd = acl_vstring_end(vbf_);
	if (pEnd == pBegin)
		return *this;
	
	size_t n = 0;
	while (pBegin < pEnd && (*pBegin == ' ' || *pBegin == '\t'))
	{
		pBegin++;
		n++;
	}
	if (pBegin == pEnd)
		clear();
	else if (n > 0)
	{
		acl_vstring_memmove(vbf_, pBegin, LEN(vbf_) - n);
		TERM(vbf_);
	}
	return *this;
}

string& string::trim_right_space()
{
	char* pBegin = STR(vbf_);
	char* pEnd = acl_vstring_end(vbf_);
	if (pEnd == pBegin)
		return *this;
	pEnd--;
	size_t n = 0;
	while (pEnd >= pBegin && (*pEnd == ' ' || *pEnd == '\t'))
	{
		pEnd--;
		n++;
	}

	size_t len = LEN(vbf_);
	if (n > 0)
	{
		len -= n;
		truncate(len);
	}
	return *this;
}

string& string::trim_space()
{
	return strip(" \t", true);
}

string& string::trim_left_line()
{
	char* pBegin = STR(vbf_);
	char* pEnd = acl_vstring_end(vbf_);
	if (pEnd == pBegin)
		return *this;

	size_t n = 0;
	while (pBegin < pEnd)
	{
		if (*pBegin == '\n')
		{
			pBegin++;
			n++;
		}
		else if (*pBegin == '\r' && (pBegin + 1) < pEnd
			&& *(pBegin + 1) == '\n')
		{
			pBegin += 2;
			n += 2;
		}
		else
			break;
	}
	if (pBegin == pEnd)
		clear();
	else if (n > 0)
	{
		acl_vstring_memmove(vbf_, pBegin, LEN(vbf_) - n);
		TERM(vbf_);
	}
	return *this;
}

string& string::trim_right_line()
{
	char* pBegin = STR(vbf_);
	char* pEnd = acl_vstring_end(vbf_);
	if (pEnd == pBegin)
		return *this;
	pEnd--;
	size_t n = 0;
	while (pEnd >= pBegin && *pEnd == '\n')
	{
		pEnd--;
		n++;
		if (pEnd >= pBegin && *pEnd == '\r')
		{
			pEnd--;
			n++;
		}
	}

	size_t len = LEN(vbf_);
	if (n > 0)
	{
		len -= n;
		truncate(len);
	}
	return *this;
}

string& string::trim_line()
{
	return strip("\r\n", true);
}

string& string::clear(void)
{
	RSET(vbf_);
	TERM(vbf_);
	scan_ptr_ = NULL;
	return *this;
}

string& string::lower()
{
	acl_lowercase(STR(vbf_));
	return *this;
}

string& string::upper()
{
	acl_uppercase(STR(vbf_));
	return *this;
}

string& string::base64_encode(void)
{
	size_t dlen = length();
	if (dlen == 0)
		return *this;
	size_t n = (dlen * 4) / 3;
	ACL_VSTRING *s = ALLOC(n) ;
	acl_vstring_base64_encode(s, c_str(), (int) dlen);
	FREE(vbf_);
	vbf_ = s;
	return *this;
}

string& string::base64_encode(const void* ptr, size_t len)
{
	acl_vstring_base64_encode(vbf_, (const char*) ptr, (int) len);
	return *this;
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
	FREE(vbf_);
	vbf_ = s;
	return *this;
}

string& string::base64_decode(const char* s)
{
	if (acl_vstring_base64_decode(vbf_,
		s, (int) strlen(s)) == NULL)
	{
		RSET(vbf_);
	}
	return *this;
}

string& string::base64_decode(const void* ptr, size_t len)
{
	if (acl_vstring_base64_decode(vbf_,
		(const char*) ptr, (int) len) == NULL)
	{
		RSET(vbf_);
	}
	return *this;
}

string& string::url_encode(const char* s)
{
	char *ptr = acl_url_encode(s);
	(*this) = ptr;
	acl_myfree(ptr);
	return *this;
}

string& string::url_decode(const char* s)
{
	char *ptr = acl_url_decode(s);

	(*this) = ptr;
	acl_myfree(ptr);
	return *this;
}

string& string::hex_encode(const void* s, size_t len)
{
	(void) acl_hex_encode(vbf_, (const char*) s, (int) len);
	return *this;
}

string& string::hex_decode(const char* s, size_t len)
{
	(void) acl_hex_decode(vbf_, s, (int) len);
	return *this;
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
	if (s != NULL)
		return *s;

	s = NEW string(21);
	acl_pthread_setspecific(__buf_key, s);
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
		__main_buf = s;
	return *s;
}

string& string::parse_int(int n)
{
	string& s = get_buf();
	s.format("%d", n);
	return s;
}

string& string::parse_int(unsigned int n)
{
	string& s = get_buf();
	s.format("%u", n);
	return s;
}

string& string::parse_int64(acl_int64 n)
{
	string& s = get_buf();
	s.format("%lld", n);
	return s;
}

string& string::parse_int64(acl_uint64 n)
{
	string& s = get_buf();
	s.format("%llu", n);
	return s;
}

} // namespace acl
