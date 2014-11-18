#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

#ifdef WIN32
#include <stdarg.h>

namespace acl {

ACL_CPP_API int ACL_CPP_PRINTF(3, 4) snprintf(char *buf,
	size_t size, const char *fmt, ...);

ACL_CPP_API int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

}

#endif
