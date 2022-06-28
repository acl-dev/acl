// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

//#pragma once

//#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料

// TODO: 在此处引用程序要求的附加头文件

#include "acl_cpp/acl_cpp_define.hpp"

#include "lib_acl.h"
#include "lib_protocol.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <vector>
#include <map>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "acl_cpp/stdlib/malloc.hpp"

#ifndef ACL_CPP_DEBUG_MEM

# if defined(_WIN32) || defined(_WIN64)
#  include "acl_cpp/stdlib/snprintf.hpp"
#  ifdef _DEBUG
#   ifndef _CRTDBG_MAP_ALLOC
#    define _CRTDBG_MAP_ALLOC
#    include <crtdbg.h>
#    include <stdlib.h>
#   endif
#    undef NEW
#    define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#  else
#    undef NEW
#    define NEW new
#  endif // _DEBUG
# else
#  define NEW new
#endif

#endif  // ACL_CPP_DEBUG_MEM

#if defined(ACL_UNIX)
#include <pthread.h>
#include <unistd.h>
#include <zlib.h>
#endif

// 加入下面一行可以加快在 VC 下的编译速度
//#if defined(_WIN32) || defined(_WIN64)
#include "acl_cpp/lib_acl.hpp"
//#endif

#define ACL_CPP_DEBUG_MIN 		40
#define ACL_CPP_DEBUG_CONN_MANAGER	41
#define ACL_CPP_DEBUG_MAX 		70

