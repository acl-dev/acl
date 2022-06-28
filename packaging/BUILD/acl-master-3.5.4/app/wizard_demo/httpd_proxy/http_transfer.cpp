#include "stdafx.h"
#include "http_transfer.h"

http_transfer::http_transfer(acl::http_method_t method, request_t& req,
	response_t& res, int port)
: port_(port)
, method_(method)
, req_(req)
, res_(res)
, client_(NULL)
{
	box_ = new acl::fiber_tbox<bool>;

	acl::socket_stream& sin = req_.getSocketStream();
	req_in_.open(sin.sock_handle());

	acl::socket_stream& sout = res_.getSocketStream();
	res_out_.open(sout.sock_handle());
	res_client_ = res_.getClient();
}

http_transfer::~http_transfer(void) {
	req_in_.unbind_sock();
	res_out_.unbind_sock();

	delete client_;
	delete box_;
}

void http_transfer::wait(bool* keep_alive) {
	bool* res = box_->pop();
	assert(res);
	*keep_alive = *res;
	delete res;
}

void http_transfer::run(void) {
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

	box_->push(res);
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

	if (!conn.open(addr, 0, 0)) {
		logger_error("connect %s error %s",
			addr.c_str(), acl::last_serror());
		return false;
	}

	logger("connect %s ok", addr.c_str());

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

	printf(">>>send head: [%s]\r\n", header.c_str());
	return true;
}

bool http_transfer::transfer_request_body(acl::socket_stream& conn) {
	long long length = req_.getContentLength();
	if (length <= 0) {
		return true;
	}

	long long n = 0;
	char buf[8192];

	while (n < length) {
		int ret = req_in_.read(buf, sizeof(buf), false);
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

	printf("response head:\r\n[%s]\r\n", header.c_str());

	//acl::ostream* out = &res_->getOutputStream();
	if (res_out_.write(header) == -1) {
		logger_error("send response head error");
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
		} else if (chunked) {
			if (!res_client_->write_chunk(res_out_, buf, ret)) {
				logger_error("send response body error");
				return false;
			}
		} else if (res_out_.write(buf, ret) == -1) {
			logger_error("send response body error");
			return false;
		}
	}

	if (chunked) {
		if (!res_client_->write_chunk_trailer(res_out_)) {
			logger_error("write chunked trailer error");
			return false;
		}
	}

	return client_->is_server_keep_alive() && false;
}
