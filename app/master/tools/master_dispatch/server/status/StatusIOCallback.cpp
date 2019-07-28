#include "stdafx.h"
#include "rpc_manager.h"
#include "status/HttpServerRpc.h"
#include "status/StatusConnection.h"
#include "status/StatusIOCallback.h"

StatusIOCallback::StatusIOCallback(StatusConnection* conn)
: conn_(conn)
{
}

StatusIOCallback::~StatusIOCallback()
{
	delete conn_;
}

// 当管理连接有数据可读时
bool StatusIOCallback::read_wakeup()
{
	// 先禁止异步流监控
	conn_->get_conn()->disable_read();

	// 发起一个 http 服务端会话过程，将之交由子线程去处理
	HttpServerRpc* rpc = new HttpServerRpc(conn_->get_conn());
	rpc_manager::get_instance().fork(rpc);
	return true;
}

// 当非阻塞流被关闭时，该回调函数将被调用
void StatusIOCallback::close_callback()
{
	// 删除自己
	delete this;
}

bool StatusIOCallback::timeout_callback()
{
	logger_error("read timeout from: %s", conn_->get_peer());
	return false;
}
