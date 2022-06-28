#include "stdafx.h"
#include <unistd.h>
#include <getopt.h>

class client : public acl::thread
{
public:
	client(const char* addr) : addr_(addr) {}
	~client(void) {}

protected:
	void* run(void)
	{
		acl::socket_stream conn;
		if (!conn.open(addr_, 10, 10)) {
			printf("%ld: connect %s error %s\r\n",
				acl::thread::self(), addr_.c_str(),
				acl::last_serror());
			return NULL;
		}

		(void) conn.set_tcp_solinger(true, 0);

		printf("connect %s ok, my addr=%s\r\n",
			addr_.c_str(), conn.get_local(true));
		for (int i = 0; i < 10; i++) {
			if (conn.format("thread-%ld: hello world\r\n",
				acl::thread::self()) == -1) {

				printf("write to %s error %s\r\n",
					addr_.c_str(), acl::last_serror());
				break;
			}
			acl::string buf;
			if (!conn.gets(buf)) {
				printf("gets from %s error %s\r\n",
					addr_.c_str(), acl::last_serror());
				break;
			} else {
				printf("%ld: %s\r\n", acl::thread::self(),
					buf.c_str());
			}
		}

		printf("sleep 2 seconds\r\n");
		sleep(2);
		printf("disconnect from %s now\r\n", addr_.c_str());
		return NULL;
	}

private:
	acl::string addr_;
};

static void add_servers(std::vector<acl::string>& servers, const char* s)
{
	acl::string buf(s);
	const std::vector<acl::string>& tokens = buf.split2(",; \t");

	for (std::vector<acl::string>::const_iterator cit = tokens.begin();
		cit != tokens.end(); ++cit) {

		servers.push_back(*cit);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s server_list\r\n", procname);
}

int main(int argc, char* argv[])
{
	std::vector<acl::string> addrs;
	int ch;

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			add_servers(addrs, optarg);
			break;
		default:
			break;
		}
	}

	if (addrs.empty()) {
		usage(argv[0]);
		return 0;
	}

	std::vector<acl::thread*> threads;

	for (std::vector<acl::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit) {

		acl::thread* thr = new client(*cit);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}
	return 0;
}
