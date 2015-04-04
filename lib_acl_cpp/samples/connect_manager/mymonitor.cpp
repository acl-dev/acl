#include "stdafx.h"
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
	if (proto_ == "http")
		sio_check_http(checker, conn);
	else if (proto_ == "pop3")
		sio_check_pop3(checker, conn);
	else
	{
		printf("unknown protocol: %s\r\n", proto_.c_str());
		checker.set_alive(true);
	}
}

void mymonitor::sio_check_pop3(acl::check_client& checker,
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

void mymonitor::sio_check_http(acl::check_client& checker,
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

//////////////////////////////////////////////////////////////////////////////
// 非阻塞检测过程，运行在检测器线程的空间中

void mymonitor::nio_check(acl::check_client& checker,
	acl::aio_socket_stream& conn)
{
	checker_ = &checker;

	// 注册非阻塞 IO 处理过程的回调过程
	conn.add_close_callback(this);
	conn.add_read_callback(this);
	conn.add_timeout_callback(this);

	int timeout = 10;

	// 异步读取一行数据，同时要求不保留 \r\n

	conn.gets(timeout);
}

bool mymonitor::read_callback(char* data, int len)
{
	// 因为 acl 的异步 IO 读到的数据肯定会在所读到的数据最后添加 \0，所以直接
	// 当字符串比较在此处是安全的

	if (strncasecmp(data, "+OK", 3) == 0)
	{
		// 将服务端连接置为存活状态
		checker_->set_alive(true);

		// 主动关闭该检测连接
		checker_->close();

		// 此处返回 true 或 false 都可以，因为上面已经主动要求关闭检测连接
		printf(">>> NIO_CHECK SERVER(%s) OK: %s, len: %d <<<\r\n",
			checker_->get_addr(), data, len);
		return true;
	}

	// 将服务端置为不可用状态
	checker_->set_alive(false);

	printf(">>> NIO_CHECK SERVER(%s) ERROR: %s, len: %d <<<\r\n",
		checker_->get_addr(), data, len);

	// 返回 false 通知框架自动关闭该连接
	return false;
}

bool mymonitor::timeout_callback()
{
	// 读超时，所以直接将连接置为不可用
	checker_->set_alive(false);

	// 返回 false 通过框架自动关闭该检测连接
	return false;
}

void mymonitor::close_callback()
{

}
