#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>

#define logger_open	acl::log::open
#define logger_close	acl::log::close

#if defined(_WIN32) || defined(_WIN64)

# if _MSC_VER >= 1500
#  define logger(fmt, ...)  \
	acl::log::msg4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#  define logger_warn(fmt, ...)  \
	acl::log::warn4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#  define logger_error(fmt, ...)  \
	acl::log::error4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#  define logger_fatal(fmt, ...)  \
	acl::log::fatal4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#  define logger_debug(section, level, fmt, ...)  \
	acl::log::msg6(section, level, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
# else
#  define logger	acl::log::msg1
#  define logger_warn	acl::log::warn1
#  define logger_error	acl::log::error1
#  define logger_fatal	acl::log::fatal1
#  define logger_debug	acl::log::msg3
# endif

#else

# ifdef ACL_CPP_LOG_SKIP_FILE
#  define logger(fmt, args...)  \
	acl::log::msg4("none", __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_warn(fmt, args...)  \
	acl::log::warn4("none", __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_error(fmt, args...)  \
	acl::log::error4("none", __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_fatal(fmt, args...)  \
	acl::log::fatal4("none", __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_debug(section, level, fmt, args...)  \
	acl::log::msg6(section, level, __FUNCTION__, __LINE__, __FUNCTION__, fmt, ##args)
# else
#  define logger(fmt, args...)  \
	acl::log::msg4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_warn(fmt, args...)  \
	acl::log::warn4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_error(fmt, args...)  \
	acl::log::error4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_fatal(fmt, args...)  \
	acl::log::fatal4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  define logger_debug(section, level, fmt, args...)  \
	acl::log::msg6(section, level, __FUNCTION__, __LINE__, __FUNCTION__, fmt, ##args)
# endif

#endif

namespace acl {

class string;

class ACL_CPP_API log
{
public:
	/**
	 * 打开日志文件, 在程序初始化里调用本函数一次
	 * @param recipients {const char*} 日志接收器列表，由 "|" 分隔，接收器
	 *  可以是本地文件或远程套接口，如:
	 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
	 *  该配置要求将所有日志同时发给 /tmp/test.log, UDP:127.0.0.1:12345,
	 *  TCP:127.0.0.1:12345 和 UNIX:/tmp/test.sock 四个日志接收器对象
	 * @param procname 程序名, 如: test
	 * @param cfg 调试日志配置, 格式为: {section}:{level}; {section}:{level}; ...
	 *  如: 100:2; 101:3; 102: 4, 表示只记录标识为 100/级别 < 2,
	 *  以及标识为 101/级别 < 3, 以及标识为 102/级别 < 4 的日志项
	 */
	static void open(const char* recipients, const char* procname = "unknown",
		const char* cfg = NULL);

	/**
	 * 程序退出前调用此函数关闭日志
	 */
	static void close(void);

	/**
	 * 初始化日志调试调用接口
	 * @param cfg {const char*} 调试标签及级别字符串, 格式如下:
	 *  {section}:{level}; {section}:{level}; ...
	 *  如: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8... or all:1
	 */
	static void debug_init(const char* cfg);

	/**
	 * 当未通过 open 打开日志流而调用记日志等相关函数时是否需要将信息
	 * 输出至标准输出
	 * @param onoff {bool}
	 */
	static void stdout_open(bool onoff);

	/**
	 * 日志记录函数
	 */

	static void ACL_CPP_PRINTF(1, 2) msg1(const char* fmt, ...);
	static void ACL_CPP_PRINTF(4, 5) msg4(const char* fname,
		int line, const char* func, const char* fmt, ...);
	static void ACL_CPP_PRINTF(3, 4) msg3(size_t section,
		size_t level, const char* fmt, ...);
	static void ACL_CPP_PRINTF(6, 7) msg6(size_t section, size_t level,
		const char* fname, int line, const char* func, const char* fmt, ...);

	static void ACL_CPP_PRINTF(1, 2) warn1(const char* fmt, ...);
	static void ACL_CPP_PRINTF(4, 5) warn4(const char* fname, int line,
		const char* func, const char* fmt, ...);

	static void ACL_CPP_PRINTF(1, 2) error1(const char* fmt, ...);
	static void ACL_CPP_PRINTF(4, 5) error4(const char* fname, int line,
		const char* func, const char* fmt, ...);

	static void ACL_CPP_PRINTF(1, 2) fatal1(const char* fmt, ...);
	static void ACL_CPP_PRINTF(4, 5) fatal4(const char* fname, int line,
		const char* func, const char* fmt, ...);

	static void vmsg2(const char* fmt, va_list ap);
	static void vmsg5(const char* fname, int line, const char* func,
		const char* fmt, va_list ap);
	static void vmsg4(size_t section, size_t level, const char* fmt, va_list ap);
	static void vmsg7(size_t section, size_t level, const char* fname,
		int line, const char* func, const char* fmt, va_list ap);

	static void vwarn2(const char* fmt, va_list ap);
	static void vwarn5(const char* fname, int line, const char* func,
		const char* fmt, va_list ap);

	static void verror2(const char* fmt, va_list ap);
	static void verror5(const char* fname, int line, const char* func,
		const char* fmt, va_list ap);

	static void vfatal2(const char* fmt, va_list ap);
	static void vfatal5(const char* fname, int line, const char* func,
		const char* fmt, va_list ap);

	/************************************************************************/
	/*                        示例                                          */
	/************************************************************************/
	static void logger_test1(void)
	{
#define DEBUG_BASE	100
#define DEBUG_TEST1	(DEBUG_BASE + 1)
#define DEBUG_TEST2	(DEBUG_BASE + 2)
#define DEBUG_TEST3	(DEBUG_BASE + 3)

		const char* logfile = "test.log", *procname = "test";
		const char* cfg = "101:2; 102:3; 103:2";

		// 在程序初始化时打开日志
		logger_open(logfile, procname, cfg);

#if defined(VC2003) || defined(VC2002) || defined(VC6)

		// 会写日志

		logger("%s(%d), %s: %s", __FILE__, __LINE__, __FUNCTION__, "zsx");

		logger_debug(DEBUG_TEST1, 1, "%s(%d), %s: hello world11(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");
		logger_debug(DEBUG_TEST2, 3, "%s(%d), %s: hello world12(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");
		logger_debug(DEBUG_TEST3, 2, "%s(%d), %s: hello world13(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");

		// 不会写日志

		logger_debug(DEBUG_TEST1, 3, "%s(%d), %s: hello world21(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");

#else	// VC2005, VC2008, VC2010

		// 会写日志

		logger("error(%s)!", "zsx");

		logger_debug(DEBUG_TEST1, 1, "hello world11(%s)!", "zsx");
		logger_debug(DEBUG_TEST2, 3, "hello world12(%s)!", "zsx");
		logger_debug(DEBUG_TEST3, 2, "hello world13(%s)!", "zsx");

		// 不会写日志

		logger_debug(DEBUG_TEST1, 3, "hello world21(%s)!", "zsx");

#endif

		// 程序结束前关闭日志
		logger_close();
	}
	static void logger_test2(void)
	{
		logger("logger ok!");
		logger_warn("logger_warn ok!");
		logger_error("logger_error ok!");
		logger_fatal("logger_fatal ok!");
	}
};

} // namespace acl
