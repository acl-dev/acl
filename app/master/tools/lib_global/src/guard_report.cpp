#include "stdafx.h"
#include "guard_report.h"

#define SET_TIME(x) do { \
  struct timeval _tv; \
  gettimeofday(&_tv, NULL); \
  (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
} while (0)

guard_report::guard_report(const char* guard_manager, acl::tcp_ipc& ipc,
	int conn_timeout /* = 10 */, int rw_timeout /* = 10 */)
: guard_manager_(guard_manager)
, ipc_(ipc)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
}

bool guard_report::report(const acl::string& body)
{
	if (body.size() >= 1460)
		return tcp_report(body);
	else
		return udp_report(body);
}

bool guard_report::resolve_domain(std::vector<acl::string>& ips, int& port)
{
	acl::string domain(guard_manager_);
	char* ptr = domain.c_str();
	char* port_s = strchr(ptr, ':');
	if (port_s == NULL || port_s == ptr || *(port_s + 1) == 0) {
		logger_error("invalid guard_manager=%s", guard_manager_.c_str());
		return false;
	}
	*port_s++ = 0;

	int h_error;
	ACL_DNS_DB *db = acl_gethostbyname(domain.c_str(), &h_error);
	if (db == NULL) {
		logger_error("acl_gethostbyname error %s, domain=%s",
			acl_netdb_strerror(h_error), domain.c_str());
		return false;
	}

	ACL_ITER iter;
	acl_foreach(iter, db) {
		ACL_HOSTNAME* host = (ACL_HOSTNAME *) iter.data;
		ips.push_back(host->ip);
	}
	acl_netdb_free(db);
	if (ips.empty()) {
		logger_error("no ip found, domain=%s", domain.c_str());
		return false;
	}

	port = atoi(port_s);
	return true;
}

bool guard_report::get_one_addr(acl::string& addr)
{
	std::vector<acl::string> ips;
	int port;

	if (resolve_domain(ips, port) == false)
		return false;

	long long n;
	SET_TIME(n);

	n = n % ips.size();
	addr.format("%s:%d", ips[n].c_str(), port);
	return true;
}

bool guard_report::tcp_report(const acl::string& body)
{
	acl::string addr;
	if (get_one_addr(addr) == false)
		return false;

	if (ipc_.send(addr, body, (unsigned) body.size()) == false) {
		logger_error("send to %s error %s",
			addr.c_str(), acl::last_serror());
		return false;
	}

	return true;
}

bool guard_report::udp_report(const acl::string& body)
{
	acl::string addr;
	if (get_one_addr(addr) == false)
		return false;

//	printf("addr=%s, %s\n", guard_manager_.c_str(), addr.c_str());
	const char* local_addr = "0.0.0.0:0";
	acl::socket_stream stream;
	if (stream.bind_udp(local_addr) == false) {
		logger_error("bind_udp %s error %s",
			local_addr, acl::last_serror());
		return false;
	}
	stream.set_peer(addr);
	if (stream.write(body) == false) {
		logger_error("write %s error %s",
			body.c_str(), acl::last_serror());
		return false;
	}

	//printf("write ok, body=[%s]\r\n", body.c_str());
	return true;
}
