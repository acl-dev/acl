#pragma once
#include "../acl_cpp_define.hpp"

#ifdef ACL_HOOK_NEW

# if defined(_WIN32) || defined(_WIN64)
#  ifdef NDEBUG
void* operator new(size_t n);
void  operator delete(void *p);
#  endif
# else
#include <new>
void* operator new(size_t n)  throw (std::bad_alloc);
void  operator delete(void *p) throw();
# endif

#elif defined(ACL_CPP_DEBUG_MEM)

/**
 * 当需要检查 acl 库中的内存分配中是否存在内存泄露问题时，通过重载 new/delete
 * 将对象的创建与销毁过记录下来，如果对象被创建后没有被释放，则内部就会标记为
 * 一次内存泄露，使用者可以定时输出这些地址信息，查看在哪个文件创建的对象没有
 * 被释放，要想使用此功能，需在 lib_acl_cpp/include/acl_cpp/acl_cpp_define.hpp
 * 中的宏 "//#define ACL_CPP_DEBUG_MEM" 打开，同时应用需要在自己的源代码中包含
 * 头文件：
 * #include "acl_cpp/stdlib/malloc.hpp
 * 最后，应用程序在创建对象时应使用 NEW 来替代 new.
 */
void* operator new(size_t, const char*, const char*, int) throw();
void operator delete(void*) throw();
void operator delete(void*, size_t) throw();

#define NEW new(__FILE__, __FUNCTION__, __LINE__)

namespace acl {

/**
 * 启动内存监控过程
 */
ACL_CPP_API void mem_checker_start(void);

/**
 * 将当前内存状态显示至屏幕，使用者可以自己增加定时器定时调用此函数
 */
ACL_CPP_API void mem_checker_show(void);
}

#endif

namespace acl {

ACL_CPP_API void  acl_slice_init(void);

/**
 * 内存分配函数
 * @param size {size_t} 需要分配的尺寸大小
 * @param filename {const char*} 源程序名字
 * @param funcname {const char*} 函数名
 * @param lineno {int} 源程序行号
 * @return {void*} 分配的内存地址，如果分配内存失败，则直接abort
 */
ACL_CPP_API void* acl_new(size_t size, const char* filename,
	const char* funcname, int lineno);


/**
 * 释放内存函数
 * @param ptr {void*} 内存地址
 * @param filename {const char*} 源程序名字
 * @param funcname {const char*} 函数名
 * @param lineno {int} 源程序行号
 */
ACL_CPP_API void  acl_delete(void *ptr, const char* filename,
	const char* funcname, int lineno);

}
