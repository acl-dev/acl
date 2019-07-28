#include "stdafx.h"
#include "redis_util.h"
#include "redis_cmdump.h"

//////////////////////////////////////////////////////////////////////////////

class qitem : public acl::thread_qitem
{
public:
	qitem(const acl::string& addr, const acl::string& msg)
	{
		time_t now = time(NULL);
		char buf[128];
		acl::rfc822 rfc;
		rfc.mkdate_cst(now, buf, sizeof(buf));

		msg_.format("%s: %s--%s\r\n", buf, addr.c_str(), msg.c_str());
	}

	~qitem(void)
	{
	}

	const acl::string& get_msg(void) const
	{
		return msg_;
	}

private:
	acl::string msg_;
};

//////////////////////////////////////////////////////////////////////////////

class dump_thread : public acl::thread
{
public:
	dump_thread(const acl::string& addr, const char* passwd,
			acl::thread_queue& queue)
	: stopped_(false)
	, addr_(addr)
	, queue_(queue)
	, conn_timeout_(0)
	, rw_timeout_(0)
	{
		if (passwd && *passwd) {
			passwd_ = passwd;
		}
	}

	~dump_thread(void)
	{
	}

	void* run(void)
	{
		acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
		client.set_password(passwd_);
		acl::redis redis(&client);
		if (!redis.monitor()) {
			logger_error("redis monitor error: %s, addr: %s",
				redis.result_error(), addr_.c_str());
			return NULL;
		}

		acl::string buf;
		while (!stopped_) {
			if (!redis.get_command(buf)) {
				logger_error("redis get_command error: %s",
					redis.result_error());
				break;
			}

			qitem* item = new qitem(addr_, buf);
			queue_.push(item);

			buf.clear();
		}

		return NULL;
	}

	void stop(void)
	{
		stopped_ = true;
		logger_error("Thread(%lu) stopping now", thread_id());
	}

private:
	bool stopped_;
	acl::string addr_;
	acl::string passwd_;
	acl::thread_queue& queue_;
	int conn_timeout_;
	int rw_timeout_;
};

//////////////////////////////////////////////////////////////////////////////

redis_cmdump::redis_cmdump(const char* addr, int conn_timeout, int rw_timeout,
	const char* passwd, bool prefer_master)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, prefer_master_(prefer_master)
{
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
}

redis_cmdump::~redis_cmdump(void)
{
}

void redis_cmdump::get_nodes(std::vector<acl::string>& addrs)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);

	std::vector<acl::redis_node*> nodes;

	redis_util::get_nodes(redis, prefer_master_, nodes);
	if (nodes.empty()) {
		logger_error("nodes NULL!");
		return;
	}

	for (std::vector<acl::redis_node*>::const_iterator cit
		= nodes.begin(); cit != nodes.end(); ++cit) {
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0) {
			logger_error("addr null");
			continue;
		}

		addrs.push_back(addr);
	}
}

void redis_cmdump::saveto(const char* filepath, bool dump_all)
{
	if (filepath == NULL || *filepath == 0) {
		logger_error("filepath null");
		return;
	}

	std::vector<acl::string> addrs;
	if (dump_all) {
		get_nodes(addrs);
		if (addrs.empty()) {
			logger_error("no master available!");
			return;
		}
	} else {
		addrs.push_back(addr_);
	}

	acl::ofstream out;
	if (!out.open_append(filepath)) {
		logger_error("open %s error: %s", filepath, acl::last_serror());
		return;
	}

	acl::thread_queue queue;

	std::vector<dump_thread*> threads;
	for (std::vector<acl::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit) {

		dump_thread* thread = new dump_thread((*cit), passwd_, queue);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	while (true) {
		qitem* item = (qitem*) queue.pop();
		if (item == NULL) {
			logger_error("queue pop error");
			break;
		}

		const acl::string& msg = item->get_msg();
		if (msg.empty()) {
			delete item;
			continue;
		}

		if (out.write(msg) == -1) {
			logger_error("write to %s error %s", filepath,
				acl::last_serror());
			delete item;
			break;
		}

		delete item;
	}

	for (std::vector<dump_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->stop();
	}

	for (std::vector<dump_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}
}
