#include "stdafx.h"
#include "rpc_manager.h"
#include "server/ServerManager.h"
#include "status/HttpClientRpc.h"
#include "status/StatusTimer.h"

StatusTimer::StatusTimer()
{
}

StatusTimer::~StatusTimer()
{
}

void StatusTimer::destroy()
{
	delete this;
}

void StatusTimer::timer_callback(unsigned int)
{
	acl::string* buf = new acl::string(256);
	ServerManager::get_instance().statusToJson(*buf);

	// 发起一个 HTTP 请求过程，将之将由子线程处理
	HttpClientRpc* rpc = new HttpClientRpc(buf, var_cfg_status_servers);
	rpc_manager::get_instance().fork(rpc);
}
