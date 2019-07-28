#pragma once
#include "../acl_cpp_define.hpp"

namespace acl
{

/**
 * 获得上次系统调用出错时的错误号
 * @return {int} 错误号
 */
ACL_CPP_API int last_error(void);

/**
 * 手工设置错误号
 * @param errnum {int} 错误号
 */
ACL_CPP_API void set_error(int errnum);

/**
 * 获得上次系统调用出错时的错误描述信息，该函数内部采用了线程局部变量，所以是线程
 * 安全的，但使用起来更简单些
 * @return {const char *} 返回错误提示信息 
 */
ACL_CPP_API const char* last_serror(void);

/**
 * 获得上次系统调用出错时的错误描述信息
 * @param buf {char*} 存储错误描述信息的内存缓冲区
 * @param size {int} buffer 的空间大小
 * @return {const char*} 返回的地址应与 buffer 相同
 */
ACL_CPP_API const char* last_serror(char* buf, size_t size);

/**
 * 类似于标准C的 strerror, 但该函数是跨平台且是线程安全的，获得对应某个错误
 * 号的错误描述信息
 * @param errnum {int} 错误号
 * @param buf {char*} 存储错误描述信息的内存缓冲区
 * @param size {int} buffer 缓冲区的大小
 * @return {const char*} 返回的地址应与 buffer 相同
*/
ACL_CPP_API const char* string_error(int errnum, char* buf, size_t size);

ACL_CPP_API int strncasecmp_(const char *s1, const char *s2, size_t n);
ACL_CPP_API void assert_(bool n);
ACL_CPP_API void meter_time(const char *filename, int line, const char *info);
ACL_CPP_API long long get_curr_stamp(void);
ACL_CPP_API double stamp_sub(const struct timeval& from,
		const struct timeval& sub);

} // namespace acl
