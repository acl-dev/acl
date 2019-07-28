#pragma once

class HttpClient : public acl::thread_job
{
public:
	HttpClient(const char* server_addr, const acl::string* buf);
	~HttpClient();

	// 设置是否当任务处理完毕后自动销毁本类实例，默认不自动销毁，
	// 这样可以兼容本类实例为堆栈对象和动态分配对象的情况
	void set_auto_free(bool on);

	// 基类虚函数

	void* run();

private:
	bool send();

private:
	acl::string server_addr_;
	const acl::string* buf_;
	bool auto_free_;
};


