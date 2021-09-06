#include "pch.h"
#include "HttpServlet.h"
#include "FiberHttpc.h"

CFiberHttpc::CFiberHttpc(acl::socket_stream* conn) : conn_(conn) {}

CFiberHttpc::~CFiberHttpc(void) { delete conn_; }

void CFiberHttpc::run(void) {
	acl::memcache_session session("127.0.0.1:11211");
	conn_->set_rw_timeout(10);
	HttpServlet servlet(conn_, &session);
	servlet.setLocalCharset("utf-8");

	while (true) {
		if (!servlet.doRun()) {
			break;
		}
	}

	printf("Disconnect from client %s\r\n", conn_->get_peer(true));
	delete this;
}
