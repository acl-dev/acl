#include "stdafx.h"
#include <memory>
#include <atomic>
#include "server_pool.h"

static int __rtimeo = 0, __wtimeo = 0;
using shared_stream = std::shared_ptr<acl::socket_stream>;

class socket_client {
public:
	socket_client(shared_stream conn, std::atomic<long>& nusers)
	: conn_(conn), nusers_(nusers) {}

	~socket_client(void) {
		--nusers_;
	}

	acl::socket_stream& get_conn(void) {
		return *conn_;
	}

private:
	shared_stream conn_;
	std::atomic<long>& nusers_;
};

using shared_client = std::shared_ptr<socket_client>;

class message {
public:
	message(shared_client client, std::atomic<long>& nmsgs,
		const char* buf, size_t len)
	: client_(client), nmsgs_(nmsgs), buf_(buf, len) {}

	~message(void) { --nmsgs_; }

	const std::string& get_data(void) const {
		return buf_;
	}

	shared_client get_client(void) {
		return client_;
	}

private:
	shared_client client_;
	std::atomic<long>& nmsgs_;
	std::string buf_;
};

using shared_message = std::shared_ptr<message>;

void server_pool_run(const char* addr, bool sync, int nfibers,
		int rtimeo, int wtimeo) {
	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return;
	}

	__rtimeo = rtimeo;
	__wtimeo = wtimeo;

	printf("listen on %s, fiber pool: %d\r\n", addr, nfibers);

	acl::fiber_sbox2<shared_message> box;

	for (int i = 0; i < nfibers; i++) {
		go[&box] {
			while (true) {
				shared_message msg;
				if (!box.pop(msg)) {
					printf("POP end!\r\n");
					break;
				}
				auto client = msg->get_client();
				auto data = msg->get_data();

				if (__wtimeo > 0) {
					int fd = client->get_conn().sock_handle();
					if (acl_write_wait(fd , __wtimeo) < 0) {
						printf("write wait error\r\n");
						break;
					}
				}

				if (client->get_conn().write(data.c_str(), data.size()) == -1) {
					printf("write error: %s\r\n", acl::last_serror());
					break;
				}
			}
		};
	}

	std::atomic<long> nusers(0), nmsgs(0);

#if 0
	go[&nusers, &nmsgs] {
		while (true) {
			std::cout << "client count: " << nusers
				<< "; message count: " << nmsgs << std::endl;
			::sleep(1);
		}
	};
#endif

	go[&ss, &box, &nusers, &nmsgs, sync] {
		while (true) {
			auto conn = ss.shared_accept();
			if (conn.get() == NULL) {
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			++nusers;

			auto client = std::make_shared<socket_client>(conn, nusers);

			go[&box, &nmsgs, client, sync] {
				char buf[4096];
				while (true) {
					if (__rtimeo > 0) {
						int fd = client->get_conn().sock_handle();
						if (acl_read_wait(fd, __rtimeo) < 0) {
							printf("read wait error\r\n");
							break;
						}
					}

					int ret = client->get_conn().read(buf, sizeof(buf), false);
					if (ret <= 0) {
						break;
					}

					if (sync) {
						if (client->get_conn().write(buf, ret) != ret) {
							break;
						} else {
							continue;
						}
					}

					++nmsgs;
					auto msg = std::make_shared<message>(client, nmsgs, buf, ret);
					box.push(msg, false);
				}
			};
		}
	};

	acl::fiber::schedule();
}
