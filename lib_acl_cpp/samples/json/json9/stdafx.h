// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once


//#include <iostream>
//#include <tchar.h>

// TODO: 在此处引用程序要求的附加头文件

#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"

#ifdef WIN32
 #include <io.h>
 #define	snprintf	_snprintf
#endif


#ifdef	DEBUG

// 以下宏定义用来帮助检查变参中的参数类型是否合法

#undef	logger
#define	logger		printf
#undef	logger_error
#define	logger_error	printf
#undef	logger_warn
#define	logger_warn	printf
#undef	logger_fatal
#define	logger_fatal	printf
#undef	logger_panic
#define	logger_panic	printf

extern void __attribute__((format(printf,3,4))) \
	dummy_debug(int, int, const char*, ...);
#undef	logger_debug
#define	logger_debug	dummy_debug
#endif
