/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   Dongcheng District, Beijing, RPC 100006
 *   E-mail: shuxin.zheng@net263.com
 * 
 * VERSION
 *   Wed 11 Jan 2017 02:02:19 PM CST
 */

#ifndef __FIBER_CLIENT_INCLUDE_H__
#define __FIBER_CLIENT_INCLUDE_H__

extern int  __nconnect;
extern int  __count;
extern char __listen_ip[64];
extern int  __listen_port;
extern int  __listen_qlen;
extern int  __rw_timeout;
extern int  __echo_data;
extern int  __stack_size;

void fiber_client(ACL_FIBER *fiber, void *ctx);

#endif
