#include "stdafx.h"
#include "action/service_list.h"
#include "action/service_stat.h"
#include "action/service_start.h"
#include "action/service_kill.h"
#include "action/service_stop.h"
#include "action/service_reload.h"
#include "action/service_restart.h"
#include "action/service_master_config.h"
#include "http_client.h"

http_client::http_client(acl::aio_socket_stream *client, int rw_timeout)
: conn_(client->get_astream())
, rw_timeout_(rw_timeout)
, content_length_(0)
{
	hdr_req_ = NULL;
	req_     = NULL;
	acl_aio_add_close_hook(conn_, on_close, this);
	acl_aio_add_timeo_hook(conn_, on_timeo, this);
}

http_client::~http_client(void)
{
	if (req_)
		http_req_free(req_);
	else if (hdr_req_)
		http_hdr_req_free(hdr_req_);
}

void http_client::reset(void)
{
	if (req_) {
		http_req_free(req_);
		req_ = NULL;
	} else if (hdr_req_)
		http_hdr_req_free(hdr_req_);

	hdr_req_ = http_hdr_req_new();
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

int http_client::on_timeo(ACL_ASTREAM*, void*)
{
	// return -1 so the connection can be closed
	return -1;
}

int http_client::on_head(int status, void* ctx)
{
	http_client* hc = (http_client*) ctx;

	acl_aio_disable_readwrite(hc->conn_);

	if (status != HTTP_CHAT_OK) {
		logger_error("invalid status=%d", status);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	if (http_hdr_req_parse(hc->hdr_req_) < 0) {
		logger_error("parse http header error");
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	hc->content_length_ = hc->hdr_req_->hdr.content_length;
	if (hc->content_length_ <= 0)
		return hc->handle() ? 0 : -1;

	hc->req_ = http_req_new(hc->hdr_req_);
	http_req_body_get_async(hc->req_, hc->conn_, on_body,
		hc, hc->rw_timeout_);

	return 0;
}

int http_client::on_body(int status, char *data, int dlen, void *ctx)
{
	http_client* hc = (http_client*) ctx;

	if (status >= HTTP_CHAT_ERR_MIN) {
		logger_error("status=%d", status);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	if (dlen <= 0) {
		logger_error("invalid dlen=%d", dlen);
		acl_aio_iocp_close(hc->conn_);
		return -1;
	}

	hc->json_.update(data);

	if (status == HTTP_CHAT_OK) {
		acl_aio_disable_readwrite(hc->conn_);
		return hc->handle() ? 0 : -1;
	}

	return 0;
}

void http_client::do_reply(int status, const char* cmd,
	const acl::string& body, bool save)
{
	HTTP_HDR_RES* hdr_res = http_hdr_res_static(status);
	http_hdr_set_keepalive(hdr_req_, hdr_res);
	http_hdr_put_str(&hdr_res->hdr, "Content-Type", "text/json");
	http_hdr_put_int(&hdr_res->hdr, "Content-Length", (int) body.size());

	acl::string buf(body.size() + 256);
	http_hdr_build(&hdr_res->hdr, buf.vstring());
	http_hdr_res_free(hdr_res);
	buf.append(body);

	if (save) {
		// logger the important command's information
		acl::string reqhdr;
		http_hdr_sprint(reqhdr.vstring(), &hdr_req_->hdr, cmd);
		logger("cmd=[%s]:\r\n[%s\r\n%s]\r\n\r\n[%s]\r\n", cmd,
			reqhdr.c_str(), json_.to_string().c_str(), buf.c_str());
	}

	acl_aio_writen(conn_, buf.c_str(), (int) buf.size());
}

static struct {
	const char* cmd;
	bool (http_client::*handler)(void);
} handlers[] = {
	{ "list",	&http_client::handle_list		 },
	{ "stat",	&http_client::handle_stat		 },
	{ "start",	&http_client::handle_start		 },
	{ "kill",	&http_client::handle_kill		 },
	{ "stop",	&http_client::handle_stop		 },
	{ "restart",	&http_client::handle_restart		 },
	{ "reload",	&http_client::handle_reload		 },

	{ "master_config",&http_client::handle_master_config	 },

	{ 0,		0					 }
};

bool http_client::handle(void)
{
	const char* cmd = http_hdr_req_param(hdr_req_, "cmd");
	if (cmd == NULL || *cmd == 0) {
		//logger_error("cmd null");
		acl::string dummy;
		do_reply(400, "none", dummy, false);
		if (hdr_req_->hdr.keep_alive)
			wait();
		else
			acl_aio_iocp_close(conn_);
		return true;
	}

#define EQ !strcasecmp

	int i;

	for (i = 0; handlers[i].cmd != NULL; i++) {
		if (EQ(cmd, handlers[i].cmd))
			break;
	}

	if (handlers[i].handler == NULL) {
		logger_warn("invalid cmd=%s", cmd);
		acl::string dummy;
		do_reply(400, "unknown", dummy, false);
		if (hdr_req_->hdr.keep_alive)
			wait();
		else
			acl_aio_iocp_close(conn_);
		return true;
	}

	if ((this->*handlers[i].handler)())
		return true;

	acl_aio_iocp_close(conn_);
	return false;
}

bool http_client::handle_list(void)
{
	service_list service(*this);
	return service.run(json_);
}

bool http_client::handle_stat(void)
{
	service_stat service(*this);
	return service.run(json_);
}

bool http_client::handle_kill(void)
{
	service_kill service(*this);
	return service.run(json_);
}

bool http_client::handle_stop(void)
{
	service_stop service(*this);
	return service.run(json_);
}

bool http_client::handle_start(void)
{
	service_start* service = new service_start(*this);
	return service->run(json_);
}

bool http_client::handle_restart(void)
{
	service_restart service(*this);
	return service.run(json_);
}

bool http_client::handle_reload(void)
{
	service_reload* service = new service_reload(*this);
	return service->run(json_);
}

bool http_client::handle_master_config(void)
{
	service_master_config service(*this);
	return service.run(json_);
}

void http_client::on_finish(void)
{
	if (hdr_req_->hdr.keep_alive)
		wait();
	else
		acl_aio_iocp_close(conn_);
}
