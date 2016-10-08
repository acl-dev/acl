// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once


//#include <iostream>
//#include <tchar.h>

// TODO: 在此处引用程序要求的附加头文件
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd 
#define chdir _chdir
#endif