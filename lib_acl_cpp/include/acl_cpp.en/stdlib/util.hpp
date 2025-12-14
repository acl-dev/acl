#pragma once
#include "../acl_cpp_define.hpp"

namespace acl {

/**
 * Get the error number when the last system call failed
 * @return {int} Error number
 */
ACL_CPP_API int last_error();

/**
 * Manually set error number
 * @param errnum {int} Error number
 */
ACL_CPP_API void set_error(int errnum);

/**
 * Get the error description when the last system call failed. This function uses thread-local variables internally, so it is thread-safe,
 * but simpler to use
 * @return {const char *} Returns error message
 */
ACL_CPP_API const char* last_serror();

/**
 * Get the error description when the last system call failed
 * @param buf {char*} Memory buffer to store error description
 * @param size {int} Size of buffer space
 * @return {const char*} Returned address should be the same as buffer
 */
ACL_CPP_API const char* last_serror(char* buf, size_t size);

/**
 * Similar to standard C's strerror, but this function is cross-platform and thread-safe, gets the error
 * description for a specific error number
 * @param errnum {int} Error number
 * @param buf {char*} Memory buffer to store error description
 * @param size {int} Size of buffer
 * @return {const char*} Returned address should be the same as buffer
*/
ACL_CPP_API const char* string_error(int errnum, char* buf, size_t size);

ACL_CPP_API int strncasecmp_(const char *s1, const char *s2, size_t n);
ACL_CPP_API void assert_(bool n);
ACL_CPP_API void meter_time(const char *filename, int line, const char *info);
ACL_CPP_API long long get_curr_stamp();
ACL_CPP_API double stamp_sub(const struct timeval& from,
		const struct timeval& sub);

} // namespace acl

