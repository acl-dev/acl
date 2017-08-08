/**
 * Copyright (C) 2017-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 08 Aug 2017 03:09:21 PM CST
 */

#pragma once

namespace acl
{

class tcp_manager;
class tcp_pool;
class string;

class tcp_ipc
{
public:
	tcp_ipc(void);
	~tcp_ipc(void);

	tcp_ipc& set_limit(int max);
	tcp_ipc& set_idle(int ttl);
	tcp_ipc& set_conn_timeout(int conn_timeout);
	tcp_ipc& set_rw_timeout(int timeout);

	bool send(const char* addr, const void* data, unsigned int len,
		string* out = NULL);

private:
	tcp_manager* manager_;
	int max_;
	int ttl_;
	int conn_timeout_;
	int rw_timeout_;

	bool send(tcp_pool&, const void*, unsigned int, string*);
};

} // namespace acl
