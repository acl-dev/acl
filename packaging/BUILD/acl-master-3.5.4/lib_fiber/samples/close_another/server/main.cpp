#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_client;
static std::map<acl::string, fiber_client*> __users;

class fiber_client : public acl::fiber {
public:
	fiber_client(acl::socket_stream* conn, bool use_kill)
	: conn_(conn)
	, kicked_(false)
	, use_kill_(use_kill)
	{}

	acl::socket_stream& get_conn(void) {
		return *conn_;
	}

	void set_kicked(void) {
		kicked_ = true;
	}

protected:
	// @override
	void run(void) {
		if (!user_login()) {
			delete this;
			return;
		}

		char buf[8192];
		while (true) {
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1) {
				printf("read error=%s\r\n", acl::last_serror());
				break;
			}
			if (strncasecmp(buf, "quit", 4) == 0) {
				conn_->format("Bye %s!\r\n", name_.c_str());
				break;
			}
			if (conn_->write(buf, ret) == -1) {
				printf("write error=%s\r\n", acl::last_serror());
				break;
			}
		}

		if (!kicked_) {
			std::map<acl::string, fiber_client*>::iterator
				it = __users.find(name_);
			if (it != __users.end()) {
				__users.erase(it);
			}
		}

		errno = 0;
		printf("user: %s logout\r\n", name_.c_str());
		delete this;
	}

private:
	acl::socket_stream* conn_;
	acl::string name_;
	bool kicked_;
	bool use_kill_;

	bool user_login(void) {
		if (!conn_->gets(name_)) {
			printf("gets error\r\n");
			return false;
		}

		std::map<acl::string, fiber_client*>::iterator it = __users.find(name_);
		if (it == __users.end()) {
			printf(">>>new user: %s, users: %zd\r\n",
				name_.c_str(), __users.size());
			if (conn_->format("Welcome %s!\r\n", name_.c_str()) > 0) {
				__users[name_] = this;
				return true;
			} else {
				printf("write error: %s\r\n", acl::last_serror());
				return false;
			}
		}

		__users.erase(it);
		it->second->set_kicked();

		acl::socket_stream& conn = it->second->get_conn();
		int fd = conn.sock_handle();

		if (use_kill_) {
			it->second->kill();
		} else {
			conn.unbind_sock();
			close(fd);
		}

		printf("Kick old %s, fd=%d\r\n", name_.c_str(), fd);

		if (conn_->format("Welcome %s!\r\n", name_.c_str()) > 0) {
			__users[name_] = this;
			return true;
		} else {
			printf("write to %s error %s\r\n",
				name_.c_str(), acl::last_serror());
			return false;
		}
	}

	~fiber_client(void) {
		delete conn_;
	}
};

class fiber_server : public acl::fiber {
public:
	fiber_server(acl::server_socket& ss, bool use_kill)
	: ss_(ss)
	, use_kill_(use_kill)
	{}

	~fiber_server(void) {}

protected:
	// @override
	void run(void) {
		while (true) {
			acl::socket_stream* conn = ss_.accept();
			if (conn == NULL) {
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			printf("accept ok, fd: %d\r\n", conn->sock_handle());
			// create one fiber for one connection
			fiber_client* fc = new fiber_client(conn, use_kill_);
			// start the client fiber
			fc->start();
		}
	}

private:
	acl::server_socket& ss_;
	bool use_kill_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -e event_type[select|poll|kernel, default: kernel]\r\n"
		" -k [use kill, default: false]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch;

	acl::fiber_event_t event = acl::FIBER_EVENT_T_KERNEL;
	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);
	bool use_kill = false;

	while ((ch = getopt(argc, argv, "hs:e:k")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'e':
			if (strcmp(optarg, "poll") == 0) {
				event = acl::FIBER_EVENT_T_POLL;
			} else if (strcmp(optarg, "select") == 0) {
				event = acl::FIBER_EVENT_T_SELECT;
			}
			break;
		case 'k':
			use_kill = true;
			break;
		default:
			break;
		}
	}

	switch (event) {
	case acl::FIBER_EVENT_T_SELECT:
		printf("use select\r\n");
		break;
	case acl::FIBER_EVENT_T_POLL:
		printf("use poll\r\n");
		break;
	case acl::FIBER_EVENT_T_KERNEL:
		printf("use kernel\r\n");
		break;
	default:
		printf("unknown event=%d\r\n", (int) event);
		return 1;
	}

	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	fiber_server fs(ss, use_kill);
	fs.start();

	acl::fiber::schedule_with(event);
	return 0;
}
