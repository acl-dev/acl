#pragma once
#include "acl_cpp_define.hpp"

namespace acl
{

	/**
	 * 在 WIN32 dos 窗口下，如果需要使用套接口操作，
	 * 则需要先调用此函数进行初始化
	 */
	ACL_CPP_API void acl_cpp_init(void);

#ifdef WIN32
	/**
	 * win32 下打开 DOS 窗口
	 */
	ACL_CPP_API void open_dos(void);

	/**
	 * win32 下关闭 DOS 窗口
	 */
	ACL_CPP_API void close_dos(void);
#endif

}
