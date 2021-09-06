#include "pch.h"
#include "FiberHttpc.h"
#include "FiberHttpd.h"

CFiberHttpd::CFiberHttpd(const char* addr) : addr_(addr) {}

CFiberHttpd::~CFiberHttpd(void) {}

void CFiberHttpd::run(void) {
	acl::server_socket ss;
	if (!ss.open(addr_)) {
		printf("listen %s error %s\r\n", addr_.c_str(), acl::last_serror());
		return;
	}

	printf("listen %s ok, http server started!\r\n", addr_.c_str());

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept NULL error %s\r\n", acl::last_serror());
			break;
		}

		printf("Accept client from %s, start http client\r\n", conn->get_peer(true));
		acl::fiber* fb = new CFiberHttpc(conn);
		fb->start();
	}

	delete this;
}
