/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 14 Jun 2017 12:23:36 PM CST
 */

#include "stdafx.h"
#include "http_client.h"

http_client::http_client(acl::aio_socket_stream *client, int rw_timeout)
	: client_(client)
	, conn_(client->get_astream())
	, rw_timeout_(rw_timeout)
	, content_length_(0)
{
	hdr_req_ = http_hdr_req_new();
	req_     = NULL;
	acl_aio_add_close_hook(conn_, on_close, this);
}

http_client::~http_client(void)
{
	if (req_)
		http_req_free(req_);
	else
		http_hdr_req_free(hdr_req_);
}

void http_client::reset(void)
{
	if (req_)
	{
		req_->hdr_req = NULL;
		http_req_free(req_);
		req_ = NULL;
	}

	http_hdr_req_reset(hdr_req_);
	json_.reset();
}

void http_client::wait(void)
{
	reset();
	http_hdr_req_get_async(hdr_req_, conn_, on_head, this, rw_timeout_);
}

int http_client::on_close(ACL_ASTREAM*, void* ctx)
{
	http_client* hc = (http_client*) ctx;
	delete hc;
	return 0;
}

int http_client::on_head(int status, void* ctx)
{
	if (status != HTTP_CHAT_OK)
	{
		logger_error("invalid status=%d", status);
		return -1;
	}

	http_client* hc = (http_client*) ctx;
	if (http_hdr_req_parse(hc->hdr_req_) < 0)
	{
		logger_error("parse http header error");
		return -1;
	}

	hc->content_length_ = hc->hdr_req_->hdr.content_length;
	if (hc->content_length_ <= 0)
		return hc->handle();

	hc->req_ = http_req_new(hc->hdr_req_);
	http_req_body_get_async(hc->req_, hc->conn_, on_body,
		hc, hc->rw_timeout_);

	return 0;
}

int http_client::on_body(int status, char *data, int dlen, void *ctx)
{
	if (status == HTTP_CHAT_ERR_MIN)
	{
		logger_error("status=%d", status);
		return -1;
	}

	if (dlen <= 0)
	{
		logger_error("invalid dlen=%d", dlen);
		return -1;
	}

	http_client* hc = (http_client*) ctx;
	if (status == HTTP_CHAT_OK)
		return hc->handle();

	hc->json_.update(data);
	return 0;
}

int http_client::handle(void)
{
	const char* cmd = http_hdr_req_param(hdr_req_, "cmd");
	if (cmd == NULL || *cmd == 0)
	{
		logger_error("cmd null");
		acl_aio_iocp_close(conn_);
		return -1;
	}

#define EQ !strcasecmp

	if (EQ(cmd, "list"))
		return handle_list();
	else if (EQ(cmd, "stat"))
		return handle_stat();
	else if (EQ(cmd, "start"))
		return handle_start();
	else if (EQ(cmd, "stop"))
		return handle_stop();
	else {
		logger_error("invalid cmd=%s", cmd);
		return -1;
	}
}

void http_client::do_reply(int status, const acl::string& body)
{
	HTTP_HDR_RES* hdr_res = http_hdr_res_static(status);
	http_hdr_set_keepalive(hdr_req_, hdr_res);
	http_hdr_put_str(&hdr_res->hdr, "Content-Type", "text/json");
	http_hdr_put_int(&hdr_res->hdr, "Content-Length", (int) body.size());

	acl::string buf(body.size() + 256);
	http_hdr_build(&hdr_res->hdr, body.vstring());
	http_hdr_res_free(hdr_res);
	buf.append(body);

	acl_aio_writen(conn_, body.c_str(), (int) body.size());

	if (status == 200 && hdr_res->hdr.keep_alive)
		wait();
	else
		acl_aio_iocp_close(conn_);
}

int http_client::handle_list(void)
{
	list_req_t req;
	list_res_t res;

	if (deserialize<list_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<list_res_t>(res.status, res);
		return -1;
	}

	res.status = 200;
	res.msg    = "ok";
	reply<list_res_t>(res.status, res);
	return 0;
}

int http_client::handle_stat(void)
{
	stat_req_t req;
	stat_res_t res;

	if (deserialize<stat_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<stat_res_t>(res.status, res);
		return -1;
	}

	res.status = 200;
	res.msg    = "ok";
	reply<stat_res_t>(res.status, res);
	return 0;
}

int http_client::handle_stop(void)
{
	stop_req_t req;
	stop_res_t res;

	if (deserialize<stop_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<stop_res_t>(res.status, res);
		return -1;
	}

	res.status = 200;
	res.msg    = "ok";
	reply<stop_res_t>(res.status, res);
	return 0;
}

int http_client::handle_start(void)
{
	start_req_t req;
	start_res_t res;

	if (deserialize<start_req_t>(json_, req) == false)
	{
		res.status = 400;
		res.msg    = "invalid json";
		reply<start_res_t>(res.status, res);
		return -1;
	}

	res.status = 200;
	res.msg    = "ok";
	reply<start_res_t>(res.status, res);
	return 0;
}
