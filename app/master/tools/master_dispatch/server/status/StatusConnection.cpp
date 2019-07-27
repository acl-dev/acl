#include "stdafx.h"
#include "status/StatusIOCallback.h"
#include "status/StatusConnection.h"

StatusConnection::StatusConnection(acl::aio_socket_stream* conn)
: IConnection(conn)
{
}

void StatusConnection::run()
{
	// 鍒涘缓鐘舵€佹眹鎶� IO 澶勭悊鍥炶皟绫诲璞�
	StatusIOCallback* callback = new StatusIOCallback(this);
	conn_->add_read_callback(callback);
	conn_->add_close_callback(callback);
	conn_->add_timeout_callback(callback);

	// 鐩戞帶寮傛娴佹槸鍚﹀彲璇�
	conn_->read_wait(var_cfg_rw_timeout);
}

void StatusConnection::close()
{
	conn_->close();
}
