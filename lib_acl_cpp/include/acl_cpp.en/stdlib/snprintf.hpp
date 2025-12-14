#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>

namespace acl {

/**
 * Standard C snprintf API wrapper, ensures the last byte in the result buffer
 * is '\0'
 * @param buf {char*} Buffer to store results
 * @param size {size_t} buf buffer length
 * @param fmt {const char*} Variable argument format string
 * @return {int} When buffer length is sufficient, returns actual copied data
 * length, otherwise:
 * 1) On UNIX/LINUX platform, returns the actual required buffer length, i.e.,
 * when buffer length is insufficient, return value
 * >= size; note that the meaning of this return value differs from that on
 * _WIN32
 *  2) On _WIN32 platform, returns -1
 */
#if defined(__GNUC__) && (__GNUC__ > 4 ||(__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
ACL_CPP_API int ACL_CPP_PRINTF(3, 4) safe_snprintf(char *buf, size_t size,
	const char *fmt, ...);
#else
ACL_CPP_API int safe_snprintf(char *buf, size_t size, const char *fmt, ...);
#endif

/**
 * Standard C snprintf API wrapper, ensures the last byte in the result buffer
 * is '\0'
 * @param buf {char*} Buffer to store results
 * @param size {size_t} buf buffer length
 * @param fmt {const char*} Variable argument format string
 * @param ap {va_list} Variable argument variable
 * @return {int} When buffer length is sufficient, returns actual copied data
 * length, otherwise:
 * 1) On UNIX/LINUX platform, returns the actual required buffer length, i.e.,
 * when buffer length is insufficient, return value
 * >= size; note that the meaning of this return value differs from that on
 * _WIN32
 *  2) On _WIN32 platform, returns -1
 */
ACL_CPP_API int safe_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

}  // namespace acl

