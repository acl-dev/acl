#include <getopt.h>
#include <cstdlib>
#include <cstdio>

#include "acl_cpp/lib_acl.hpp"		// must before http_server.hpp
#include "fiber/http_server.hpp"

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -t threads\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	acl::string addr("127.0.0.1|9001");
	int  ch, nthreads = 2;

	while ((ch = getopt(argc, argv, "hs:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::http_server server(addr);

	server.get("/", [](acl::request_t&, acl::response_t& res) {
		std::string buf("hello world1!\r\n");
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size());
	});

	server.get("/hello", [](acl::request_t&, acl::response_t& res) {
		std::string buf("hello world2!\r\n");
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size());
	});

	server.run(nthreads);
	return 0;
}
