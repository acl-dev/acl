#include "stdafx.h"
#include "server/ServerManager.h"
#include "server/ServerConnection.h"
#include "server/ServerIOCallback.h"

ServerIOCallback::ServerIOCallback(ServerConnection* conn)
: conn_(conn)
{
}

ServerIOCallback::~ServerIOCallback()
{
	ServerManager::get_instance().del(conn_);
	delete conn_;
}

bool ServerIOCallback::read_callback(char* data, int len)
{
	const char* myname = "read_callback";

	if (data == NULL || *data == 0 || len <= 0)
	{
		logger_warn("invalid data: %s, len: %d",
			data ? data : "null", len);
		return false;
	}

	// 处理服务端发来的命令

	acl::url_coder coder;
	coder.decode(data);

	logger_debug(DEBUG_SVR, 1, "client: %s", data);

	const char* ptr = coder.get("count");
	if (ptr == NULL)
	{
		logger_warn("%s(%d), %s: no count", __FILE__, __LINE__, myname);
		return true;
	}
	unsigned int n = (unsigned int) atoi(ptr);
	conn_->set_conns(n);

	ptr = coder.get("used");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_used(n);
	}

	ptr = coder.get("pid");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_pid(n);
	}

	ptr = coder.get("max_threads");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_max_threads(n);
	}

	ptr = coder.get("curr_threads");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_curr_threads(n);
	}

	ptr = coder.get("busy_threads");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_busy_threads(n);
	}

	ptr = coder.get("qlen");
	if (ptr && *ptr)
	{
		n = (unsigned int) atoi(ptr);
		conn_->set_qlen(n);
	}

	ptr = coder.get("type");
	if (ptr && *ptr)
		conn_->set_type(ptr);

	// 尝试将服务端连接对象添加进服务端管理对象中
	ServerManager::get_instance().set(conn_);

	return true;
}

void ServerIOCallback::close_callback()
{
	delete this;
}

bool ServerIOCallback::timeout_callback()
{
	return true;
}
