#include "stdafx.h"
#include "http_client.h"
#include "http_stream.h"

http_stream::http_stream(acl::aio_handle& handle, http_client& client)
: http_aclient(handle, NULL)
, client_(client)
{
}

http_stream::~http_stream(void)
{
	printf("delete http_stream!\r\n");
}

void http_stream::destroy(void)
{
	client_.on_destroy(this);
}

bool http_stream::on_connect(void)
{
	client_.on_connect(*this);
	printf(">>> begin send_request\r\n");

	//this->ws_handshake();
	this->send_request(NULL, 0);

	return true;
}

void http_stream::on_disconnect(void)
{
	client_.on_disconnect(*this);
}

void http_stream::on_ns_failed(void)
{
	client_.on_ns_failed(*this);
}

void http_stream::on_connect_timeout(void)
{
	client_.on_connect_timeout(*this);
}

void http_stream::on_connect_failed(void)
{
	client_.on_connect_failed(*this);
}

bool http_stream::on_read_timeout(void)
{
	return client_.on_read_timeout(*this);
}

bool http_stream::on_http_res_hdr(const acl::http_header& header)
{
	return client_.on_http_res_hdr(*this, header);
}

bool http_stream::on_http_res_body(char* data, size_t dlen)
{
	return client_.on_http_res_body(*this, data, dlen);
}

bool http_stream::on_http_res_finish(bool success)
{
	return client_.on_http_res_finish(*this, success);
}
