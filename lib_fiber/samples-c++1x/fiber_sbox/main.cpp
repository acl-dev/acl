#include "stdafx.h"
#include <memory>
#include <atomic>

class message {
public:
	message(std::atomic<long>& nmsgs, int id)
	: nmsgs_(nmsgs), id_(id) {}

	~message(void) { --nmsgs_; }

	int get_id(void) const {
		return id_;
	}

private:
	std::atomic<long>& nmsgs_;
	int id_;
};

using shared_message = std::shared_ptr<message>;

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -c fiber_pool_count [default: 5] \r\n"
		" -n message_count [default: 100]\r\n"
		" -S [if post in sync mode, default: async mode]\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	bool sync = false;
	int  ch, nfibers = 5, count = 100;

	while ((ch = getopt(argc, argv, "hc:n:S")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'S':
			sync = true;
			break;
		default:
			break;
		}
	}

	acl::fiber::stdout_open(true);
	acl::log::stdout_open(true);

	acl::fiber_sbox2<shared_message> box(!sync);
	std::vector<ACL_FIBER*> fibers;
	std::atomic<int> nfibers_left(nfibers);

	for (int i = 0; i < nfibers; i++) {
		auto fb = go[&box, &fibers, &nfibers_left] {
			while (true) {
				shared_message msg;
				if (!box.pop(msg)) {
					std::cout << "POP end!" << std::endl;
					break;
				}

				int id = msg->get_id();
				std::cout << "fiber-" << acl::fiber::self()
					<< ", id=" << id << std::endl;
			}

			std::cout << "fiber-" << acl::fiber::self()
				<< " exited!" << std::endl;
			--nfibers_left;
		};

		fibers.push_back(fb);
	}

	std::atomic<long> nmsgs(0);

	go[&nmsgs, &nfibers_left] {
		while (nfibers_left > 0) {
			std::cout << "message count: " << nmsgs << std::endl;
			acl::fiber::delay(500);
		}
		std::cout << "All consumers exited!" << std::endl;
	};

	go[&box, &fibers, &nmsgs, nfibers, count] {
		for (int i = 0; i < count; i++) {
			auto msg = std::make_shared<message>(nmsgs, i);
			++nmsgs;
			box.push(msg);
			if (i > 0 && i % 10 == 0) {
				::sleep(1);
			}
		}

		for (auto fb : fibers) {
			std::cout << "Begin kill fiber-"
				<< acl_fiber_id(fb) << std::endl;
			acl_fiber_kill(fb);
		}
	};

	acl::fiber::schedule();
	return 0;
}
