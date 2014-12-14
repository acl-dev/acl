#include "stdafx.h"

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s server_addr\r\n", argv[0]);
		return 0;
	}

	acl::socket_stream stream;

	if (stream.open(argv[1], 10, 0) == false)
	{
		printf("connect %s error: %s\r\n",
			argv[1], acl::last_serror());
		return 1;
	}

	acl::url_coder coder;
	coder.set("count", 10);
	coder.set("used", 0);
	coder.set("pid", (int) getpid());
	coder.set("max_threads", 10);
	coder.set("curr_threads", 1);
	coder.set("busy_threads", 0);
	coder.set("qlen", 0);
	coder.set("type", "default");

	if (stream.format("%s\r\n", coder.to_string().c_str()) == -1)
	{
		printf("write %s to %s error\r\n", coder.to_string().c_str(),
			argv[1]);
		return 1;
	}

	char  buf[1024];
	int   fd;
	int   ret = acl_read_fd(stream.sock_handle(), buf, sizeof(buf) - 1, &fd);
	if (ret == -1)
	{
		printf("read from %d error %s\r\n", stream.sock_handle(),
			acl::last_serror());
		return 1;
	}

	buf[ret] = 0;
	printf("accept fd: %d, buf: %s\r\n", fd, buf);

	acl::string line;
	acl::socket_stream conn;
	if (conn.open(fd) == false)
	{
		printf("open fd %d error\r\n", fd);
		return 1;
	}

	while (true)
	{
		if (conn.gets(line) == false)
		{
			printf("gets error %s\r\n", acl::last_serror());
			break;
		}

		if (line == "quit")
		{
			conn.format("Bye!\r\n");
			break;
		}

		if (conn.format("%s\r\n", line.c_str()) == -1)
		{
			printf("write error\r\n");
			break;
		}

		line.clear();
	}

	printf("over now\r\n");

	return 0;
}
