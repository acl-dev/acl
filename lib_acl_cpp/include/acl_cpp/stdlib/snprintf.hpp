#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>

namespace acl {

/**
 * 标准 C snprintf API 封装，可以保证结果缓冲区中的最后一个字节为 '\0'
 * @param buf {char*} 存储结果的缓冲区
 * @param size {size_t} buf 缓冲区长度
 * @param fmt {const char*} 变参格式字符串
 * @return {int} 当缓冲区长度够用时返回实际拷贝的数据长度，否则:
 *  1) UNIX/LINUX 平台下返回实际需要的缓冲区长度，即当缓冲区长度不够时返回值
 *     >= size；需要注意该返回值的含义与 _WIN32 下的不同
 *  2) _WIN32 平台下返回 -1
 */
#if defined(__GNUC__) && (__GNUC__ > 4 ||(__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
ACL_CPP_API int ACL_CPP_PRINTF(3, 4) safe_snprintf(char *buf, size_t size,
	const char *fmt, ...);
#else
ACL_CPP_API int safe_snprintf(char *buf, size_t size, const char *fmt, ...);
#endif

/**
 * 标准 C snprintf API 封装，可以保证结果缓冲区中的最后一个字节为 '\0'
 * @param buf {char*} 存储结果的缓冲区
 * @param size {size_t} buf 缓冲区长度
 * @param fmt {const char*} 变参格式字符串
 * @param ap {va_list} 变参变量
 * @return {int} 当缓冲区长度够用时返回实际拷贝的数据长度，否则:
 *  1) UNIX/LINUX 平台下返回实际需要的缓冲区长度，即当缓冲区长度不够时返回值
 *     >= size；需要注意该返回值的含义与 _WIN32 下的不同
 *  2) _WIN32 平台下返回 -1
 */
ACL_CPP_API int safe_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

}  // namespace acl
