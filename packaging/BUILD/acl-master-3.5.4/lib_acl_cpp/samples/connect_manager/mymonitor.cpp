#include "stdafx.h"
#include "check_sync.h"
#include "check_async.h"
#include "mymonitor.h"

mymonitor::mymonitor(acl::connect_manager& manager, const acl::string& proto)
: connect_monitor(manager)
, proto_(proto)
{
}

mymonitor::~mymonitor(void)
{
}

//////////////////////////////////////////////////////////////////////////////
// 阻塞检测过程，运行在某一个子线程空间中

void mymonitor::sio_check(acl::check_client& checker,
	acl::socket_stream& conn)
{
	// 同步检测过程
	check_sync check;
	if (proto_ == "http")
		check.sio_check_http(checker, conn);
	else if (proto_ == "pop3")
		check.sio_check_pop3(checker, conn);
	else
	{
		printf("unknown protocol: %s\r\n", proto_.c_str());
		checker.set_alive(true);
	}
}

/////////////////////////////////////////////////////////////////////////////
// 非阻塞检测过程，运行在检测器线程的空间中

void mymonitor::nio_check(acl::check_client& checker,
	acl::aio_socket_stream& conn)
{
	// 创建异步检测对象
	check_async* callback = new check_async(checker);

	// 注册非阻塞 IO 处理过程的回调过程
	conn.add_close_callback(callback);
	conn.add_read_callback(callback);
	conn.add_timeout_callback(callback);

	int timeout = 10;

	// 异步读取一行数据，同时要求不保留 \r\n

	conn.gets(timeout);
}

/////////////////////////////////////////////////////////////////////////////

void mymonitor::on_connected(const acl::check_client& checker, double cost)
{
	printf("addr=%s, connected, cost=%.2f\r\n",
		checker.get_addr(), cost);
}

void mymonitor::on_refuse(const acl::check_client& checker, double cost)
{
	printf("addr=%s, connection refused, cost=%.2f\r\n",
		checker.get_addr(), cost);
}

void mymonitor::on_timeout(const acl::check_client& checker, double cost)
{
	printf("addr=%s, connection timeout, cost=%.2f\r\n",
		checker.get_addr(), cost);
}
