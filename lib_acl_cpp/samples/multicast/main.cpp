#include "stdafx.h"

class multicast_thread : public acl::thread {
public:
	multicast_thread() : loopback_(false) {}
	virtual ~multicast_thread() {}

	void set_multicast_loopback(bool on) {
		loopback_ = on;
	}

	bool open(const char* addr, const char* iface, int port) {
		unsigned oflags = acl::OPEN_FLAG_REUSEPORT;
		if (loopback_) {
			oflags |= acl::OPEN_FLAG_MULTICAST_LOOP;
		}

		if (ss_.bind_multicast(addr, iface, port, -1, oflags)) {
			printf("Open ok, addr=%s, iface=%s, port=%d\r\n",
				addr, iface, port);
			return true;
		}

		printf("Bind error=%s, addr=%s, iface=%s, port=%d\r\n",
			acl::last_serror(), addr, iface, port);
		return false;
	}

protected:
	acl::socket_stream ss_;
	bool loopback_;
};

class sender_thread : public multicast_thread {
public:
	sender_thread(int count, acl::atomic_long& counter)
	: count_(count), counter_(counter)
	{
	}

	~sender_thread() {}

protected:
	// @override
	void* run() {
		const char* data = "hello world!\r\n";
		size_t dlen = strlen(data);
		for (int i = 0; i < count_; i++) {
			if (ss_.write(data, dlen, false) == -1) {
				printf("send error %s\r\n", acl::last_serror());
				break;
			}
			++counter_;
		}

		return NULL;
	}


private:
	int count_;
	acl::atomic_long& counter_;
};

class reader_thread : public multicast_thread {
public:
	reader_thread(acl::atomic_long& counter, bool echo)
	: counter_(counter), echo_(echo)
	{}

	~reader_thread() {}

protected:
	// @override
	void* run() {
		while (true) {
			char buf[512];
			int ret = ss_.read(buf, sizeof(buf) - 1, false);
			if (ret == -1) {
				printf("Read error=%s\r\n", acl::last_serror());
				break;
			}

			if (echo_ && ss_.write(buf, ret) == -1) {
				printf("Send error=%s\r\n", acl::last_serror());
				break;
			}

			long long n = ++counter_;
			if (n < 50) {
				buf[ret] = 0;
				printf("Thread-%lu read(count=%lld): %s",
					acl::thread::self(), n, buf);
				fflush(stdout);
			} else if (n % 10000 == 0) {
				char tmp[128];
				snprintf(tmp, sizeof(tmp), "read count=%lld", n);
				acl::meter_time("reader", __LINE__, tmp);
			}
		}
		return NULL;
	}

private:
	acl::atomic_long& counter_;
	bool echo_;
};

class monitor_thread : public acl::thread {
public:
	monitor_thread(acl::atomic_long& counter) : counter_(counter) {}
	~monitor_thread() {}

protected:
	// @override
	void* run() {
		while (true) {
			sleep(1);
			printf(">>>read count=%lld<<<\r\n", counter_.value());
		}
		return NULL;
	}

private:
	acl::atomic_long& counter_;
};

static void usage(const char* proc) {
	printf("usage: %s -h [help]\r\n"
		" -m multicast_addr\r\n"
		" -l local_iface[default: 0.0.0.0]\r\n"
		" -p port[default: 8089]\r\n"
		" -n count_for_sender[default: 10000]\r\n"
		" -w senders_thread_count[default: 1]\r\n"
		" -r readers_thread_count[default: 0]\r\n"
		" -E [if echo the read data, default: false]\r\n"
		" -O [if enable IP_MULTICAST_LOOP, default: false]\r\n"
		, proc);
}

int main(int argc, char* argv[]) {
	acl::string multicast_addr, local_iface = "0.0.0.0";
	int ch, nsenders = 1, nreaders = 0, port = 8089, count = 10000;
	acl::atomic_long write_counter, read_counter;
	bool echo = false, loopback = false;

	while ((ch = getopt(argc, argv, "hm:l:p:n:w:r:EO")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'w':
			nsenders= atoi(optarg);
			break;
		case 'r':
			nreaders = atoi(optarg);
			break;
		case 'm':
			multicast_addr = optarg;
			break;
		case 'l':
			local_iface = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'E':
			echo = true;
			break;
		case 'O':
			loopback = true;
			break;
		default:
			break;
		}
	}

	if (multicast_addr.empty()) {
		printf("multicast_addr empty!\r\n");
		usage(argv[0]);
		return 0;
	}

	acl::log::stdout_open(true);

	std::vector<acl::thread*> senders, readers;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < nsenders; i++) {
		sender_thread* thread = new sender_thread(count, write_counter);
		thread->set_multicast_loopback(loopback);

		if (!thread->open(multicast_addr, local_iface, port)) {
			delete thread;
			continue;
		}

		thread->set_detachable(false);
		thread->start();
		senders.push_back(thread);
	}

	for (int i = 0; i < nreaders; i++) {
		reader_thread* thread = new reader_thread(read_counter, echo);
		thread->set_multicast_loopback(loopback);

		if (!thread->open(multicast_addr, local_iface, port)) {
			delete thread;
			continue;
		}

		thread->set_detachable(false);

		thread->start();
		readers.push_back(thread);
	}

	for (std::vector<acl::thread*>::iterator it = senders.begin();
		it != senders.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	if (nsenders > 0) {
		struct timeval end;
		gettimeofday(&end, NULL);

		double cost = acl::stamp_sub(end, begin);
		double speed = (write_counter.value() * 1000)
				/ (cost > 0 ? cost : 0.0001);

		printf("All over, count=%lld, cost=%.2f seconds, speed=%.2f\r\n",
			write_counter.value(), cost, speed);
	}

	acl::thread* monitor;
	if (nreaders > 0) {
		monitor = new monitor_thread(read_counter);
		monitor->set_detachable(false);
		monitor->start();
	} else {
		monitor = NULL;
	}

	for (std::vector<acl::thread*>::iterator it = readers.begin();
		it != readers.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	if (monitor) {
		monitor->wait();
		delete monitor;
	}

	return 0;
}

