#include "stdafx.h"
#include "http_client.h"
#include "http_server.h"

http_server::http_server(acl::aio_handle& aio, int rw_timeout)
: aio_(aio)
, listener_(NULL)
, rw_timeout_(rw_timeout)
{
}

http_server::~http_server(void)
{
	if (listener_)
		listener_->destroy();
}

bool http_server::open(const char* addr)
{
	listener_ = new acl::aio_listen_stream(&aio_);
	if (listener_->open(addr) == false) {
		logger_error("open %s error %s", addr, acl::last_serror());
		listener_->destroy();
		listener_ = NULL;
		return false;
	}

	listener_->add_accept_callback(this);
	logger("master manager started on %s", addr);
	return true;
}

bool http_server::accept_callback(acl::aio_socket_stream* client)
{
	http_client* hc = new http_client(client, rw_timeout_);
	hc->wait();
	return true;
}
