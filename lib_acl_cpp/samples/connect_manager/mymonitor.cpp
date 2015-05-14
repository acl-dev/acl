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
// ���������̣�������ĳһ�����߳̿ռ���

void mymonitor::sio_check(acl::check_client& checker,
	acl::socket_stream& conn)
{
	// ͬ��������
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
// �����������̣������ڼ�����̵߳Ŀռ���

void mymonitor::nio_check(acl::check_client& checker,
	acl::aio_socket_stream& conn)
{
	// �����첽������
	check_async* callback = new check_async(checker);

	// ע������� IO ������̵Ļص�����
	conn.add_close_callback(callback);
	conn.add_read_callback(callback);
	conn.add_timeout_callback(callback);

	int timeout = 10;

	// �첽��ȡһ�����ݣ�ͬʱҪ�󲻱��� \r\n

	conn.gets(timeout);
}
