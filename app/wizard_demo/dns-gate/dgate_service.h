/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Sat Apr 30 10:20:12 2022
 */

#pragma once

void dgate_service_start(void);

void dgate_push_request(acl::socket_stream* server, const char* peer_addr,
	const char* data, size_t dlen);
