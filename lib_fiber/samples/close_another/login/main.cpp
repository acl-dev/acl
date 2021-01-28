#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_client : public acl::fiber {
public:
	fiber_client(const char* addr, int max) : addr_(addr), max_(max) {}

protected:
	// @override
	void run(void) {
		for (int i = 0; i < max_; i++) {
			if (!user_login()) {
				break;
			}
		}

		printf("fiber_client logout\r\n");
		delete this;
	}

private:
	acl::socket_stream conn_;
	acl::string addr_;
	int max_;

	bool user_login(void) {
		if (!conn_.open(addr_, 5, 5)) {
			printf("connect %s error %s\r\n",
				addr_.c_str(), acl::last_serror());
			return false;
		}
		if (conn_.write("zsx\r\n") == -1) {
			printf("login error %s\r\n", acl::last_serror());
			return false;
		}

		char buf[8192];
		while (true) {
			int ret = conn_.read(buf, sizeof(buf) - 1, false);
			if (ret == -1) {
				printf("read error=%s\r\n", acl::last_serror());
				break;
			} else {
				buf[ret] = 0;
				printf("read ok=%s", buf);
			}
		}


		return true;
	}

	~fiber_client(void) {}
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s server_addr\r\n"
		" -c max_fibers\r\n"
		" -n connect_count\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, cocurrent = 10, max = 10;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:c:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			max = atoi(optarg);
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	for (int i = 0; i < cocurrent; i++) {
		fiber_client* client = new fiber_client(addr, max);
		client->start();
	}

	acl::fiber::schedule_with(acl::FIBER_EVENT_T_POLL);
	return 0;
}
