#include "stdafx.h"
#include "check_sync.h"

check_sync::check_sync(void)
{
}

check_sync::~check_sync(void)
{
}

void check_sync::sio_check_pop3(acl::check_client& checker,
	acl::socket_stream& conn)
{
	acl::string buf;
	if (conn.gets(buf) == false)
	{
		checker.set_alive(false);
		return;
	}

	if (strncasecmp(buf.c_str(), "+OK", 3) == 0)
	{
		printf(">>> SIO_CHECK SERVER(%s) OK: %s, len: %d <<<\r\n",
			checker.get_addr(), buf.c_str(), (int) buf.length());
		checker.set_alive(true);
	}
	else
	{
		printf(">>> SIO_CHECK SERVER(%s) ERROR: %s, len: %d <<<\r\n",
			checker.get_addr(), buf.c_str(), (int) buf.length());
		checker.set_alive(false);
	}
}

void check_sync::sio_check_http(acl::check_client& checker,
	acl::socket_stream& conn)
{
	acl::http_request req(&conn, 60, false);
	acl::http_header& hdr = req.request_header();

	acl::string ctype("text/plain; charset=gbk");
	hdr.set_url("/").set_content_type("text/plain; charset=gbk");
	if (req.request(NULL, 0) == false)
	{
		printf(">>> send request error\r\n");
		checker.set_alive(false);
		return;
	}

	acl::string buf;
	if (req.get_body(buf) == false)
	{
		printf(">>> HTTP get_body ERROR, SERVER: %s <<<\r\n",
			checker.get_addr());
		checker.set_alive(false);
		return;
	}

	int status = req.http_status();
	if (status == 200 || status == 404)
	{
		printf(">>> SIO_CHECK HTTP SERVER(%s) OK: %d <<<\r\n",
			checker.get_addr(), status);
		checker.set_alive(true);
	}
	else
	{
		printf(">>> SIO_CHECK HTTP SERVER(%s) ERROR: %d <<<\r\n",
			checker.get_addr(), status);
		checker.set_alive(false);
	}
}
