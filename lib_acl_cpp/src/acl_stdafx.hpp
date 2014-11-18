// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once

//#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料

// TODO: 在此处引用程序要求的附加头文件

#include "acl_cpp/acl_cpp_define.hpp"

#include "lib_acl.h"
#include "lib_protocol.h"

#include "acl_cpp/stdlib/malloc.hpp"

#ifdef WIN32
# include "acl_cpp/stdlib/snprintf.hpp"
# ifdef _DEBUG
#  ifndef _CRTDBG_MAP_ALLOC
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#   include <stdlib.h>
#  endif
#   undef NEW
#   define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
# else
#   undef NEW
#   define NEW new
# endif // _DEBUG
#else
# define NEW new
#endif

// 加入下面一行可以加快在 VC 下的编译速度
#ifdef WIN32
#include "acl_cpp/lib_acl.hpp"
#endif
