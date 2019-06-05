#pragma once
#include "../acl_cpp_define.hpp"
#include "../stream/aio_socket_stream.hpp"

struct HTTP_HDR;
struct HTTP_HDR_RES;
struct HTTP_RES;
struct HTTP_HDR_REQ;
struct HTTP_REQ;

struct ACL_ASTREAM;

namespace acl {

class aio_handle;
class aio_socket_stream;
class polarssl_conf;
class polarssl_io;
class http_header;

class ACL_CPP_API http_aclient : public aio_open_callback
{
public:
	http_aclient(aio_handle& handle, polarssl_conf* ssl_conf = NULL);
	~http_aclient(void);

	http_header& request_header(void);

	bool open(const char* addr, int conn_timeout);

	virtual bool on_connect(void) = 0;
	virtual void on_disconnect(void) = 0;
	virtual void on_connect_timeout(void) = 0;
	virtual void on_connect_failed(void) = 0;

	virtual bool on_http_res_hdr(const http_header& header) = 0;
	virtual bool on_http_res_body(char* data, size_t dlen) = 0;
	virtual bool on_http_res_finish(void) = 0;

protected:
	void send_request(const void* body, size_t len);

protected:
	// @override
	bool open_callback(void);

	// @override
	bool timeout_callback(void);

	// @override
	void close_callback(void);

	// @override
	bool read_wakeup(void);

	// @override
	bool read_callback(char* data, int len);

protected:
	aio_handle&        handle_;
	polarssl_conf*     ssl_conf_;
	aio_socket_stream* conn_;
	http_header*       header_;
	HTTP_HDR_RES*      hdr_res_;
	HTTP_RES*          http_res_;

private:
	static int connect_callback(ACL_ASTREAM* stream, void* ctx);
	static int http_res_hdr_cllback(int status, void* ctx);
	static int http_res_callback(int status, char* data, int dlen, void* ctx);
};

} // namespace acl
