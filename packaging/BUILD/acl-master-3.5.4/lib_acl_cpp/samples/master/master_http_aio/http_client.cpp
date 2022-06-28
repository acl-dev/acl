#include "stdafx.h"
#include "http_client.h"

http_client::http_client(acl::aio_socket_stream* conn, int buf_size)
: conn_(conn)
, res_body_(buf_size)
, keep_alive_(false)
{
	stream_ = conn->get_astream();
	for (int i = 0; i < buf_size; i++)
		res_body_ << 'X';
	res_hdr_.format("HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"Connection: keep-alive\r\n\r\n",
		buf_size);
	res_hdr2_.format("HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n\r\n",
		buf_size);
}

http_client::~http_client()
{
}

bool http_client::write_callback()
{
	return true;
}

bool http_client::timeout_callback()
{
	return false;
}

void http_client::close_callback()
{
	//logger("connection closed now, fd: %d", conn_->get_socket());
	delete this;
}

bool http_client::read_wakeup()
{
#define	STR	acl_vstring_str

	while (true)
	{
		ACL_VSTRING* buf = acl_aio_gets_nonl_peek(stream_);
		if (buf == NULL)
			return true;

		if (ACL_VSTRING_LEN(buf) == 0)
			break;

		//logger("buf: |%s|", STR(buf));
		if (strcasecmp(STR(buf), "Connection: keep-alive") == 0)
			keep_alive_ = true;

		ACL_VSTRING_RESET(buf);
	}

	if (keep_alive_)
	{
		keep_alive_ = false;
		conn_->write(res_hdr_.c_str(), (int) res_hdr_.length());
		conn_->write(res_body_.c_str(), (int) res_body_.length());
		return true;
	}
	else
	{
		conn_->write(res_hdr2_.c_str(), (int) res_hdr2_.length());
		conn_->write(res_body_.c_str(), (int) res_body_.length());
		return false;
	}
}
