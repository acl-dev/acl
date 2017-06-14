/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 14 Jun 2017 12:21:44 PM CST
 */

#pragma once
#include "serialize.h"

class http_client
{
public:
	http_client(acl::aio_socket_stream* client, int rw_timeout = 10);

	void wait(void);

private:
	~http_client(void);

private:
	acl::aio_socket_stream* client_;
	ACL_ASTREAM*            conn_;
	int                     rw_timeout_;
	HTTP_HDR_REQ*           hdr_req_;
	HTTP_REQ               *req_;
	http_off_t              content_length_;
	acl::json               json_;

	void reset(void);
	int handle(void);
	int handle_list(void);
	int handle_stat(void);
	int handle_stop(void);
	int handle_start(void);

	void do_reply(int status, const acl::string& buf);

	template<typename T>
	void reply(int status, T& o)
	{
		acl::string buf;
		serialize<T>(o, buf);
		do_reply(status, buf);
	}

	static int on_head(int status, void* ctx);
	static int on_body(int status, char* data, int dlen, void* ctx);
	static int on_close(ACL_ASTREAM* conn, void* ctx);
};
