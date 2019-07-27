#pragma once

class HttpClient : public acl::thread_job
{
public:
	HttpClient(const char* server_addr, const acl::string* buf);
	~HttpClient();

	// 璁剧疆鏄惁褰撲换鍔″鐞嗗畬姣曞悗鑷姩閿€姣佹湰绫诲疄渚嬶紝榛樿涓嶈嚜鍔ㄩ攢姣侊紝
	// 杩欐牱鍙互鍏煎鏈被瀹炰緥涓哄爢鏍堝璞″拰鍔ㄦ€佸垎閰嶅璞＄殑鎯呭喌
	void set_auto_free(bool on);

	// 鍩虹被铏氬嚱鏁�

	void* run();

private:
	bool send();

private:
	acl::string server_addr_;
	const acl::string* buf_;
	bool auto_free_;
};


