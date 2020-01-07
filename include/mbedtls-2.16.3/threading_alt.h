/**
 * Copyright (C) 2019-2025
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@gmail.com
 * 
 * VERSION
 *   Tue 07 Jan 2020 01:32:15 PM CST
 */

#ifndef __THREADING_ALT_INCLUDE_H__
#define __THREADING_ALT_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)

// same as in lib_acl/include/thread/acl_pthread.h
struct mbedtls_threading_mutex_t
{
    HANDLE id;
    char  dynamic;
};

#else

#include <pthread.h>

#define mbedtls_threading_mutex_t pthread_mutex_t

#endif

#ifdef __cplusplus
}
#endif

#endif
