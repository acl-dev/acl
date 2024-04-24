#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

class locker {
public:
	locker(const char* filepath, bool close_reader, int sleep_s)
	: filepath_(filepath), close_reader_(close_reader), sleep_s_(sleep_s) {}
	~locker() {}

	void run() {
		if (!fp_.open(filepath_, O_RDWR | O_CREAT, 0600)) {
			printf("open %s error\r\n", filepath_.c_str());
			return;
		}

		if (!fp_.try_lock()) {
			printf("lock %s error %s\r\n", filepath_.c_str(), acl::last_serror());
			return;
		}

		printf("lock %s ok\r\n", filepath_.c_str());

		acl::ifstream reader;
		if (reader.open_read(filepath_)) {
			printf("open_read %s ok\r\n", filepath_.c_str());
		} else {
			printf("open_read %s error\r\n", filepath_.c_str());
		}

		if (close_reader_) {
			reader.close();
		}

		::sleep(sleep_s_);

		if (!fp_.unlock()) {
			printf("Unlock %s error %s\r\n", filepath_.c_str(), acl::last_serror());
		} else {
			printf("Unlock %s ok\r\n", filepath_.c_str());
		}

		::sleep(sleep_s_);
	}

private:
	acl::string filepath_;
	bool close_reader_;
	int sleep_s_;

	acl::fstream fp_;
};

#ifdef USE_FIBER
class fiber_locker : public acl::fiber {
public:
	fiber_locker(const char* filepath, bool close_reader, int sleep_s)
	: filepath_(filepath), close_reader_(close_reader), sleep_s_(sleep_s) {}
	~fiber_locker() {}

protected:
	// @override
	void run() {
		printf("Running in fiber mode\r\n");
		locker lk(filepath_, close_reader_, sleep_s_);
		lk.run();
	}

private:
	acl::string filepath_;
	bool close_reader_;
	int sleep_s_;
};
#endif

class thread_locker : public acl::thread {
public:
	thread_locker(const char* filepath, bool close_reader, int sleep_s)
	: filepath_(filepath), close_reader_(close_reader), sleep_s_(sleep_s) {}
	~thread_locker() {}

protected:
	// @override
	void* run() {
		printf("Running in thread mode\r\n");
		locker lk(filepath_, close_reader_, sleep_s_);
		lk.run();
		return NULL;
	}

private:
	acl::string filepath_;
	bool close_reader_;
	int sleep_s_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		"  -t [if using thread mode, default: false]\r\n"
		"  -c [if closing reader, default: false]\r\n"
		"  -w sleep_time[default: 5]\r\n", procname);
}

int main(int argc, char* argv[]) {
	int ch, sleep_s = 5;
	bool use_thread = false, close_reader = false;

	while ((ch = getopt(argc, argv, "htcw:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			use_thread = true;
			break;
		case 'c':
			close_reader = true;
			break;
		case 'w':
			sleep_s = atoi(optarg);
			if (sleep_s <= 0) {
				sleep_s = 5;
			}
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (use_thread) {
		thread_locker lk("./dummy.lock", close_reader, sleep_s);
		lk.start();
		lk.wait();
	} else {
#ifdef USE_FIBER
		acl::fiber::stdout_open(true);
		fiber_locker lk("./dummy.lock", close_reader, sleep_s);
		lk.start();
		acl::fiber::schedule();
#endif
	}

	return 0;
}
