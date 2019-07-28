// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once


//#include <iostream>
//#include <tchar.h>

// TODO: 在此处引用程序要求的附加头文件

#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"

#ifdef	WIN32
#define	snprintf _snprintf
#endif

extern char *var_cfg_html_path;
extern char *var_cfg_upload_path;
