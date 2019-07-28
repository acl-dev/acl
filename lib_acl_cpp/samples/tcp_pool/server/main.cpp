#include "stdafx.h"
#include "lib_acl.h"

static ACL_ATOMIC* __atomic;
static int __count;
static int __nstep = 100000;

class client_thread : public acl::thread
{
public:
	client_thread(acl::socket_stream* conn) : conn_(conn) {}
	~client_thread(void) {}

protected:
	void* run(void)
	{
		acl::tcp_reader reader(*conn_);
		acl::string buf;
		long long n;

		while (true) {
			if (reader.read(buf) == false) {
				printf("read over %s\r\n", acl::last_serror());
				break;
			}
			buf.clear();
			n = acl_atomic_int64_add_fetch(__atomic, 1);
			if (n % __nstep == 0) {
				char info[128];
				snprintf(info, sizeof(info), "n=%lld", n);
				acl::meter_time(__FILE__, __LINE__, info);
			}
		}

		delete conn_;
		return NULL;
	}

private:
	acl::socket_stream* conn_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s addr -i display_count\r\n", procname);
}

int main(int argc, char* argv[])
{
	int ch;
	acl::string addr("127.0.0.1:8887");

	while ((ch = getopt(argc, argv, "hs:i:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'i':
			__nstep = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false) {
		printf("listen on %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	else
		printf("listen on %s ok\r\n", addr.c_str());

	__atomic = acl_atomic_new();
	acl_atomic_set(__atomic, &__count);
	acl_atomic_int64_set(__atomic, 0);

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		acl::thread* thread = new client_thread(conn);
		thread->start();
	}

	acl_atomic_free(__atomic);
	return 0;
}
