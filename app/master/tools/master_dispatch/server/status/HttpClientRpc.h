#pragma once

class HttpClientRpc : public acl::rpc_request
{
public:
	HttpClientRpc(acl::string* buf, const char* server_addrs);

protected:
	// 瀹炵幇鍩虹被铏氬嚱鏁�

	// 鍦ㄥ瓙绾跨▼寮€濮嬭繍琛�
	void rpc_run();

	// 褰� rpc_run 杩斿洖鍚庡湪涓荤嚎绋嬩腑杩愯
	void rpc_onover();

private:
	acl::string* buf_;
	acl::string  server_addrs_;

	~HttpClientRpc();
};
