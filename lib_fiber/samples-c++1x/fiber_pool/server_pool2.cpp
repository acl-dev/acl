#include "stdafx.h"
#include <memory>
#include <atomic>
#include "server_pool.h"

#if __cplusplus >= 201402L

using unique_stream = std::unique_ptr<acl::socket_stream>;

class socket_client2 {
public:
	socket_client2(unique_stream conn, std::atomic<long>& nusers)
	: conn_(std::move(conn)), nusers_(nusers) {}

	~socket_client2(void) {
		printf(">>>socket_client2 deleted<<<\r\n");
		--nusers_;
	}

	acl::socket_stream& get_conn(void) {
		return *conn_;
	}

private:
	unique_stream conn_;
	std::atomic<long>& nusers_;
};

using shared_client = std::shared_ptr<socket_client2>;

class message2 {
public:
	message2(shared_client client, std::atomic<long>& nmsgs,
		const char* buf, size_t len)
	: client_(client), nmsgs_(nmsgs), buf_(buf, len) {}

	~message2(void) {
		--nmsgs_;
	}

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

using unique_message2 = std::unique_ptr<message2>;

void server_pool2_run(const char* addr, bool sync, int nfibers) {
	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return;
	}

	printf("listen on %s, fiber pool: %d\r\n", addr, nfibers);

	acl::fiber_sbox2<unique_message2> box;

	for (int i = 0; i < nfibers; i++) {
		go[&box] {
			while (true) {
				unique_message2 msg;
				if (!box.pop(msg)) {
					continue;
				}

				auto client = msg->get_client();
				auto data = msg->get_data();
				if (client->get_conn().write(data.c_str(), data.size()) == -1) {
					printf("write error: %s\r\n", acl::last_serror());
					break;
				}
			}
		};
	}

	std::atomic<long> nusers(0), nmsgs(0);

	go[&nusers, &nmsgs] {
		while (true) {
			std::cout << "client count: " << nusers
				<< "; message2 count: " << nmsgs << std::endl;
			::sleep(1);
		}
	};

	go[&ss, &box, &nusers, &nmsgs, sync] {
		while (true) {
			auto conn = ss.accept();
			if (conn == NULL) {
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			++nusers;

			go[&box, &nmsgs, conn, sync, &nusers] {
				unique_stream stream(conn);
				auto client = std::make_shared<socket_client2>(std::move(stream), nusers);

				char buf[4096];
				while (true) {
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
					auto msg = std::make_unique<message2>(client, nmsgs, buf, ret);
					box.push(std::move(msg));
				}
			};
		}
	};

	acl::fiber::schedule();
}

#else
void server_pool2_run(const char*, bool, int) {
	std::cout << "Need c++14!" << std::endl;
}
#endif
