#include "stdafx.h"
#include "util.h"
#include "thread_client.h"

thread_client::thread_client(const char* server_addr, bool keep_alive,
	int count, int length)
: server_addr_(server_addr)
, keep_alive_(keep_alive)
, count_(count)
, length_(length)
{
}

thread_client::~thread_client()
{
}

void* thread_client::run()
{
	acl::socket_stream conn;

	// 连接远程服务器
	if (conn.open(server_addr_, 10, 10) == false)
	{
		printf("connect %s error %s\r\n", server_addr_.c_str(),
			acl::last_serror());
		return NULL;
	}

	char* data = (char*) acl_mymalloc(length_);
	memset(data, 'X', length_);
	data[length_ - 1] = '\n';

	acl::string buf;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int   i = 0;
	for (; i < count_; i++)
	{
		// 向服务器写一行数据
		if (conn.write(data, length_) == -1)
		{
			printf("write to %s error %s\r\n",
				server_addr_.c_str(), acl::last_serror());
			break;
		}

		// 从服务器读一行数据
		if (conn.gets(buf) == false)
		{
			printf("gets from %s error %s\r\n",
				server_addr_.c_str(), acl::last_serror());
			break;
		}

		if (i % 1000 == 0)
		{
			buf.format("total: %d, curr: %d", count_, i);
			ACL_METER_TIME(buf.c_str());
		}

		if (keep_alive_)
			continue;

		// 如果采用短连接，则先关闭连接再打开连接
		conn.close();
		if (conn.open(server_addr_.c_str(), 10, 10) == false)
		{
			printf("connect %s error %s\r\n", server_addr_.c_str(),
				acl::last_serror());
			break;
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, curr: %d, spent: %.2f, speed: %.2f\r\n",
		count_, i, spent, (i * 1000) / (spent > 1 ? spent : 1));

	acl_myfree(data);
	return NULL;
}
