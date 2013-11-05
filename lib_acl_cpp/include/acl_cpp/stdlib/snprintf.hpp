#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

#ifdef WIN32
#include <stdarg.h>

namespace acl {

#ifdef	WIN32
ACL_CPP_API int snprintf(char *buf, size_t size, const char *fmt, ...);
#else
ACL_CPP_API int __attribute__((format(printf, 3, 4)))
	snprintf(char *buf, size_t size, const char *fmt, ...);
#endif
ACL_CPP_API int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

}

#endif
