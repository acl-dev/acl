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

	// 鍙戣捣涓€涓� HTTP 璇锋眰杩囩▼锛屽皢涔嬪皢鐢卞瓙绾跨▼澶勭悊
	HttpClientRpc* rpc = new HttpClientRpc(buf, var_cfg_status_servers);
	rpc_manager::get_instance().fork(rpc);
}
