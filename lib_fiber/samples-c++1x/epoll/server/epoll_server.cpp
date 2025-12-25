/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: shuxin.zheng@qq.com
 * 
 * VERSION
 *   Sun 03 Dec 2017 20:49:54 AM CST
 */

#include <signal.h>
#include <sys/epoll.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "fiber/libfiber.hpp"

#define	EPOLL_ET	(1 << 0)
#define	EPOLL_ONESHOT	(1 << 1)

static unsigned __epoll_flag = 0;

static void epoll_add_read(int epfd, int fd, void *ptr) {
	struct epoll_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	if (__epoll_flag & EPOLL_ET) {
		ev.events |= EPOLLET;
	}

	if (__epoll_flag & EPOLL_ONESHOT) {
		ev.events |= EPOLLONESHOT;
	}

	ev.data.ptr = ptr;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		printf("add error %s\r\n", acl::last_serror());
		exit (1);
	}
}

static void epoll_read_again(int epfd, int fd, void *ptr) {
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLONESHOT;
	if (__epoll_flag & EPOLL_ET) {
		ev.events |= EPOLLET;
	}

	ev.data.ptr = ptr;

	if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
		printf("add error %s\r\n", acl::last_serror());
		exit (1);
	}
}

class epoll_server {
public:
	epoll_server(int epfd, acl::server_socket& ss) : epfd_(epfd), ss_(ss) {}

	~epoll_server() = default;

	void run(void) {
		printf("fiber-%d started\r\n", acl::fiber::self());

		epoll_add_read(epfd_, ss_.sock_handle(), &ss_);

#define MAX	1000
		struct epoll_event events[MAX];

		while (true) {
			int nfds = epoll_wait(epfd_, events, MAX, -1);
			if (nfds == -1) {
				printf("wait error %s\r\n", acl::last_serror());
				sleep(1);
				continue;
			}

			//if (nfds >= 20) { printf("fiber-%d: ready=%d\n", acl::fiber::self(), nfds); }

			for (int i = 0; i < nfds; i++) {
				if (events[i].data.ptr == &ss_) {
					handle_accept(ss_);
				} else {
					handle_client(events[i].data.ptr);
				}
			}
		}

		close(epfd_);
	}

private:
	void handle_accept(acl::server_socket& ss) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return;
			}
			printf("accept error %s\r\n", acl::last_serror());
			exit (1);
		} else {
			printf("fiber-%d: accept one fd %d\r\n",
				acl::fiber::self(), conn->sock_handle());
			conn->set_tcp_non_blocking(true);

			epoll_add_read(epfd_, conn->sock_handle(), conn);
			if (__epoll_flag & EPOLL_ONESHOT) {
				epoll_read_again(epfd_, ss.sock_handle(), &ss);
			}
		}
	}

	void handle_client(void *ctx) {
		acl::socket_stream* conn = (acl::socket_stream *) ctx;
		while (true) {
			int ret = client_read_once(conn);
			if (ret == 0) {
				break;
			} else if (ret == -1) {
#if 0
				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				epoll_ctl(epfd_, EPOLL_CTL_DEL, conn->sock_handle(), &ev);
#endif
				delete conn;
				break;
			}
		}
	}

	int client_read_once(acl::socket_stream* conn) {
		int fd = conn->sock_handle(), ret;
		char buf[4096];
		if ((ret = read(fd, buf, sizeof(buf) - 1)) == 0) {
			printf(">>>read over fd: %d\n", fd);
			return -1;
		} else if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			}
			printf("thread-%lu gets error %s, conn=%p\r\n",
				acl::thread::self(), acl::last_serror(), conn);
			return -1;
		} else if (write(fd, buf, ret) != ret) {
			printf("thread-%lu write error %s\r\n",
				acl::thread::self(), acl::last_serror());
			return -1;
		} else if (ret == sizeof(buf) - 1) {
			return 1;
		} else {
			if (__epoll_flag & EPOLL_ONESHOT) {
				epoll_read_again(epfd_, conn->sock_handle(), conn);
			}
			return 0;
		}
	}

private:
	int epfd_;
	acl::server_socket& ss_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s server_addr\r\n"
		" -c the_fiber_count[default: 0]\r\n"
		" -O [if using EPOLL_ONESHOT]\r\n"
		" -E [if using EPOLL_ET]\r\n", procname);
}

int main(int argc, char* argv[]) {
	acl::string addr("127.0.0.1:8887");
	int  ch, cnt = 0;

	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt(argc, argv, "hs:c:OE")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			cnt = atoi(optarg);
			break;
		case 'O':
			__epoll_flag |= EPOLL_ONESHOT;
			break;
		case 'E':
			__epoll_flag |= EPOLL_ET;
			break;
		default:
			break;
		}
	}

	acl::server_socket ss(127, false);
	if (ss.open(addr) == false) {
		printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr.c_str());

	if (cnt <= 0) {
		int epfd = epoll_create(1024);
		epoll_server server(epfd, ss);
		server.run();
		return 0;
	}


	go[&ss, cnt] {
		int epfd = epoll_create(1024);
		for (int i = 0; i < cnt; i++) {
			go[epfd, &ss] {
				epoll_server server(epfd, ss);
				server.run();
			};
		}
	};

	acl::fiber::schedule();
	return 0;
}
