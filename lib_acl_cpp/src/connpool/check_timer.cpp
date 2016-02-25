#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#include "acl_cpp/connpool/check_client.hpp"
#endif
#include "check_timer.hpp"

namespace acl
{

check_timer::check_timer(connect_monitor& monitor,
	aio_handle& handle, int conn_timeout)
: id_(-1)
, stopping_(false)
, monitor_(monitor)
, handle_(handle)
, conn_timeout_(conn_timeout)
{
}

void check_timer::timer_callback(unsigned int id)
{
	id_ = (int) id;

	if (stopping_)
	{
		logger("check_timer stopping ...");
		return;
	}

	connect_manager& manager = monitor_.get_manager();

	// 先提取所有服务器地址，因为要操作的对象处于多线程环境中，
	// 所以需要进行互斥锁保护
	manager.lock();

	const std::vector<connect_pool*>& pools = manager.get_pools();

	for (std::vector<connect_pool*>::const_iterator cit = pools.begin();
		cit != pools.end(); ++cit)
	{
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_warn("addr null");
			continue;
		}

		std::map<string, int>::iterator it = addrs_.find(addr);
		if (it == addrs_.end())
			addrs_[addr] = 1;
		else
			it->second++;
	}

	manager.unlock();

	if (addrs_.empty())
		logger_warn(">>>no addr been set!<<<");

	// 连接所有服务器地址

	struct timeval begin;
	std::map<string, int>::iterator cit_next;

	for (std::map<string, int>::iterator cit = addrs_.begin();
		cit != addrs_.end(); cit = cit_next)
	{
		cit_next = cit;
		++cit_next;

		// 如果该值大于 1 则说明该地址的上一个检测还未结束
		if (cit->second > 1)
			continue;

		gettimeofday(&begin, NULL);

		const char* addr = cit->first.c_str();
		aio_socket_stream* conn = aio_socket_stream::open(&handle_,
			addr, conn_timeout_);
		if (conn == NULL)
		{
			logger_warn("connect server: %s error", addr);
			manager.set_pools_status(addr, false);
			addrs_.erase(cit);
		}
		else
		{
			check_client* checker = new check_client(*this,
				addr, *conn, begin);
			conn->add_open_callback(checker);
			conn->add_close_callback(checker);
			conn->add_timeout_callback(checker);
			checkers_.push_back(checker);
		}
	}
}

void check_timer::remove_client(const char* addr, check_client* checker)
{
	// 从当前检查服务器地址列表中删除当前的检测地址
	std::map<string, int>::iterator it1 = addrs_.find(addr);
	if (it1 != addrs_.end())
		addrs_.erase(it1);
	else
		logger_warn("not found addr: %s", addr);

	// 从检测连接集群中删除本连接对象
	for (std::vector<check_client*>::iterator it2 = checkers_.begin();
		it2 != checkers_.end(); ++it2)
	{
		if ((*it2) == checker)
		{
			checkers_.erase(it2);
			break;
		}
	}
}

bool check_timer::finish(bool graceful)
{
	if (!graceful || checkers_.empty())
		return true;

	// 需要等待所有检测连接关闭

	if (id_ >= 0)
	{
		handle_.del_timer(this, id_);
		id_ = -1;
		keep_timer(false);
	}

	// 遍历当前所有正在检测的处于非阻塞状态的连接检测对象，异步关闭之

	for (std::vector<check_client*>::iterator it = checkers_.begin();
		it != checkers_.end(); ++it)
	{
		// 在阻塞检测方式如果该检测对象处于阻塞状态，则不能直接关闭
		if (!(*it)->blocked())
			(*it)->close();
	}

	return false;
}

} // namespace acl
