#include "stdafx.h"
#include "util.h"
#include "http_thread.h"
#include "http_job.h"

http_job::http_job(acl::thread_pool& thrpool, const char* url,
	const char* dns_ip, int dns_port)
: thrpool_(thrpool)
, url_(url)
, dns_ip_(dns_ip)
, dns_port_(dns_port)
{
}

http_job::~http_job()
{
}

void* http_job::run()
{
	char  addr[256];
	if (acl::http_utils::get_addr(url_, addr, sizeof(addr)) == false)
	{
		logger_error("invalid url: %s", url_.c_str());
		return NULL;
	}

	char* domain = addr, *ptr = strchr(domain, ':');
	int   port;
	if (ptr == NULL)
		port = 80;
	else
	{
		*ptr++ = 0;
		port = atoi(ptr);
	}

	std::vector<acl::string> ips;

	struct timeval begin, end;

	gettimeofday(&begin, NULL);

	// 查询域名的 IP 列表
	if (dns_lookup(domain, ips) == false)
	{
		delete this;
		return NULL;
	}

	gettimeofday(&end, NULL);

	// 计算 DNS 的查询耗时
	double spent = util::stamp_sub(&end, &begin);

	std::vector<acl::thread*> threads;

	// 遍历 IP 地址列表，每一个 IP 创建一个 HTTP 客户端线程
	std::vector<acl::string>::const_iterator cit = ips.begin();
	for (; cit != ips.end(); ++cit)
	{
		// 创建并启动一个线程对象
		acl::thread* thr = new http_thread(domain, (*cit).c_str(),
				port, url_.c_str(), spent);
		thr->set_detachable(false);  // 设置线程为非分离状态
		thr->start();
		threads.push_back(thr);
	}

	// 等待所有 HTTP 工作线程执行完毕
	std::vector<acl::thread*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		acl::thread* thr = (*it);
		thr->wait();
		delete thr;
	}

	delete this;  // 自销毁
	return NULL;
}

bool http_job::dns_lookup(const char* domain, std::vector<acl::string>& ips)
{
	ACL_RES *res;
	ACL_DNS_DB *dns_db;

	res = acl_res_new(dns_ip_.c_str(), dns_port_);

	// 直接向 DNS 服务器发送 DNS 查询数据包
	dns_db = acl_res_lookup(res, domain);
	if (dns_db == NULL)
	{
		logger_error("acl_res_lookup failed, dns addr: %s:%d, domain: %s",
			dns_ip_.c_str(), dns_port_, domain);
		acl_res_free(res);
		return false;
	}

	ACL_ITER iter;
	acl_foreach(iter, dns_db)
	{
		ACL_HOST_INFO* info = (ACL_HOST_INFO*) iter.data;
		ips.push_back(info->ip);
	}

	acl_res_free(res);
	acl_netdb_free(dns_db);

	if (ips.empty())
	{
		logger_error("no ip for domain: %s", domain);
		return false;
	}

	return true;
}
