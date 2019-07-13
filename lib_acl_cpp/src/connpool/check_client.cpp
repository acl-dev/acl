#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"
#include "acl_cpp/connpool/check_client.hpp"
#endif
#include "check_timer.hpp"

namespace acl
{

check_client::check_client(check_timer& timer, const char* addr,
	aio_socket_stream& conn, struct timeval& begin)
: blocked_(true)
, aliving_(false)
, timedout_(false)
, timer_(timer)
, conn_(conn)
, addr_(addr)
{
	memcpy(&begin_, &begin, sizeof(begin_));
}

void check_client::set_alive(bool yesno)
{
	aliving_ = yesno;
}

void check_client::set_blocked(bool on)
{
	blocked_ = on;
}

void check_client::close(void)
{
	conn_.close();
}

bool check_client::open_callback(void)
{
	set_alive(true);
	struct timeval end;
	gettimeofday(&end, NULL);
	double cost = stamp_sub(end, begin_);

	timer_.get_monitor().on_connected(*this, cost);
	timer_.get_monitor().on_open(*this);
	return true;
}

void check_client::close_callback(void)
{
	struct timeval end;
	gettimeofday(&end, NULL);
	double cost = stamp_sub(end, begin_);

	if (timedout_) {
		logger_warn("server: %s dead, timeout, spent: %.2f ms",
			addr_.c_str(), cost);
		timer_.get_monitor().on_timeout(addr_.c_str(), cost);
	} else if (!aliving_) {
		logger_warn("server: %s dead, spent: %.2f ms",
			addr_.c_str(), cost);
		timer_.get_monitor().on_refused(addr_.c_str(), cost);
	}
	//else
	//	logger("server: %s alive, spent: %.2f ms",
	//		addr_.c_str(), spent);

	// 放在此处的好处是保证了当前流肯定处于关闭过程中，同时将本检测对象
	// 从 timer_ 中删除以减少 timer_ 中检测对象的个数
	timer_.get_monitor().get_manager().set_pools_status(addr_, aliving_);
	timer_.remove_client(addr_, this);

	delete this;
}

bool check_client::timeout_callback()
{
	// 连接超时，则直接返回失败
	timedout_ = true;
	return false;
}

} // namespace acl
