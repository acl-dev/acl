#pragma once

class http_job : public acl::thread_job
{
public:
	http_job(acl::thread_pool& thrpool, const char* url,
		const char* dns_ip, int dns_port);

private:
	// 要求类实例必须是动态创建
	~http_job();

protected:
	// 基类纯虚函数
	virtual void* run();
	
private:
	acl::thread_pool& thrpool_;
	acl::string url_;
	acl::string dns_ip_;
	int   dns_port_;

private:
	bool dns_lookup(const char* domain, std::vector<acl::string>& ips);
};
