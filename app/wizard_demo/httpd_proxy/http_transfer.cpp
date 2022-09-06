#include "stdafx.h"
#include "http_transfer.h"

http_transfer::http_transfer(acl::sslbase_conf* ssl_conf, acl::http_method_t method,
	request_t& req, response_t& res, int port)
: ssl_conf_(ssl_conf)
, method_(method)
, req_(req)
, res_(res)
, port_(port)
, client_(NULL)
{
	box_ = new acl::fiber_tbox<bool>;
	res_client_ = res_.getClient();
}

http_transfer::~http_transfer(void) {
	delete client_;
	delete box_;
}

void http_transfer::wait(bool* keep_alive) {
	bool* res = box_->pop();
	assert(res);
	*keep_alive = *res;
	delete res;
}

static int nwait = 0;
void http_transfer::run(void) {
	nwait++;
	bool* res = new bool;
	switch (method_) {
	case acl::HTTP_METHOD_GET:
		*res = transfer_get();
		break;
	case acl::HTTP_METHOD_POST:
		*res = transfer_post();
		break;
	default:
		logger_error("not support method: %d", (int) method_);
		*res = false;
		break;
	}

	nwait--;
	printf(">>>>nwait=%d\n", nwait);
	box_->push(res);
}

bool http_transfer::setup_ssl(acl::socket_stream& conn,
		acl::sslbase_conf& ssl_conf, const char* host)
{
	acl::sslbase_io* hook = (acl::sslbase_io*) conn.get_hook();
	if (hook != NULL) {
		logger("fd=%d, already in ssl status", conn.sock_handle());
		return true;
	}

	// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
	// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程

	logger("begin setup ssl hook...");

	// 采用阻塞 SSL 握手方式
	acl::sslbase_io* ssl = ssl_conf.create(false);

	// 设置 SSL SNI, 以便服务端选择合适的证书
	ssl->set_sni_host(host);

	if (conn.setup_hook(ssl) == ssl) {
		logger_error("setup_hook error, fd=%d", conn.sock_handle());
		ssl->destroy();
		return false;
	}

	if (!ssl->handshake()) {
		logger_error("ssl handshake failed, fd=%d", conn.sock_handle());
		ssl->destroy();
		return false;
	}

	if (!ssl->handshake_ok()) {
		logger("handshake trying again, fd=%d", conn.sock_handle());
		ssl->destroy();
		return false;
	}

	logger("handshake_ok, fd=%d", conn.sock_handle());
	return true;
}

bool http_transfer::open_peer(request_t& req, acl::socket_stream& conn)
{
	const char* host = req.getRemoteHost();
	if (host == NULL || *host == 0) {
		logger_error("no Host in request head");
		return false;
	}

	acl::string buf(host);

	char* ptr = strrchr(buf.c_str(), ':');
	if (ptr != NULL && *(ptr + 1) != 0) {
		*ptr++ = 0;
		int port = atoi(ptr);
		if (port > 0 && port < 65535) {
			port_ = port;
		}
	}

	acl::string addr;
	addr.format("%s|%d", buf.c_str(), port_);

	if (!conn.open(addr, 5, 5)) {
		logger_error("connect %s error %s",
			addr.c_str(), acl::last_serror());
		return false;
	}

	logger("connect %s ok, fd=%d, use ssl=%s", addr.c_str(),
		conn.sock_handle(), ssl_conf_ ? "yes" : "no");

	if (ssl_conf_ && !setup_ssl(conn, *ssl_conf_, host)) {
		logger_error("setup ssl error!");
		return false;
	}

	bool is_request = true, unzip = false, fixed_stream = true;
	client_ = new acl::http_client(&conn, is_request, unzip, fixed_stream);
	return true;
}

bool http_transfer::transfer_request_head(acl::socket_stream& conn) {
	acl::string header;
	req_.sprint_header(header, NULL);
	if (header.empty()) {
		logger_error("http request head empty");
		return false;
	}

	header += "\r\n";

	if (conn.write(header) == -1) {
		logger_error("write request header error");
		return false;
	}

	logger_debug(DEBUG_REQ, 2, ">>>send head: [%s]", header.c_str());
	return true;
}

bool http_transfer::transfer_request_body(acl::socket_stream& conn) {
	long long length = req_.getContentLength();
	if (length <= 0) {
		return true;
	}

	long long n = 0;
	char buf[8192];

	acl::istream* in = &req_.getInputStream();
	while (n < length) {
		int ret = in->read(buf, sizeof(buf), false);
		if (ret == -1) {
			logger_error("read request body error");
			return false;
		}

		if (conn.write(buf, ret) == -1) {
			logger_error("send request body error");
			return false;
		}

		n += ret;
	}

	return true;
}

bool http_transfer::transfer_get(void) {
	if (!open_peer(req_, conn_)) {
		logger_error("open server error");
		return false;
	}

	if (!transfer_request_head(conn_)) {
		logger_error("transfer_request_head error");
		return false;
	} else {
		return transfer_response();
	}
}

bool http_transfer::transfer_post(void) {
	if (!open_peer(req_, conn_)) {
		logger_error("open server error");
		return false;
	}

	if (!transfer_request_head(conn_)) {
		logger_error("transfer_request_head error");
		return false;
	} else if (!transfer_request_body(conn_)) {
		logger_error("transfer_request_body error");
		return false;
	} else {
		return transfer_response();
	}
}

bool http_transfer::transfer_response(void) {
	assert(client_);
	if (!client_->read_head()) {
		logger_error("read response head error");
		return false;
	}

	bool keep_alive = false; // xxxx
	client_->header_update("Connection", "Close");

	acl::string header;
	client_->sprint_header(header, NULL);
	if (header.empty()) {
		logger_error("response header empty");
		return false;
	}

	header += "\r\n";

	logger_debug(DEBUG_RES, 2, "response head:\r\n[%s]", header.c_str());

	acl::ostream* out = &res_.getOutputStream();
	if (out->write(header) == -1) {
		logger_error("send response head error=%s", acl::last_serror());
		return false;
	}

	//acl::http_client* out_client = res_->getClient();
	//assert(out_client);

	long long length = client_->body_length();
	if (length == 0) {
		return client_->is_server_keep_alive() && keep_alive;
	}

	HTTP_HDR_RES* hdr_res = client_->get_respond_head(NULL);
	assert(hdr_res);
	bool chunked = hdr_res->hdr.chunked ? true : false;

	char buf[8192];

	while (true) {
		int ret = client_->read_body(buf, sizeof(buf));
		if (ret <= 0) {
			break;
		} else if (!chunked) {
			if (out->write(buf, ret) == -1) {
				logger_error("send response body error=%s, rfd=%d, wfd=%d",
					acl::last_serror(), client_->get_stream().sock_handle(),
					res_client_->get_stream().sock_handle());
				return false;
			}
		} else if (!res_client_->write_chunk(*out, buf, ret)) {
			logger_error("reply chunk body error=%s, rfd=%d, wfd=%d",
				acl::last_serror(), client_->get_stream().sock_handle(),
				res_client_->get_stream().sock_handle());
			return false;
		}
	}

	if (chunked) {
		if (!res_client_->write_chunk_trailer(*out)) {
			logger_error("write chunked trailer error=%s, rfd=%d, wfd=%d",
				acl::last_serror(), client_->get_stream().sock_handle(),
				res_client_->get_stream().sock_handle());
			return false;
		}
	}

	return client_->is_server_keep_alive() && false;
}
