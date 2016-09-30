#include "stdafx.h"
#include "http_servlet.h"
#include "configure.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

void master_service::on_accept(acl::socket_stream& conn)
{
	logger("connect from %s, fd %d", conn.get_peer(), conn.sock_handle());

	conn.set_rw_timeout(0);

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(&conn, &session);

	// charset: big5, gb2312, gb18030, gbk, utf-8
	servlet.setLocalCharset("utf-8");

	while(servlet.doRun()) {}

	logger("disconnect from %s", conn.get_peer());
}

void master_service::proc_pre_jail(void)
{
	logger(">>>proc_pre_jail<<<");
}

void master_service::proc_on_init(void)
{
	logger(">>>proc_on_init<<<");
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
}
