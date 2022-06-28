#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/xml.hpp"
#include "acl_cpp/stdlib/json.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/ostream.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_ctype.hpp"
#include "acl_cpp/http/http_pipe.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/http_response.hpp"
#endif

namespace acl
{

http_response::http_response(socket_stream* client)
{
	debug_     = false;
	header_ok_ = false;
	head_sent_ = false;
	client_    = NEW http_client(client, false, true);
}

http_response::~http_response(void)
{
	close();
}

void http_response::close()
{
	if (client_) {
		delete client_;
		client_ = NULL;
	}
	head_sent_ = false;
}

http_header& http_response::response_header()
{
	return header_;
}

http_client* http_response::get_client() const
{
	return client_;
}

bool http_response::read_header()
{
	if (client_) {
		// 在读 HTTP 请求头时将此标志重置，以便于在长连接的响应
		// 过程中可以重复响应 HTTP 头
		head_sent_ = false;
		client_->reset();
		header_.reset();
	} else {
		logger_error("client_ not opened");
		header_ok_ = false;
		return false;
	}

	// 读取客户端的请求头并进行分析

	if (!client_->read_head()) {
		close();
		header_ok_ = false;
		return false;
	}
	header_ok_ = true;
	return true;
}

http_pipe* http_response::get_pipe(const char* to_charset)
{
	if (to_charset == NULL || *to_charset == 0) {
		return NULL;
	}

	// 需要获得响应头字符集信息
	const char* ptr = client_->header_value("Content-Type");
	if (ptr == NULL || *ptr == 0) {
		return NULL;
	}

#if !defined(ACL_MIME_DISABLE)

	http_ctype ctype;
	ctype.parse(ptr);

	const char* from_charset = ctype.get_charset();

	if (from_charset && strcasecmp(from_charset, to_charset) != 0) {
		http_pipe* hp = NEW http_pipe();
		hp->set_charset(from_charset, to_charset);
		return hp;
	} else {
		return NULL;
	}
#else
	return NULL;
#endif // !defined(ACL_MIME_DISABLE)
}

bool http_response::get_body(xml& out, const char* to_charset /* = NULL */)
{
	if (!header_ok_) {
		logger_error("header not read yet");
		return false;
	} else if (client_->body_length() == 0) {
		return true;
	} else if (client_->body_length() < 0) {
		const char* method = client_->request_method();
		if (method && (strcmp(method, "GET") == 0
			|| strcmp(method, "CONNECT") == 0)) {

			return true;
		}

		logger_error("client request body length(%d) invalid",
			(int) client_->body_length());
		return false;
	}

	if (debug_) {
		client_->print_header("----request---");
	}

	http_pipe* hp = get_pipe(to_charset);
	if (hp) {
		hp->append(&out);
	}

	string buf;
	int   ret;

	while (true) {
		// 循环读取客户端请求数据体
		ret = client_->read_body(buf);
		if (ret == 0) {
			break;
		}
		if (ret < 0) {
			logger_error("read client body error");
			close();
			return false;
		}

		// 流式分析 xml 格式的数据体
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.update(buf.c_str());
		}
		if (debug_) {
			printf("%s", buf.c_str());
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_response::get_body(json& out, const char* to_charset /* = NULL */)
{
	if (!header_ok_) {
		logger_error("header not read yet");
		return false;
	} else if (client_->body_length() == 0) {
		return true;
	}
	else if (client_->body_length() < 0) {
		const char* method = client_->request_method();
		if (method && (strcmp(method, "GET") == 0
			|| strcmp(method, "CONNECT") == 0)) {

			return true;
		}

		logger_error("client request body length(%d) invalid",
			(int) client_->body_length());
		return false;
	}

	if (debug_) {
		client_->print_header("----request---");
	}

	http_pipe* hp = get_pipe(to_charset);
	if (hp) {
		hp->append(&out);
	}

	string buf;
	int   ret;

	while (true) {
		// 循环读取客户端请求数据体
		ret = client_->read_body(buf);
		if (ret == 0) {
			break;
		}
		if (ret < 0) {
			logger_error("read client body error");
			close();
			return false;
		}

		// 流式分析 json 格式的数据体
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.update(buf.c_str());
		}
		if (debug_) {
			printf("%s", buf.c_str());
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_response::get_body(string& out, const char* to_charset /* = NULL */)
{
	if (!header_ok_) {
		logger_error("header not read yet");
		return false;
	} else if (client_->body_length() == 0) {
		return true;
	} else if (client_->body_length() < 0) {
		const char* method = client_->request_method();
		if (method && (strcmp(method, "GET") == 0
			|| strcmp(method, "CONNECT") == 0)) {

			return true;
		}

		logger_error("client request body length(%d) invalid",
			(int) client_->body_length());
		return false;
	}

	http_pipe* hp = get_pipe(to_charset);
	if (hp) {
		pipe_string ps(out);
		hp->append(&ps);
	}

	string buf;
	int    ret;
	// 读 HTTP 请求体
	while (true) {
		ret = client_->read_body(buf);
		if (ret < 0) {
			close();
			break;
		} else if (ret == 0) {
			break;
		}
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.append(buf);
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}
	return true;
}

int http_response::read_body(char* buf, size_t size)
{
	if (!header_ok_) {
		logger_error("header not read yet");
		return -1;
	}
	return client_->read_body(buf, size);
}

bool http_response::response(const void* data, size_t len)
{
	if (client_ == NULL) {
		logger_error("conn not opened");
		return false;
	}

	// 第一次调用本函数时应先发送 HTTP 响应头
	if (!head_sent_) {
		if (!client_->write_head(header_)) {
			close();
			return false;
		}
		head_sent_ = true;
	}

	if (data == NULL || len == 0) {
		head_sent_ = false;
	}

	// 发送 HTTP 响应体数据
	if (!client_->write_body(data, len)) {
		close();
		return false;
	}

	return true;
}

} // namespace acl
