#include "stdafx.h"
#include <vector>
#include <thread>
#include <iostream>

int main() {
	acl::fiber::stdout_open(true);

	for (size_t i = 0; i < 5; i++) {
		go[] {
			const char* addr = "127.0.0.1:0";
			acl::socket_stream ss;
			if (!ss.bind_udp(addr, -1, acl::OPEN_FLAG_REUSEPORT)) {
				printf("bind %s error %s\r\n", addr, acl::last_serror());
				exit(1);
			}

			ss.set_rw_timeout(5, true);

			int fd = ss.sock_handle();

			const char* dest_addr = "127.0.0.1:8888";
			const char* data = "hello world!";
			char buf[1024];

			for (size_t j = 0; j < 5; j++) {
				int ret = ss.sendto(data, strlen(data), dest_addr, 0);
				if (ret == -1) {
					printf("sendto to %s error\r\n", dest_addr);
					break;
				}

				ACL_SOCKADDR src_addr;
				socklen_t addrlen = sizeof(src_addr);

				ret = ::recvfrom(fd, buf, sizeof(buf) - 1, 0,
					(struct sockaddr*) &src_addr, &addrlen);
				if (ret == -1) {
					printf("recvfrom error %s\r\n", acl::last_serror());
					break;
				}
				buf[ret] = 0;
				printf("recvfrom: %s\r\n", buf);
			}
		};
	}

	acl::fiber::schedule();
	printf("All over!\r\n");
	return 0;
}
