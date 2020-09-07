// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once


//#include <iostream>
//#include <tchar.h>

// TODO: 在此处引用程序要求的附加头文件

#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

#ifdef	WIN32
#define	snprintf _snprintf
#endif

typedef acl::HttpServletRequest  HttpRequest;
typedef acl::HttpServletResponse HttpResponse;

// __cplusplus show c++ version as below:
// 199711L: c++98, 201103L: c++11, 201402L: c++14, 201703L: c++17

#if defined(__cplusplus) && __cplusplus >= 201103L
#include <functional>
typedef std::function<bool(HttpRequest&, HttpResponse&)> http_handler_t;
typedef std::function<bool(const char*, HttpRequest&, HttpResponse&)>
	http_default_handler_t;
#else
typedef bool (*http_handler_t)(HttpRequest&, HttpResponse&);
typedef bool (*http_default_handler_t)(const char*, HttpRequest&, HttpResponse&);
#endif

typedef std::map<acl::string, http_handler_t> http_handlers_t;

enum {
	http_handler_get = 0,
	http_handler_post,
	http_handler_head,
	http_handler_put,
	http_handler_patch,
	http_handler_connect,
	http_handler_purge,
	http_handler_delete,
	http_handler_options,
	http_handler_profind,
	http_handler_websocket,
	http_handler_error,
	http_handler_unknown,
	http_handler_max,
};

#undef logger
#undef logger_warn
#undef logger_error
#undef logger_fatal
#undef logger_debug

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
# define logger(fmt, args...)  \
	acl::log::msg4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
# define logger_warn(fmt, args...)  \
	acl::log::warn4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
# define logger_error(fmt, args...)  \
	acl::log::error4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
# define logger_fatal(fmt, args...)  \
	acl::log::fatal4(__FILE__, __LINE__, __FUNCTION__, fmt, ##args)
# define logger_debug(section, level, fmt, args...)  \
	acl::log::msg6(section, level, __FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#endif // !_WIN32 && !_WIN64

extern char *var_cfg_str;
extern acl::master_str_tbl var_conf_str_tab[];

extern int  var_cfg_bool;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern acl::master_int_tbl var_conf_int_tab[];

extern long long int  var_cfg_int64;
extern acl::master_int64_tbl var_conf_int64_tab[];
