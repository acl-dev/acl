#pragma once

class http_server : public acl::aio_accept_callback
{
public:
	http_server(acl::aio_handle& aio, int rw_timeout);
	~http_server(void);

	bool open(const char* addr);

protected:
	// @override
	bool accept_callback(acl::aio_socket_stream* client);

private:
	acl::aio_handle& aio_;
	acl::aio_listen_stream* listener_;
	int rw_timeout_;
};
