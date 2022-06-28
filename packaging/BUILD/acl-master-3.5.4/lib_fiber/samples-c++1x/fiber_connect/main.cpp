#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class myfiber : public acl::fiber {
public:
	myfiber(const char* addr, acl::fiber_sem* sem,
		acl::fiber_tbox<bool>* box)
	: addr_(addr), sem_(sem), box_(box)
	{
	}

	~myfiber(void) {}

	void wait(void) {
		if (sem_) {
			printf("%s: wait sem\r\n", __FUNCTION__);
			sem_->wait();
			printf("%s: wait sem ok\r\n", __FUNCTION__);
		} else if (box_) {
			printf("%s: wait box\r\n", __FUNCTION__);
			box_->pop();
			printf("%s: wait box ok\r\n", __FUNCTION__);
		} else {
			assert(0);
		}
	}

protected:
	// @override
	void run(void) {
		acl::socket_stream conn;

		printf("%s: begin to connect %s\r\n", __FUNCTION__, addr_.c_str());

		printf("\r\n");
		if (conn.open(addr_, 0, 0)) {
			printf(">>> %s: connect %s ok\r\n", __FUNCTION__, addr_.c_str());
		} else {
			printf(">>> %s: connect %s error %s\r\n", __FUNCTION__,
				addr_.c_str(), acl::last_serror());
		}

		if (sem_) {
			sem_->post();
		} else if (box_) {
			box_->push(NULL);
		}
	}

private:
	acl::string addr_;
	acl::fiber_sem* sem_;
	acl::fiber_tbox<bool>* box_;
};

static void fiber_redis(const char* addr, bool use_sem)
{
	acl::socket_stream conn;

	printf("\r\n");
	if (conn.open(addr, 0, 0)) {
		printf(">>> %s: connect %s ok\r\n", __FUNCTION__, addr);
	} else {
		printf(">>> %s: connect %s error %s\r\n",
			__FUNCTION__, addr, acl::last_serror());
	}

	acl::fiber_sem sem(0);
	acl::fiber_tbox<bool> box;

	myfiber* fb;

	if (use_sem) {
		fb = new myfiber(addr, &sem, NULL);
	} else {
		fb = new myfiber(addr, NULL, &box);
	}

	printf("%s: begin create one fiber\r\n", __FUNCTION__);
	fb->start();

	printf("%s: begin to wait fiber\r\n", __FUNCTION__);
	fb->wait();

	printf("\r\n");
	delete fb;
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s addr\r\n"
		" -S [if use fiber_sem or use fiber_tbox]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	bool use_sem = false;
	acl::string addr("127.0.0.1:6379");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:S")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'S':
			use_sem = true;
			break;
		default:
			break;
		}
	}

	printf("\r\n");

	go[&] {
		fiber_redis(addr, use_sem);
	};

	acl::fiber::schedule();
	return 0;
}
