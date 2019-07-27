#include "stdafx.h"
#include "rpc_manager.h"
#include "rpc_stats.h"
#include "http_rpc.h"
#include "http_client.h"

http_client::http_client(acl::aio_socket_stream* conn)
: conn_(conn)
{
	http_ = new http_rpc(conn_, (unsigned) var_cfg_echo_length);
}

http_client::~http_client()
{
	delete http_;
}

bool http_client::write_callback()
{
	return true;
}

bool http_client::timeout_callback()
{
	return false;
}

void http_client::close_callback()
{
	//logger("connection closed now, fd: %d", conn_->get_socket());
	delete this;
}

bool http_client::read_wakeup()
{
	// 娴嬭瘯鐘舵€�
	rpc_read_wait_del();
	rpc_add();

	// 鍏堢姝㈠紓姝ユ祦鐩戞帶
	conn_->disable_read();

	// 鍙戣捣涓€涓� http 浼氳瘽杩囩▼
	rpc_manager::get_instance().fork(http_);

	return true;
}
