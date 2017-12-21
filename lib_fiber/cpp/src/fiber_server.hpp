/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 20 Dec 2017 11:24:35 PM CST
 */

#pragma once

const char *acl_fiber_server_conf(void);
void acl_fiber_server_main(int argc, char *argv[],
	void (*service)(void*, ACL_VSTREAM*), void *ctx, int name, ...);
