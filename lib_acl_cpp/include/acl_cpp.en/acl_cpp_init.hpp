#pragma once
#include "acl_cpp_define.hpp"

namespace acl
{

	/**
	 * Under _WIN32 DOS window, if you need to use socket operations,
	 * you need to call this function first for initialization
	 */
	ACL_CPP_API void acl_cpp_init(void);

	/**
	 * Get the capabilities exposed by the current acl_cpp library
	 * @return {const char*} Returns a non-empty string
	 */
	ACL_CPP_API const char* acl_cpp_verbose(void);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Open DOS window under win32
	 */
	ACL_CPP_API void open_dos(void);

	/**
	 * Close DOS window under win32
	 */
	ACL_CPP_API void close_dos(void);
#endif

}

