#include "stdafx.h"
#include "status/StatusIOCallback.h"
#include "status/StatusConnection.h"

StatusConnection::StatusConnection(acl::aio_socket_stream* conn)
: IConnection(conn)
{
}

void StatusConnection::run()
{
	// 创建状态汇报 IO 处理回调类对象
	StatusIOCallback* callback = new StatusIOCallback(this);
	conn_->add_read_callback(callback);
	conn_->add_close_callback(callback);
	conn_->add_timeout_callback(callback);

	// 监控异步流是否可读
	conn_->read_wait(var_cfg_rw_timeout);
}

void StatusConnection::close()
{
	conn_->close();
}
