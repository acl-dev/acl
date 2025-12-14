#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>

#ifndef ACL_LOGGER_MACRO_OFF

# define logger_open	acl::log::open
# define logger_close	acl::log::close

# if defined(_WIN32) || defined(_WIN64)

#  if _MSC_VER >= 1500
#   define logger(fmt, ...)  \
        acl::log::msg4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#   define logger_warn(fmt, ...)  \
        acl::log::warn4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#   define logger_error(fmt, ...)  \
        acl::log::error4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#   define logger_fatal(fmt, ...)  \
        acl::log::fatal4(__FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#   define logger_debug(section, level, fmt, ...)  \
        acl::log::msg6(section, level, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#  else
#   define logger       acl::log::msg1
#   define logger_warn  acl::log::warn1
#   define logger_error acl::log::error1
#   define logger_fatal acl::log::fatal1
#   define logger_debug acl::log::msg3
#  endif

# else

#  ifdef ACL_CPP_LOG_SKIP_FILE
#   define logger(fmt, args...)  \
        acl::log::msg4("none", __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_warn(fmt, args...)  \
        acl::log::warn4("none", __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_error(fmt, args...)  \
        acl::log::error4("none", __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_fatal(fmt, args...)  \
        acl::log::fatal4("none", __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_debug(section, level, fmt, args...)  \
        acl::log::msg6(section, level, __FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  else
#   define logger(fmt, args...)  \
        acl::log::msg4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_warn(fmt, args...)  \
        acl::log::warn4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_error(fmt, args...)  \
        acl::log::error4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_fatal(fmt, args...)  \
        acl::log::fatal4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#   define logger_debug(section, level, fmt, args...)  \
        acl::log::msg6(section, level, __FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#  endif

# endif
#endif // ACL_LOG_MACRO_OFF

namespace acl {

class string;

class ACL_CPP_API log {
public:
	/**
	 * Open log file, call this function once during program initialization
	 * @param recipients {const char*} Log recipient list, separated by "|".
	 * Recipients
	 *  can be local files or remote sockets, e.g.:
	 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
	 * This configuration requires sending all logs to four log recipient objects
	 * simultaneously:
	 *  /tmp/test.log, UDP:127.0.0.1:12345,
	 *  TCP:127.0.0.1:12345 and UNIX:/tmp/test.sock
	 * @param procname Program name, e.g.: test
	 * @param cfg Debug log configuration, format: {section}:{level};
	 * {section}:{level}; ...
	 * e.g.: 100:2; 101:3; 102: 4, means only record logs with identifier 100/level
	 * < 2,
	 *  and identifier 101/level < 3, and identifier 102/level < 4
	 */
	static void open(const char* recipients, const char* procname = "unknown",
		const char* cfg = NULL);

	/**
	 * Call this function to close log before program exits
	 */
	static void close();

	/**
	 * Initialize log debug call interface
	 * @param cfg {const char*} Debug tag and level string, format:
	 *  {section}:{level}; {section}:{level}; ...
	 *  e.g.: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8... or all:1
	 */
	static void debug_init(const char* cfg);

	/**
	 * When log stream is not opened through open and logging related functions are
	 * called, whether to output information
	 * to standard output
	 * @param onoff {bool}
	 */
	static void stdout_open(bool onoff);

	/**
	 * Logging functions
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
	/*                        Examples                                      */
	/************************************************************************/

#ifndef ACL_LOGGER_MACRO_OFF

	static void logger_test1() {
# define DEBUG_TEST_BASE	100
# define DEBUG_TEST1		(DEBUG_TEST_BASE + 1)
# define DEBUG_TEST2		(DEBUG_TEST_BASE + 2)
# define DEBUG_TEST3		(DEBUG_TEST_BASE + 3)

		const char* logfile = "test.log", *procname = "test";
		const char* cfg = "101:2; 102:3; 103:2";

		// Open log during program initialization
		logger_open(logfile, procname, cfg);

# if defined(VC2003) || defined(VC2002) || defined(VC6)

		// Will write log

		logger("%s(%d), %s: %s", __FILE__, __LINE__, __FUNCTION__, "zsx");

		logger_debug(DEBUG_TEST1, 1, "%s(%d), %s: hello world11(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");
		logger_debug(DEBUG_TEST2, 3, "%s(%d), %s: hello world12(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");
		logger_debug(DEBUG_TEST3, 2, "%s(%d), %s: hello world13(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");

		// Will not write log

		logger_debug(DEBUG_TEST1, 3, "%s(%d), %s: hello world21(%s)!",
			__FILE__, __LINE__, __FUNCTION__, "zsx");

# else	// VC2005, VC2008, VC2010

		// Will write log

		logger("error(%s)!", "zsx");

		logger_debug(DEBUG_TEST1, 1, "hello world11(%s)!", "zsx");
		logger_debug(DEBUG_TEST2, 3, "hello world12(%s)!", "zsx");
		logger_debug(DEBUG_TEST3, 2, "hello world13(%s)!", "zsx");

		// Will not write log

		logger_debug(DEBUG_TEST1, 3, "hello world21(%s)!", "zsx");

# endif

		// Close log before program ends
		logger_close();
	}

	static void logger_test2() {
		logger("logger ok!");
		logger_warn("logger_warn ok!");
		logger_error("logger_error ok!");
		logger_fatal("logger_fatal ok!");
	}

#endif // ACL_LOGGER_MACRO_OFF
};

} // namespace acl

