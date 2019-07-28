#pragma once

class http_rpc;

class http_client : public acl::aio_callback
{
public:
	http_client(acl::aio_socket_stream* conn, int buf_size);
	~http_client();

private:
	virtual bool read_wakeup();
	virtual bool write_callback();
	virtual bool timeout_callback();
	virtual void close_callback();
private:
	acl::aio_socket_stream* conn_;
	ACL_ASTREAM* stream_;
	acl::string res_hdr_;
	acl::string res_hdr2_;
	acl::string res_body_;
	bool keep_alive_;
};
