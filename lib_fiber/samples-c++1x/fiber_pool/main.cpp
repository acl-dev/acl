#include "stdafx.h"
#include <memory>
#include <atomic>

class client_socket {
public:
	client_socket(acl::socket_stream* conn, std::atomic<long>& nusers)
	: conn_(conn), nusers_(nusers) {}

	~client_socket(void) {
		printf("delete conn=%p\r\n", conn_);
		--nusers_;
		delete conn_;
	}

	acl::socket_stream& get_conn(void) {
		return *conn_;
	}

private:
	acl::socket_stream* conn_;
	std::atomic<long>& nusers_;
};

using shared_client = std::shared_ptr<client_socket>;

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

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s ip:port, default: 127.0.0.1:9001\r\n"
		" -c fiber_pool_count [default: 100] \r\n"
		" -r timeout\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr("127.0.0.1:9001");
	int  ch, nfibers = 100;

	while ((ch = getopt(argc, argv, "hs:c:r:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber::stdout_open(true);
	acl::log::stdout_open(true);

	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}

	printf("listen on %s, fiber pool: %d\r\n", addr.c_str(), nfibers);

	acl::fiber_sbox2<shared_message> box;

	for (int i = 0; i < nfibers; i++) {
		go[&box] {
			while (true) {
				shared_message msg = box.pop();
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
			std::cout << "client count: " << nusers << "; message count: " << nmsgs << std::endl;
			::sleep(1);
		}
	};

	go[&ss, &box, &nusers, &nmsgs] {
		while (true) {
			auto conn = ss.accept();
			if (conn == NULL) {
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			++nusers;

			auto client = std::make_shared<client_socket>(conn, nusers);

			go[&box, &nmsgs, client] {
				char buf[4096];
				while (true) {
					int ret = client->get_conn().read(buf, sizeof(buf), false);
					if (ret <= 0) {
						break;
					}

#if 0
					if (client->get_conn().write(buf, ret) != ret) {
						break;
					} else {
						continue;
					}
#endif
					++nmsgs;
					auto msg = std::make_shared<message>(client, nmsgs, buf, ret);
					box.push(msg);
				}
			};
		}
	};

	acl::fiber::schedule();
	return 0;
}
