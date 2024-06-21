#include "stdafx.h"

int main() {
	const char* addr = "127.0.0.1:8288";
	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("Listen on %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	printf("Listen on %s ok\r\n", addr);

	go[&ss] {
		while (true) {
			acl::shared_stream conn = ss.shared_accept();
			if (conn == nullptr) {
				break;
			}

			go[conn] {
				char buf[4096];
				int ret = recv(conn->sock_handle(), buf, sizeof(buf), MSG_DONTWAIT);
				if (ret == -1) {
					if (errno == EAGAIN) {
						printf("No data available!\r\n");
					} else {
						printf("Recv error %s\r\n", acl::last_serror());
						return;
					}
				}

				while (true) {
					if (!conn->alive()) {
						printf("Connection isn't aliving now\r\n");
						break;
					}

					ret = conn->read(buf, sizeof(buf), false);
					if (ret == -1) {
						printf("Read over!\r\n");
						break;
					}
					if (conn->write(buf, ret) == -1) {
						printf("Write error!\r\n");
						break;
					}
					::sleep(2);
				}
			};
		}
	};

	acl::fiber::schedule();
	return 0;
}
