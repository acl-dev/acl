#include "stdafx.h"
#include <memory>
#include <iostream>
#include "master_service.h"
#include "https_client.h"

#define	DEBUG	100

https_client::https_client(acl::ostream& out, acl::sslbase_conf* conf)
: out_(out)
, ssl_conf_(conf)
{
}

https_client::~https_client()
{
}

bool https_client::connect_server(const acl::string& server_addr,
	acl::http_client& client)
{
	// 先查本地映射表中有没有映射项
	master_service& ms = acl::singleton2<master_service>::get_instance();
	const char* addr = ms.get_addr(server_addr.c_str());
	if (addr == NULL) {
		addr = server_addr.c_str();
	}

	if (!client.open(addr, 60, 60, true)) {
		out_.format("connect server %s error", addr);
		return false;
	} else {
		logger_debug(DEBUG, 1, "connect server ok");
	}

	if (ssl_conf_) {
		logger_debug(DEBUG, 1, "begin open ssl");

		acl::sslbase_io* ssl = ssl_conf_->create(false);
		if (client.get_stream().setup_hook(ssl) == ssl) {
			out_.puts("open ssl client error");
			ssl->destroy();
			return false;
		} else {
			logger_debug(DEBUG, 1, "open ssl ok");
		}
	}

	return true;
}

bool https_client::http_request(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	const char* host = req.getRemoteHost();
	if (host == NULL || *host == 0) {
		host = req.getLocalAddr();
	}

	acl::string server_addr;
	if (ssl_conf_ == NULL) {
		server_addr.format("%s:80", host);
	} else {
		server_addr.format("%s:443", host);
	}

	std::auto_ptr<acl::http_client> backend(new acl::http_client);

	// 连接服务器
	if (!connect_server(server_addr, *backend)) {
		return false;
	}

	acl::http_client* front = req.getClient();

	// 可以在此处强制替换 HTTP 请求头中的 HOST 字段
	//front->header_update("Host", "www.test.com:443");

	// 取得  HTTP 请求头数据
	acl::string req_hdr;
	front->sprint_header(req_hdr, NULL);

	printf(">>>req_hdr: %s\r\n", req_hdr.c_str());

	// 转发 HTTP 请求头至服务器
	if (backend->get_ostream().write(req_hdr) == -1) {
		out_.puts(">>>>write header error");
		return false;
	}
	if (backend->get_ostream().write("\r\n") == -1) {
		out_.puts(">>>>write CRLF error");
		return false;
	}

	// 如果还有数据体，则转发请求的数据体给服务器
	long long int len = req.getContentLength();

	if (len > 0) {
		char req_body[8192];
		int ret;

		while (true) {
			ret = front->read_body(req_body, sizeof(req_body) - 1);
			if (ret < 0) {
				out_.puts(">>> read req body error");
				return false;
			}
			if (ret == 0) {
				break;
			}

			req_body[ret] = 0;
			out_.write(req_body, ret);
			if (backend->get_ostream().write(req_body, ret) == -1) {
				out_.puts(">>> write body to server error");
				return false;
			}
		}
	}
	out_.puts("");

	// 开始从后端服务器读取响应头和响应体数据

	out_.puts(">>>> begin read res header<<<<<");
	if (!backend->read_head()) {
		out_.puts(">>>>>>>>read header error<<<<<<<<<<");
		return false;
	}

	acl::string res_hdr;
	backend->sprint_header(res_hdr, NULL);
	if (res.getOutputStream().write(res_hdr) == -1) {
		out_.puts(">>>>>write res hdr error<<<<<<");
		return false;
	}

	if (res.getOutputStream().write("\r\n") == -1) {
		out_.puts(">>>write CRLF error");
		return false;
	}

	out_.puts("------------------res hdr----------------");
	out_.write(res_hdr);
	out_.puts("------------------res hdr end------------");

	char buf[8192];

	while (true) {
		int ret = backend->read_body(buf, sizeof(buf) - 1);
		if (ret < 0) {
			logger_error(">>> read body error");
			return false;
		} else if (ret == 0) {
			break;
		}

		buf[ret] = 0;
		out_.write(buf, ret);
		if (res.getOutputStream().write(buf, ret) == -1) {
			out_.puts(">>> write res body error");
			return false;
		}
	}

	const char* ptr = backend->header_value("Transfer-Encoding");
	if (ptr == NULL || *ptr == 0 || strcasecmp(ptr, "chunked") != 0) {
		return backend->keep_alive();
	}

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(NULL, 0);
}
