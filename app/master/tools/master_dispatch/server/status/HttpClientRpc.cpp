#include "stdafx.h"
#include "status/HttpClient.h"
#include "status/HttpClientRpc.h"

/////////////////////////////////////////////////////////////////////////////

HttpClientRpc::HttpClientRpc(acl::string* buf, const char* server_addrs)
: buf_(buf)
, server_addrs_(server_addrs)
{
}

HttpClientRpc::~HttpClientRpc()
{
	delete buf_;
}

/////////////////////////////////////////////////////////////////////////////

// 瀛愮嚎绋嬩腑杩愯
void HttpClientRpc::rpc_run()
{
	std::vector<acl::string>& servers = server_addrs_.split2(";, \t");
	size_t size = servers.size();
	if (size == 0)
	{
		logger_error("invalid server_addrs: %s", server_addrs_.c_str());
		return;
	}
	else if (size == 1)
	{
		// 褰撳彧鏈変竴涓鐞嗘湇鍔″櫒鏃讹紝鍒欎笉蹇呭惎鍔ㄧ嚎绋嬶紝鐩存帴鍦ㄦ湰绾跨▼涓鐞�
		HttpClient job(servers[0], buf_);
		job.run();
		return;
	}

	// 褰撻渶瑕佸悜澶氫釜绠＄悊鏈嶅姟锛屽垯涓轰簡淇濊瘉瀹炴椂鎬э紝鍒欏皢鍙戦€佷换鍔℃斁鍦ㄧ嚎绋嬫睜
	// 涓幓澶勭悊
	acl::thread_pool threads;

	// 鍚姩绾跨▼姹犺繃绋�
	threads.start();

	// 灏嗗彂閫佷换鍔′氦缁欑嚎绋嬫睜澶勭悊
	std::vector<acl::string>::const_iterator cit = servers.begin();
	for (; cit != servers.end(); ++cit)
	{
		HttpClient* job = new HttpClient((*cit).c_str(), buf_);
		job->set_auto_free(true);
		threads.execute(job);
	}

	// 绛夊緟鎵€鏈夌嚎绋嬪彂閫佸畬姣�
	threads.wait();
}


/////////////////////////////////////////////////////////////////////////////

// 涓荤嚎绋嬩腑杩愯
void HttpClientRpc::rpc_onover()
{
	delete this;
}
