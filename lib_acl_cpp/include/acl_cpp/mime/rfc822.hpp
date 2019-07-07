#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <time.h>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

/**
 * 邮件地址
 */
struct rfc822_addr
{
	char* addr;	// 邮件地址，格式为： xxx@xxx.xxx
	char* comment;	// 邮件备注
};

typedef enum
{
	tzone_gmt,
	tzone_cst
} tzone_t;

class ACL_CPP_API rfc822 : public noncopyable
{
public:
	rfc822(void);
	~rfc822(void);

	/**
	 * 解析符合 RFC822 标准的时间格式
	 * @param in {const char*} 时间字符串，如：
	 *  Wed, 11 May 2011 09:44:37 +0800 (CST)
	 *  Wed, 11 May 2011 16:17:39 GMT
	 */
	time_t parse_date(const char *in);

	/**
	 * 生成符合 RFC822 标准的时间格式
	 * @param t {time_t}
	 * @param out {char*} 存储转换结果
	 * @param size {size_t} out 空间大小
	 * @param zone {tzone_t} 所在时区
	 */
	void mkdate(time_t t, char* out, size_t size, tzone_t  zone = tzone_cst);

	/**
	 * 生成东八区的时间格式
	 * @param t {time_t}
	 * @param out {char*} 存储转换结果
	 * @param size {size_t} out 空间大小
	 */
	void mkdate_cst(time_t t, char* out, size_t size);

	/**
	 * 生成格林威治时间的时间格式
	 * @param t {time_t}
	 * @param out {char*} 存储转换结果
	 * @param size {size_t} out 空间大小
	 */
	void mkdate_gmt(time_t t, char* out, size_t size);

	/**
	 * 解析邮件地址列表，将符合 RFC822 标准的邮件地址列表解析成
	 * 人能正常看懂的邮件地址列表，同时将用户名注释部分进行
	 * RFC2047解码
	 * @param in {const char*} RFC822 格式的邮件地址列表，如:
	 *  "=?gb2312?B?1dSx+A==?= <zhaobing@51iker.com>;\r\n"
	 *  "\t\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;\r\n"
	 *  "\t\"zhengshuxin3\";\"zhengshuxin4\" <zhengshuxin2@51iker.com>;"
	 *  "<xuganghui@51iker.com>;<wangwenquan@51iker.com>;"
	 * @param to_charset {const char*} 目标字符集，例如：gbk, gb18030, utf-8
	 * @return {const std::list<rfc822_addr*>&} 解析结果
	 */
	const std::list<rfc822_addr*>& parse_addrs(const char* in,
		const char* to_charset = "utf-8");

	/**
	 * 解析一个符合 RFC822 标准的邮件地址，同时将用户名注释部分按
	 * RFC2047 标准进行解码
	 * @param in {const char*} RFC822 格式的邮件地址
	 * @param to_charset {const char*} 目标字符集，例如：gbk, gb18030, utf-8
	 * @return {const rfc822_addr*} 返回 NULL 表明输入的邮件地址不符合
	 *  RFC822 规范
	 */
	const rfc822_addr* parse_addr(const char* in,
		const char* to_charset = "utf-8");

	/**
	 * 检查邮件地址是否合法
	 * @param in {const char*} RFC822 格式的邮件地址
	 * @return {bool}
	 */
	bool check_addr(const char* in);

private:
	std::list<rfc822_addr*> addrs_;

	void reset(void);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
