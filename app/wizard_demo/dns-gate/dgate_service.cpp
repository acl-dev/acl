/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Sat Apr 30 10:20:34 2022
 */

#include "stdafx.h"
#include <thread>
#include "dgate_service.h"

class request_message {
public:
	request_message(acl::socket_stream* server, const char* peer_addr,
			const char* data, size_t dlen) {
		server_ = server;
		peer_addr_ = peer_addr;
		data_.copy(data, dlen);
	}

	~request_message(void) {}

	acl::socket_stream* server_;
	acl::string peer_addr_;
	acl::string data_;
};

static acl::fiber_tbox<request_message>* __request_box;

static void add_addrs(acl::string& buf, const std::vector<acl::string>& addrs) {
	for (std::vector<acl::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit) {

		if (cit == addrs.begin()) {
			buf.format_append("%s", (*cit).c_str());
		} else {
			buf.format_append(", %s", (*cit).c_str());
		}
	}
}

static void show_addrs(const char* client_addr, const char* name,
		const acl::rfc1035_response& res) {
	acl::string buf;
	buf.format("client=%s, name=%s", client_addr, name);

	const std::vector<acl::string>& addrs4a = res.get_addrs4a();
	if (!addrs4a.empty()) {
		buf.append(", A=");
		add_addrs(buf, addrs4a);
	}

	const std::vector<acl::string>& addrs4aaaa = res.get_addrs4aaaa();
	if (!addrs4aaaa.empty()) {
		buf.append(", AAAA=");
		add_addrs(buf, addrs4aaaa);
	}

	const std::vector<acl::string>& cnames = res.get_cnames();
	if (!cnames.empty()) {
		buf.append(", CNAME=");
		add_addrs(buf, cnames);
	}
	logger("%s", buf.c_str());
}

static void handle_request(request_message& msg) {
	acl::rfc1035_request req;
	if (!req.parse_request(msg.data_, msg.data_.size())) {
		//logger_error("invalid request data");
		return;
	}

	const char* name = req.get_name();
	//unsigned short qid = req.get_qid();

	const char* local_addr = "0.0.0.0|0";
	acl::socket_stream client;
	if (!client.bind_udp(local_addr)) {
		logger_error("bind %s error %s, name=%s",
			local_addr, acl::last_serror(), name);
		return;
	}

	if (client.sendto(msg.data_, msg.data_.size(),
			var_cfg_upstream_addr, 0) == -1) {
		logger_error("sendo request to %s error %s, name=%s",
			var_cfg_upstream_addr, acl::last_serror(), name);
		return;
	}

	char buf[1024];
	int len;
	if ((len = client.read(buf, sizeof(buf) - 1, false)) == -1) {
		logger_error("read from %s error=%s, name=%s",
			var_cfg_upstream_addr, acl::last_serror(), name);
		return;
	}

	acl::rfc1035_response res;
	if (!res.parse_reply(buf, (size_t) len)) {
		logger_error("invalid reply, name=%s", name);
		return;
	}

	acl::string key(name);
	key.lower();
	if (var_display_disabled.find(key) == var_display_disabled.end()) {
		show_addrs(msg.peer_addr_.c_str(), name, res);
	}

	acl::socket_stream reply;
	reply.open(msg.server_->sock_handle(), true);
	if (reply.sendto(buf, len, msg.peer_addr_, 0) == -1) {
		logger_error("sendto reply to %s error %s, name=%s",
			msg.peer_addr_.c_str(), acl::last_serror(), name);
	}

	reply.unbind_sock();
}

static void waiting_message(acl::fiber_tbox<request_message> *box) {
	while (true) {
		request_message* msg = box->pop();
		if (msg == NULL) {
			logger("wait one null message and stop now!");
			break;
		}

		go[=] {
			handle_request(*msg);
			delete msg;
		};
	}
}

static void service_main(acl::fiber_tbox<request_message>* box) {
	go[=] {
		waiting_message(box);
	};

	acl::fiber::schedule();
}

void dgate_service_start(void) {
	__request_box = new acl::fiber_tbox<request_message>;

	std::thread dgate_thread(service_main, __request_box);
	dgate_thread.detach();
}

void dgate_push_request(acl::socket_stream* server, const char* peer_addr,
	const char* data, size_t dlen) {
	request_message* msg = new request_message(server, peer_addr, data, dlen);
	__request_box->push(msg);
}
