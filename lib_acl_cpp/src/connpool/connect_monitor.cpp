#include "acl_stdafx.hpp"
#include <map>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/aio_timer_callback.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"

namespace acl
{

//////////////////////////////////////////////////////////////////////////////

static double stamp_sub(const struct timeval& from, const struct timeval& sub)
{
	struct timeval res;

	memcpy(&res, &from, sizeof(struct timeval));

	res.tv_usec -= sub.tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub.tv_sec;

	return res.tv_sec * 1000.0 + res.tv_usec/1000.0;
}

/**
 * 异步连接回调函数处理类
 */
class check_client : public aio_open_callback
{
public:
	check_client(connect_manager& manager,
		const char* addr,
		std::map<string, int>& addrs,
		aio_socket_stream* conn,
		std::vector<aio_socket_stream*>& conns,
		struct timeval& begin);

	~check_client() {}

private:
	// 基类虚函数
	bool open_callback();
	void close_callback();
	bool timeout_callback();

private:
	typedef enum
	{
		CHECK_S_REFUSED,
		CHECK_S_CONNECTED,
		CHECK_S_TIMEDOUT
	} check_status_t;

	check_status_t status_;
	struct timeval begin_;
	connect_manager& manager_;
	aio_socket_stream* conn_;
	string addr_;
	std::map<string, int>& addrs_;
	std::vector<aio_socket_stream*>& conns_;
};

check_client::check_client(connect_manager& manager,
	const char* addr,
	std::map<string, int>& addrs,
	aio_socket_stream* conn,
	std::vector<aio_socket_stream*>& conns,
	struct timeval& begin)
: status_(CHECK_S_REFUSED)
, manager_(manager)
, conn_(conn)
, addr_(addr)
, addrs_(addrs)
, conns_(conns)
{
	memcpy(&begin_, &begin, sizeof(begin_));
}

bool check_client::open_callback()
{
	status_ = CHECK_S_CONNECTED;

	return false;  // 返回 false 让基类关闭连接
}

void check_client::close_callback()
{
	struct timeval end;
	gettimeofday(&end, NULL);
	double spent = stamp_sub(end, begin_);

	// 如果未成功连接服务器，则设置该连接池状态为不可用状态，
	// 否则设置为存活状态

	if (status_ == CHECK_S_TIMEDOUT)
		logger_warn("server: %s dead, timeout, spent: %.2f ms",
			addr_.c_str(), spent);
	else if (status_ == CHECK_S_REFUSED)
		logger_warn("server: %s dead, refused, spent: %.2f ms",
			addr_.c_str(), spent);
	//else if (status_ == CHECK_S_CONNECTED)
	//	logger("server: %s alive, spent: %.2f ms",
	//		addr_.c_str(), spent);

	// 从当前检查服务器地址列表中删除当前的检测地址
	std::map<string, int>::iterator it1 = addrs_.find(addr_);
	if (it1 != addrs_.end())
		addrs_.erase(it1);

	manager_.set_pools_status(addr_, status_ == CHECK_S_CONNECTED
		? true : false);

	// 从检测连接集群中删除本连接对象
	std::vector<aio_socket_stream*>::iterator it2 = conns_.begin();
	for (; it2 != conns_.end(); ++it2)
	{
		if ((*it2) == conn_)
		{
			conns_.erase(it2);
			break;
		}
	}

	delete this;
}

bool check_client::timeout_callback()
{
	status_ = CHECK_S_TIMEDOUT;

	// 连接超时，则直接返回失败
	return false;
}

//////////////////////////////////////////////////////////////////////////

class check_timer : public aio_timer_callback
{
public:
	check_timer(connect_manager& manager, aio_handle& handle,
		int conn_timeout);
	~check_timer() {}

	bool finish(bool graceful);

protected:
	// 基类纯虚函数
	void timer_callback(unsigned int id);
	void destroy(void) {}

private:
	int id_;
	bool stopping_;
	connect_manager& manager_;
	aio_handle& handle_;
	int   conn_timeout_;
	std::map<string, int> addrs_;
	std::vector<aio_socket_stream*> conns_;
};

check_timer::check_timer(connect_manager& manager,
	aio_handle& handle, int conn_timeout)
: id_(-1)
, stopping_(false)
, manager_(manager)
, handle_(handle)
, conn_timeout_(conn_timeout)
{
}

void check_timer::timer_callback(unsigned int id)
{
	id_ = (int) id;

	if (stopping_)
		return;

	const char* addr;
	std::map<string, int>::iterator cit1;

	// 先提取所有服务器地址

	manager_.lock();

	const std::vector<connect_pool*>& pools = manager_.get_pools();
	std::vector<connect_pool*>::const_iterator cit2 = pools.begin();

	for (; cit2 != pools.end(); ++cit2)
	{
		addr = (*cit2)->get_addr();
		if (addr == NULL || *addr == 0)
			continue;

		cit1 = addrs_.find(addr);
		if (cit1 == addrs_.end())
			addrs_[addr] = 1;
		else
			cit1->second++;
	}

	manager_.unlock();

	// 连接所有服务器地址

	struct timeval begin;

	aio_socket_stream* conn;
	check_client* checker;
	std::map<string, int>::iterator cit1_next;

	for (cit1 = addrs_.begin(); cit1 != addrs_.end(); cit1 = cit1_next)
	{
		cit1_next = cit1;
		++cit1_next;

		if (cit1->second > 1)
			continue;

		gettimeofday(&begin, NULL);

		addr = cit1->first.c_str();
		conn = aio_socket_stream::open(&handle_,
			addr, conn_timeout_);
		if (conn == NULL)
		{
			manager_.set_pools_status(addr, false);
			addrs_.erase(cit1);
		}
		else
		{
			checker = new check_client(manager_, addr,
				addrs_, conn, conns_, begin);
			conn->add_open_callback(checker);
			conn->add_close_callback(checker);
			conn->add_timeout_callback(checker);
			conns_.push_back(conn);
		}
	}
}

bool check_timer::finish(bool graceful)
{
	if (!graceful || conns_.empty())
		return true;

	// 需要等待所有检测连接关闭

	if (id_ >= 0)
	{
		handle_.del_timer(this, id_);
		id_ = -1;
		keep_timer(false);
	}

	// 遍历当前所有正在检测的连接，异步关闭之
	std::vector<aio_socket_stream*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
		(*it)->close();
	return false;
}

//////////////////////////////////////////////////////////////////////////

connect_monitor::connect_monitor(connect_manager& manager,
	int check_inter /* = 1 */, int conn_timeout /* = 10 */)
: stop_(false)
, stop_graceful_(true)
, handle_(ENGINE_KERNEL)
, manager_(manager)
, check_inter_(check_inter)
, conn_timeout_(conn_timeout)
{

}

connect_monitor::~connect_monitor()
{

}

void connect_monitor::stop(bool graceful)
{
	stop_ = true;
	stop_graceful_ = graceful;
}

void* connect_monitor::run()
{
	// 检查服务端连接状态定时器
	check_timer timer(manager_, handle_, conn_timeout_);

	timer.keep_timer(true);  // 保持定时器
	handle_.set_timer(&timer, check_inter_ * 1000000);

	while (!stop_)
		handle_.check();

	// 等待定时器结束
	while (!timer.finish(stop_graceful_))
		handle_.check();

	return NULL;
}

} // namespace acl
