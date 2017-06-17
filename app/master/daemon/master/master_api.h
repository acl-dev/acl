/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 09:57:32 AM CST
 */

#ifndef __MASTER_API_INCLUDE_H__
#define __MASTER_API_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "master.h"

ACL_MASTER_SERV *acl_master_lookup(const char *name);
ACL_MASTER_SERV *acl_master_start(const char *path);
ACL_MASTER_SERV *acl_master_restart(const char *path);
int acl_master_stop(const char *name);


#ifdef __cplusplus
}
#endif

#endif
