#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <time.h>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

/**
 * Email address
 */
struct rfc822_addr {
	char* addr;	// Email address, format: xxx@xxx.xxx
	char* comment;	// Email comment
};

typedef enum {
	tzone_gmt,
	tzone_cst
} tzone_t;

class ACL_CPP_API rfc822 : public noncopyable {
public:
	rfc822();
	~rfc822();

	/**
	 * Parse time format conforming to RFC822 standard
	 * @param in {const char*} Time string, e.g.:
	 *  Wed, 11 May 2011 09:44:37 +0800 (CST)
	 *  Wed, 11 May 2011 16:17:39 GMT
	 */
	time_t parse_date(const char *in);

	/**
	 * Generate time format conforming to RFC822 standard
	 * @param t {time_t}
	 * @param out {char*} Store conversion result
	 * @param size {size_t} out space size
	 * @param zone {tzone_t} Time zone
	 */
	void mkdate(time_t t, char* out, size_t size, tzone_t  zone = tzone_cst);

	/**
	 * Generate time format for UTC+8 time zone
	 * @param t {time_t}
	 * @param out {char*} Store conversion result
	 * @param size {size_t} out space size
	 */
	void mkdate_cst(time_t t, char* out, size_t size);

	/**
	 * Generate time format for Greenwich Mean Time
	 * @param t {time_t}
	 * @param out {char*} Store conversion result
	 * @param size {size_t} out space size
	 */
	void mkdate_gmt(time_t t, char* out, size_t size);

	/**
	 * Parse email address list. Parse email address list conforming to RFC822 standard into
	 * email address list that humans can normally read, and decode username comment part according to
	 * RFC2047
	 * @param in {const char*} Email address list in RFC822 format, e.g.:
	 *  "=?gb2312?B?1dSx+A==?= <zhaobing@51iker.com>;\r\n"
	 *  "\t\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;\r\n"
	 *  "\t\"zhengshuxin3\";\"zhengshuxin4\" <zhengshuxin2@51iker.com>;"
	 *  "<xuganghui@51iker.com>;<wangwenquan@51iker.com>;"
	 * @param to_charset {const char*} Target character set, e.g.: gbk, gb18030, utf-8
	 * @return {const std::list<rfc822_addr*>&} Parsing result
	 */
	const std::list<rfc822_addr*>& parse_addrs(const char* in,
		const char* to_charset = "utf-8");

	/**
	 * Parse an email address conforming to RFC822 standard, and decode username comment part according to
	 * RFC2047 standard
	 * @param in {const char*} Email address in RFC822 format
	 * @param to_charset {const char*} Target character set, e.g.: gbk, gb18030, utf-8
	 * @return {const rfc822_addr*} Returns NULL indicates input email address does not conform to
	 *  RFC822 specification
	 */
	const rfc822_addr* parse_addr(const char* in,
		const char* to_charset = "utf-8");

	/**
	 * Check whether email address is valid
	 * @param in {const char*} Email address in RFC822 format
	 * @return {bool}
	 */
	bool check_addr(const char* in);

private:
	std::list<rfc822_addr*> addrs_;

	void reset();
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

