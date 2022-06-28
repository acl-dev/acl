#include "stdafx.h"
#include "check_async.h"

check_async::check_async(acl::check_client& checker)
: checker_(checker)
{
}

check_async::~check_async(void)
{
}

bool check_async::read_callback(char* data, int len)
{
	// 因为 acl 的异步 IO 读到的数据肯定会在所读到的数据最后添加 \0，
	// 所以直接当字符串比较在此处是安全的

	if (strncasecmp(data, "+OK", 3) == 0)
	{
		// 发送 QUIT 命令
		checker_.get_conn().format("QUIT\r\n");

		// 将服务端连接置为存活状态
		checker_.set_alive(true);

		// 主动关闭该检测连接
		checker_.close();

		// 此处返回 true 或 false 都可以，因为上面已经主动要求关闭检测连接
		printf(">>> NIO_CHECK SERVER(%s) OK: %s, len: %d <<<\r\n",
			checker_.get_addr(), data, len);
		return true;
	}

	// 发送 QUIT 命令
	checker_.get_conn().format("QUIT\r\n");

	// 将服务端置为不可用状态
	checker_.set_alive(false);

	printf(">>> NIO_CHECK SERVER(%s) ERROR: %s, len: %d <<<\r\n",
		checker_.get_addr(), data, len);

	// 返回 false 通知框架自动关闭该连接
	return false;
}

bool check_async::timeout_callback()
{
	// 读超时，所以直接将连接置为不可用
	checker_.set_alive(false);

	// 返回 false 通过框架自动关闭该检测连接
	return false;
}

void check_async::close_callback()
{
	// 动态创建对象，需要动态删除
	delete this;
}
