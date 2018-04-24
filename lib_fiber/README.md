本模块(lib_fiber)为基于协程方式进行高并发、高性能开发的网络协程库。使用者可以象创建线程一样创建协程，相对于线程而言，协程更为“轻量”，因此使用者可以创建大量(成千上万)的协程。每个协程可以与一个网络连接绑定；同时使用者可以采用“同步”思维方式编写网络程序，而不必象非阻塞程序一样采用异步回调方式，因此使用者使用起来并没有多大编程复杂度。
本网络协程库的协程部分是基于 Russ Cox (golang 的协程作者) 在 2005 年实现的 libtask，libtask 实现了协程编程的基本原型，lib_fiber 一方面使协程编程接口更加简单易用(用户可以直接调用 acl_fiber_create 创建协程)，另一方面 lib_fiber 实现了线程安全的协程库，通过给每个线程一个独立的协程调度器，从而方便用户使用多核，此外，lib_fiber 还增加了基于协程的信号量、协程局部变量等功能。

本协程库支持的平台有：Linux/FreeBSD/MacOS/Windows，支持的事件引擎如下：  

|Event|Linux|BSD|Mac|Windows|
|-----|----|------|---|---|
|<b>select</b>|yes|yes|yes|yes|
|<b>poll</b>|yes|yes|yes|yes|
|<b>epoll</b>|yes|no|no|no|
|<b>kqueue</b>|no|yes|yes|no|
|<b>iocp</b>|no|no|no|yes|
|<b>Win GUI message</b>|no|no|no|yes|

### 示例一  
下面是一个简单使用网络协程库编写的一个**简单的高并发服务器**：  

```c++
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"

static int  __nconnect = 0;
static int  __count = 0;
static char __listen_ip[64];
static int  __listen_port = 9001;
static int  __listen_qlen = 64;
static int  __rw_timeout = 0;
static int  __echo_data  = 1;
static int  __stack_size = 32000;

static int check_read(int fd, int timeout)
{
	struct pollfd pfd;
	int n;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLIN;

	n = poll(&pfd, 1, timeout);
	if (n < 0) {
		printf("poll error: %s\r\n", strerror(errno));
		return -1;
	}

	if (n == 0)
		return 0;
	if (pfd.revents & POLLIN)
		return 1;
	else
		return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	int  *cfd = (int *) ctx;
	char  buf[8192];
	int   ret;

	printf("client fiber-%d: fd: %d\r\n", acl_fiber_self(), *cfd);

	while (1) {
		if (__rw_timeout > 0) {
			ret = check_read(*cfd, 10000);
			if (ret < 0)
				break;
			if (ret == 0)
				continue;
		}

		ret = read(*cfd, buf, sizeof(buf));
		if (ret == 0) {
			printf("read close by peer fd: %d\r\n", *cfd);
			break;
		} else if (ret < 0) {
			if (errno == EINTR) {
				printf("catch a EINTR signal\r\n");
				continue;
			}

			printf("read error %s, fd: %d\n", strerror(errno), *cfd);
			break;
		}

		__count++;

		if (!__echo_data)
			continue;

		if (write(*cfd, buf, ret) < 0) {
			if (errno == EINTR)
				continue;
			printf("write error, fd: %d\r\n", *cfd);
			break;
		}
	}

	printf("close %d\r\n", *cfd);
	close(*cfd);
	free(cfd);

	if (--__nconnect == 0) {
		printf("\r\n----total read/write: %d----\r\n", __count);
		__count = 0;
	}
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  lfd, on = 1;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(__listen_port);
	sa.sin_addr.s_addr = inet_addr(__listen_ip);

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0)
		abort();

	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		printf("setsockopt error %s\r\n", strerror(errno));
		exit (1);
	}

	if (bind(lfd, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		printf("bind error %s\r\n", strerror(errno));
		exit (1);
	}

	if (listen(lfd, 128) < 0) {
		printf("listen error %s\r\n", strerror(errno));
		exit (1);
	}

	printf("fiber-%d listen %s:%d ok\r\n",
		acl_fiber_self(), __listen_ip, __listen_port);

	for (;;) {
		int len = sizeof(sa), *fd;
		int cfd = accept(lfd, (struct sockaddr *)& sa, (socklen_t *) &len);
		if (cfd < 0) {
			printf("accept error %s\r\n", strerror(errno));
			break;
		}

		fd = malloc(sizeof(int));
		assert(fd != NULL);
		*fd = cfd;

		__nconnect++;
		printf("accept one, fd: %d\r\n", cfd);

		// 将接收到的客户端连接传递给新创建的协程
		acl_fiber_create(echo_client, fd, __stack_size);
	}

	close(lfd);
	exit(0);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -s listen_ip\r\n"
		"  -p listen_port\r\n"
		"  -r rw_timeout\r\n"
		"  -q listen_queue\r\n"
		"  -z stack_size\r\n"
		"  -S [if using single IO, default: no]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;

	snprintf(__listen_ip, sizeof(__listen_ip), "%s", "127.0.0.1");

	while ((ch = getopt(argc, argv, "hs:p:r:q:Sz:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__listen_ip, sizeof(__listen_ip), "%s", optarg);
			break;
		case 'p':
			__listen_port = atoi(optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'q':
			__listen_qlen = atoi(optarg);
			break;
		case 'S':
			__echo_data = 0;
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		default:
			break;
		}
	}

	signal(SIGPIPE, SIG_IGN);
	acl_msg_stdout_enable(1);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);

	// 创建监听协程
	acl_fiber_create(fiber_accept, NULL, 32768);

	printf("call fiber_schedule\r\n");

	// 开始协程调度过程
	acl_fiber_schedule();

	return 0;
}
```

### 示例二  
上面的例子中因为使用了系统原生的网络 API，所以感觉代码有些臃肿，下面的例子使用 acl 库中提供的网络 API，显得更为简单些：
```c++
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override
	void run(void)
	{
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());

		char buf[8192];
		while (true)
		{
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1)
				break;
			if (conn_->write(buf, ret) == -1)
				break;
		}

		delete conn_;
		delete this;
	}

private:
	acl::socket_stream* conn_;

	~fiber_client(void) {}
};

class fiber_server : public acl::fiber
{
public:
	fiber_server(acl::server_socket& ss) : ss_(ss) {}
	~fiber_server(void) {}

protected:
	// @override
	void run(void)
	{
		while (true)
		{
			acl::socket_stream* conn = ss_.accept();
			if (conn == NULL)
			{
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			printf("accept ok, fd: %d\r\n", conn->sock_handle());
			// create one fiber for one connection
			fiber_client* fc = new fiber_client(conn);
			// start the fiber
			fc->start();
			continue;
		}
	}

private:
	acl::server_socket& ss_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	fiber_server fs(ss);
	fs.start();		// start listen fiber

	acl::fiber::schedule();	// start fiber schedule

	return 0;
}
```
### 示例三  
如果使用C++11的特性，则示例二会更为简单，如下：
```c++
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static void fiber_client(acl::socket_stream* conn)
{
	printf("fiber-%d running\r\n", acl::fiber::self());

	char buf[8192];
	while (true)
	{
		int ret = conn->read(buf, sizeof(buf), false);
		if (ret == -1)
			break;
		if (conn->write(buf, ret) == -1)
			break;
	}

	delete conn;
}

static void fiber_server(acl::server_socket& ss)
{
	while (true)
	{
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		printf("accept ok, fd: %d\r\n", conn->sock_handle());

		go[=] {
			fiber_client(conn);
		};
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	go[&] {
		fiber_server(ss);
	};

	acl::fiber::schedule();	// start fiber schedule

	return 0;
}
```
### 参考  

 - 网络协程编程：http://zsxxsz.iteye.com/blog/2312043     
 - 用协程编写高并发网络服务：http://zsxxsz.iteye.com/blog/2309654    
 - 使用协程方式编写高并发的WEB服务：http://zsxxsz.iteye.com/blog/2309665
