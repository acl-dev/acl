# The high performance coroutine library, supporting Linux/BSD/Mac/Windows

[中文说明](README.md)  

<!-- vim-markdown-toc GFM -->

* [About](#about)
* [Which IO events are supported?](#which-io-events-are-supported-)
* [SAMPLES](#samples)
    * [One server sample](#one-server-sample)
    * [One client sample](#one-client-sample)
	* [Windows GUI sample](#windows-gui-sample)
	* [More SAMPLES](#more-samples)
* [BUILDING](#building)
    * [On Unix](#on-unix)
	* [On Windows](#on-windows)
* [Benchmark](#benchmark)
* [API support](#api-support)
    * [Base API](#base-api)
    * [IO API](#io-api)
    * [Net API](#net-api)
    * [Channel API](#channel-api)
    * [Sync API](#sync-api)
* [About API Hook](#about-api-hook)
* [FAQ](#faq)

<!-- vim-markdown-toc -->

## About
The libfiber project comes from the coroutine module of the [acl project](#https://github.com/acl-dev/acl) in lib_fiber directory of which. It can be used on OS platfroms including Linux, FreeBSD, MacOS, and Windows, which supports select, poll, epoll, kqueue, iocp, and even Windows GUI messages for different platfrom. With libfiber, you can write network application services having the high performance and large cocurrent more easily than the traditional asynchronus  framework with event-driven model. <b>What's more</b>, with the help of libfiber, you can even write network module of the Windows GUI application written by MFC, wtl or other GUI framework on Windows in coroutine way. That's realy amazing.

## Which IO events are supported ?
The libfiber supports many events including select/poll/epoll/kqueue/iocp, and Windows GUI messages.

Event|Linux|BSD|Mac|Windows
-----|----|------|---|---
<b>select</b>|yes|yes|yes|yes
<b>poll</b>|yes|yes|yes|yes
<b>epoll</b>|yes|no|no|no
<b>kqueue</b>|no|yes|yes|no
<b>iocp</b>|no|no|no|yes
<b>Win GUI message</b>|no|no|no|yes

## SAMPLES

### One server sample
```C
// fiber_server.c

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fiber/lib_fiber.h"
#include "patch.h" // in the samples path

static size_t      __stack_size  = 128000;
static const char *__listen_ip   = "127.0.0.1";
static int         __listen_port = 9001;

static void fiber_client(ACL_FIBER *fb, void *ctx)
{
	SOCKET *pfd = (SOCKET *) ctx;
	char buf[8192];

	while (1) {
#if defined(_WIN32) || defined(_WIN64)
		int ret = acl_fiber_recv(*pfd, buf, sizeof(buf), 0);
#else
		int ret = recv(*pfd, buf, sizeof(buf), 0);
#endif
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			if (acl_fiber_last_error() == FIBER_EINTR) {
				continue;
			}
			break;
		}
#if defined(_WIN32) || defined(_WIN64)
		if (acl_fiber_send(*pfd, buf, ret, 0) < 0) {
#else
		if (send(*pfd, buf, ret, 0) < 0) {
#endif			
			break;
		}
	}

	socket_close(*pfd);
	free(pfd);
}

static void fiber_accept(ACL_FIBER *fb, void *ctx)
{
	const char *addr = (const char *) ctx;
	SOCKET lfd = socket_listen(__listen_ip, __listen_port);

	assert(lfd >= 0);

	for (;;) {
		SOCKET *pfd, cfd = socket_accept(lfd);
		if (cfd == INVALID_SOCKET) {
			printf("accept error %s\r\n", acl_fiber_last_serror());
			break;
		}
		pfd = (SOCKET *) malloc(sizeof(SOCKET));
		*pfd = cfd;

		// create and start one fiber to handle the client socket IO
		acl_fiber_create(fiber_client, pfd, __stack_size);
	}

	socket_close(lfd);
	exit (0);
}

// FIBER_EVENT_KERNEL represents the event type on
// Linux(epoll), BSD(kqueue), Mac(kqueue), Windows(iocp)
// FIBER_EVENT_POLL: poll on Linux/BSD/Mac/Windows
// FIBER_EVENT_SELECT: select on Linux/BSD/Mac/Windows
// FIBER_EVENT_WMSG: Win GUI message on Windows
// acl_fiber_create/acl_fiber_schedule_with are in `lib_fiber.h`.
// socket_listen/socket_accept/socket_close are in patch.c of the samples path.

int main(void)
{
	int event_mode = FIBER_EVENT_KERNEL;

#if defined(_WIN32) || defined(_WIN64)
	socket_init();
#endif

	// create one fiber to accept connections
	acl_fiber_create(fiber_accept, NULL, __stack_size);

	// start the fiber schedule process
	acl_fiber_schedule_with(event_mode);

#if defined(_WIN32) || defined(_WIN64)
	socket_end();
#endif

	return 0;
}
```

### One client sample

```C
// fiber_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fiber/lib_fiber.h"
#include "patch.h" // in the samples path

static const char *__server_ip   = "127.0.0.1";
static int         __server_port = 9001;

// socket_init/socket_end/socket_connect/socket_close are in patch.c of the samples path

static void fiber_client(ACL_FIBER *fb, void *ctx)
{
	SOCKET cfd = socket_connect(__server_ip, __server_port);
	const char *s = "hello world\r\n";
	char buf[8192];
	int i, ret;

	if (cfd == INVALID_SOCKET) {
		return;
	}

	for (i = 0; i < 1024; i++) {
#if defined(_WIN32) || defined(_WIN64)
		if (acl_fiber_send(cfd, s, strlen(s), 0) <= 0) {
#else
		if (send(cfd, s, strlen(s), 0) <= 0) {
#endif			
			printf("send error %s\r\n", acl_fiber_last_serror());
			break;
		}

#if defined(_WIN32) || defined(_WIN64)
		ret = acl_fiber_recv(cfd, buf, sizeof(buf), 0);
#else
		ret = recv(cfd, buf, sizeof(buf), 0);
#endif		
		if (ret <= 0) {
			break;
		}
	}

#if defined(_WIN32) || defined(_WIN64)
	acl_fiber_close(cfd);
#else
	close(cfd);
#endif
}

int main(void)
{
	int event_mode = FIBER_EVENT_KERNEL;
	size_t stack_size = 128000;

	int i;

#if defined(_WIN32) || defined(_WIN64)
	socket_init();
#endif

	for (i = 0; i < 100; i++) {
		acl_fiber_create(fiber_client, NULL, stack_size);
	}

	acl_fiber_schedule_with(event_mode);

#if defined(_WIN32) || defined(_WIN64)
	socket_end();
#endif

	return 0;
}
```

### Windows GUI sample
There is one Windows GUI sample with libfiber in [directory](samples/WinEchod). The screen shot is ![here](res/winecho.png)  

The server coroutine and client coroutine are all running in the same thread as the GUI, so you can operate the GUI object in server and client coroutine without worrying about the memroy collision problem. And you can write network process with sequence way, other than asynchronus callback way which is so horrible. With the libfirber for Windows GUI, the asynchronus API like CAsyncSocket should be discarded. The network APIs are intergrated with the Windows GUI seamlessly because the libfiber using GUI message pump as event driven internal.

### More SAMPLES
You can get more samples in [samples](https://github.com/acl-dev/acl/tree/master/lib_fiber/samples), which use many APIs in [acl project](https://github.com/acl-dev/acl/) library.
## BUILDING
### On Unix
```
$cd libfiber
$make
$cd samples
$make
```

<b>The simple Makefile shown below:</b>

```
fiber_server: fiber_server.c
	gcc -o fiber_server fiber_server.c patch.c -I{path_of_fiber_header} -L{path_of_fiber_lib) -lfiber -ldl -lpthread

fiber_client: fiber_client.c
	gcc -o fiber_client fiber_client.c patch.c -I{path_of_fiber_header} -L{path_of_fiber_lib) -lfiber -ldl -lpthread
```

### On Windows
You can open the [fiber_vc2012.sln](fiber_vc2012.sln)/ [fiber_vc2013.sln](fiber_vc2013.sln)/[fiber_vc2015.sln](fiber_vc2015.sln) with vc2012/vc2013/vc2015, and build the libfiber library and the [samples](samples) included.

## Benchmark
The picture below show the IOPS (io echo per-second) benchmark written by libfiber, comparing with the samples writen by [libmill](https://github.com/sustrik/libmill), golang and [libco](https://github.com/Tencent/libco). The samples written by libmill and libco are in [directory](benchmark), the sample written by golang is in [here](https://github.com/acl-dev/master-go/tree/master/examples/echo), and the sample written by libfiber is in [server sample directory](samples/server). The testing client is in [here](https://github.com/acl-dev/acl/tree/master/lib_fiber/samples/client2) from the [acl project](https://github.com/acl-dev/acl/).

![Benchmark](res/benchmark.png)

## API support  

### Base API  
- acl_fiber_create  
- acl_fiber_self  
- acl_fiber_status  
- acl_fiber_kill   
- acl_fiber_killed  
- acl_fiber_signal  
- acl_fiber_yield  
- acl_fiber_ready  
- acl_fiber_switch  
- acl_fiber_schedule_init  
- acl_fiber_schedule  
- acl_fiber_schedule_with  
- acl_fiber_scheduled  
- acl_fiber_schedule_stop  
- acl_fiber_set_specific  
- acl_fiber_get_specific  
- acl_fiber_delay  
- acl_fiber_last_error  
- acl_fiber_last_serror  

### IO API
- acl_fiber_recv  
- acl_fiber_recvfrom  
- acl_fiber_read  
- acl_fiber_readv  
- acl_fiber_recvmsg  
- acl_fiber_write  
- acl_fiber_writev  
- acl_fiber_send  
- acl_fiber_sendto  
- acl_fiber_sendmsg  
- acl_fiber_select  
- acl_fiber_poll  
- acl_fiber_close  

### Net API
- acl_fiber_socket  
- acl_fiber_listen  
- acl_fiber_accept  
- acl_fiber_connect  
- acl_fiber_gethostbyname_r
- acl_fiber_getaddrinfo
- acl_fiber_freeaddrinfo

### Channel API  
- acl_channel_create  
- acl_channel_free  
- acl_channel_send  
- acl_channel_send_nb  
- acl_channel_recv  
- acl_channel_recv_nb  
- acl_channel_sendp  
- acl_channel_recvp  
- acl_channel_sendp_nb  
- acl_channel_recvp_nb  
- acl_channel_sendul  
- acl_channel_recvul  
- acl_channel_sendul_nb  
- acl_channel_recvul_nb  

### Sync API
<b>ACL_FIBER_MUTEX</b>  
- acl_fiber_mutex_create  
- acl_fiber_mutex_free  
- acl_fiber_mutex_lock  
- acl_fiber_mutex_trylock  
- acl_fiber_mutex_unlock  

<b>ACL_FIBER_RWLOCK</b>  
- acl_fiber_rwlock_create  
- acl_fiber_rwlock_free  
- acl_fiber_rwlock_rlock  
- acl_fiber_rwlock_tryrlock  
- acl_fiber_rwlock_wlock  
- acl_fiber_rwlock_trywlock  
- acl_fiber_rwlock_runlock  
- acl_fiber_rwlock_wunlock  

<b>ACL_FIBER_EVENT</b>  
- acl_fiber_event_create  
- acl_fiber_event_free  
- acl_fiber_event_wait  
- acl_fiber_event_trywait  
- acl_fiber_event_notify  

<b>ACL_FIBER_SEM</b>  
- acl_fiber_sem_create  
- acl_fiber_sem_free  
- acl_fiber_sem_wait  
- acl_fiber_sem_post  
- acl_fiber_sem_num  

## About API Hook
On Linux/BSD/Mac, many IO and Net APIs are hooked. So you can just use the System standard APIs in your applications with libfiber, the hooked APIs will be replaced with libfiber APIs. In this case, you can <b>`coroutine`</b> your DB application with mysql driven and change nothing in mysql driven.  
The standard APIs been hooked are shown below:
- close
- sleep
- read
- readv
- recv
- recvfrom
- recvmsg
- write
- writev
- send
- sendto
- sendmsg
- sendfile64
- socket
- listen
- accept
- connect
- select
- poll
- epoll: epoll_create, epoll_ctl, epoll_wait
- gethostbyname(_r)
- getaddrinfo/freeaddrinfo

## FAQ
1. <b>Is the coroutine schedule in multi-threads?</b>  
No. The coroutine schedule of libfiber is in one single thread. But you can start multiple threads that one one thread has one schedule process.  
2. <b>How are the multi-cores of CPU used?</b>  
multiple threads can be started with its own coroutine schedule, each thread can ocpupy one CPU.  
3. <b>How does different threads mutex in coroutine schedule status?</b>  
Even though the OS system mutex APIs, such as pthread_mutex_t's APIs can be used, the ACL_FIBER_EVENT's APIs are recommended. It's safety when the OS system mutex APIs are used in short time without recursive invocation. But it's unsafety using system mutex APIs in this case: One coroutine A1 of thread A had locked the thread-mutex-A, the coroutine A2 of thread A wanted to lock the thread-mutex-B which had been locked by one coroutine B1 of thread B, when the coroutine B2 of thread B wanted to lock the thread-mutex-A, thread deadlock happened! So, the coroutine mutex for threads and coroutines named ACL_FIBER_EVENT's APIs of libfiber were created, which can be used to make critical region between multiple coroutines in different threads(multiple continues in the same thread or not; it can also be used for different threads without coroutines).  
4. <b>Should the mysql-driven source codes be changed when used with libfiber?</b>  
In UNIX OS, the System IO APIs are hooked by libfiber, so nothing should be changed in mysql-driven.  
5. <b>How to avoid make the mysqld overloaded when many coroutines started?</b>  
The ACL_FIBER_SEM's APIs can be used to protect the mysqld being overloaded by many connections of many coroutines. These APIs can limit the connections number to the mysqld from coroutines.  
6. <b>Does the DNS domain resolving block the coroutine schedule?</b>  
No, because the System domain-resolving APIs such as gethostbyname(_r) and getaddrinfo are also hooked in libfiber.  
