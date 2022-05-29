#include "stdafx.h"
#include <thread>
#include "black_list.h"
#include "rules/rules_option.h"
#include "manage/manage_service.h"
#include "dgate_db.h"
#include "dgate_service.h"

class request_message {
public:
	request_message(dgate_db* db, acl::socket_stream* server,
			const char* peer_addr, const char* data, size_t dlen) {
		db_ = db;
		server_ = server;
		peer_addr_ = peer_addr;
		data_.copy(data, dlen);
	}

	~request_message(void) {}

	dgate_db* db_;
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
	const char* pname = name;
	const acl::token_node* node = var_display_disabled.search(&pname);
	if (node != NULL) {
		return;
	}

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

static void redis_save(acl::redis_client_cluster& conns, const char* name,
		time_t now, const char* now_s) {
	std::map<acl::string, double> members;
	members[now_s] = (double) now;

	acl::redis cmd(&conns);
	cmd.zadd(name, members);
}

static void db_save(dgate_db& db, const char* name, time_t now, const char* now_s) {
	db.add("127.0.0.1", name, now, now_s);
}

static void save_record(request_message& msg, const char* name) {
	time_t now = time(NULL);
	acl::rfc822 rfc;
	char buf[128];
	buf[0] = 0;
	rfc.mkdate_cst(now, buf, sizeof(buf));
	if (buf[0] == 0) {
		logger_error("can't mkdate_cst, name=%s", name);
		return;
	}

	if (var_redis_conns) {
		redis_save(*var_redis_conns, name, now, buf);
	}
	if (msg.db_) {
		db_save(*msg.db_, name, now, buf);
	}
}

static void reply_dummy(request_message& msg, acl::rfc1035_request& req,
	const std::vector<acl::string>& addrs) {
	const char* name = req.get_name();
	unsigned short qid = req.get_qid();

	char buf[1024];
	acl::rfc1035_response res;
	res.set_name(name).set_qid(qid).set_type(acl::rfc1035_type_a)
		.set_ttl(600);
	size_t n  = res.build_reply(addrs, buf, sizeof(buf));

	acl::socket_stream reply_sock;
	reply_sock.open(msg.server_->sock_handle(), true);
	if (reply_sock.sendto(buf, n, msg.peer_addr_, 0) == -1) {
		logger_error("sendto reply to %s error %s, name=%s",
			msg.peer_addr_.c_str(), acl::last_serror(), name);
	}
	reply_sock.unbind_sock();
}

static void reply_dummy(request_message& msg, acl::rfc1035_request& req) {
	std::vector<acl::string> addrs;
	addrs.push_back("220.181.109.164");
	addrs.push_back("220.181.109.165");
	addrs.push_back("220.181.109.166");
	reply_dummy(msg, req, addrs);
}

static void reply_dummy(request_message& msg, acl::rfc1035_request& req,
		const std::vector<std::string>& hells) {
	std::vector<acl::string> addrs;
	for (std::vector<std::string>::const_iterator cit = hells.begin();
		cit != hells.end(); ++cit) {
		addrs.push_back((*cit).c_str());
	}
	reply_dummy(msg, req, addrs);
}

static void handle_request(request_message& msg) {
	acl::rfc1035_request req;
	if (!req.parse_request(msg.data_, msg.data_.size())) {
		//logger_error("invalid request data");
		return;
	}

	const char* name = req.get_name();
	if (var_black_list->is_blocked(name)) {
		logger_warn("BLOCKED, name=%s", name);
		reply_dummy(msg, req);
		return;
	}

	acl::string matched;
	int now_hour, now_min;
	const std::vector<std::string>* hells = var_rules_option->get_hells(name,
			matched, now_hour, now_min);
	if (hells != NULL) {
		logger_warn("BLOCKED, name=%s, matched=%s, now=%d:%d",
			name, matched.c_str(), now_hour, now_min);
		reply_dummy(msg, req, *hells);
		return;
	}

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

	show_addrs(msg.peer_addr_.c_str(), name, res);

	acl::socket_stream reply_sock;
	reply_sock.open(msg.server_->sock_handle(), true);
	if (reply_sock.sendto(buf, len, msg.peer_addr_, 0) == -1) {
		logger_error("sendto reply to %s error %s, name=%s",
			msg.peer_addr_.c_str(), acl::last_serror(), name);
	}

	reply_sock.unbind_sock();

	save_record(msg, name);
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

	if (var_cfg_manage_addr && *var_cfg_manage_addr) {
		go[] {
			manage_service_start(var_cfg_manage_addr);
		};
	}

	acl::fiber::schedule();
}

void dgate_service_start(void) {
	__request_box = new acl::fiber_tbox<request_message>;

	std::thread dgate_thread(service_main, __request_box);
	dgate_thread.detach();
}

void dgate_push_request(dgate_db* db, acl::socket_stream* server,
		const char* peer_addr, const char* data, size_t dlen) {
	request_message* msg = new
		request_message(db, server, peer_addr, data, dlen);
	__request_box->push(msg);
}
