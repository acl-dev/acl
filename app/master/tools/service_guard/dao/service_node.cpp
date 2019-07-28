#include "stdafx.h"
#include "service_node.h"

#define SERVICE		"service"
#define SERVICES	"services"
#define MACHINES	"machines"
#define MAIN_SERVICES	"main_services"

service_node::service_node(acl::redis_client_cluster& redis)
: redis_(redis)
, when_(time(NULL))
{
}

bool service_node::save(const char* ip, const service_list_res_t& res)
{
	when_ = time(NULL);

	acl::string key;
	std::map<acl::string, double> services;
	std::map<acl::string, double> ip_services;

	for (std::vector<service_info_t>::const_iterator cit = res.data.begin();
		cit != res.data.end(); ++cit) {

		services[(*cit).conf] = when_;

		key.format("%s|%s", ip, (*cit).conf.c_str());
		ip_services[key] = when_;
	}

	//////////////////////////////////////////////////////////////////////

	bool ret = true;

	// add services for each ip into redis zset
	if (add_machine_services(ip, services) == false)
		ret = false;

	// add all services for one key into redis zset
	if (add_services(ip_services) == false)
		ret = false;

	// add all machine IP for one key into redis zet;
	if (add_machines(ip) == false)
		ret = false;

	// add machine IP if where're main service
	if (add_main_services(ip, services) == false)
		ret = false;

	return ret;
}

bool service_node::add_machine_services(const char* ip,
	const std::map<acl::string, double>& services)
{
	acl::redis cmd(&redis_);
	if (cmd.zadd(ip, services) == -1)
	{
		logger_error("zadd error=%s, key=%s, ip=%s, members=%d",
			cmd.result_error(), ip, ip, (int) services.size());
		return false;
	}

	return true;
}

bool service_node::add_services(const std::map<acl::string, double>& services)
{
	acl::redis cmd(&redis_);
	if (cmd.zadd(SERVICES, services) == -1)
	{
		logger_error("zadd error=%s, key=%s, members=%d",
			cmd.result_error(), SERVICES, (int) services.size());
		return false;
	}

	return true;

}

bool service_node::add_machines(const char* ip)
{
	std::map<acl::string, double> members;
	members[ip] = when_;
	acl::redis cmd(&redis_);

	if (cmd.zadd(MACHINES, members) == -1)
	{
		logger_error("zadd error=%s, key=%s, ip=%s",
			cmd.result_error(), MACHINES, ip);
		return false;
	}

	return true;
}

bool service_node::add_main_services(const char* ip,
	const std::map<acl::string, double>& services)
{
	for (std::map<acl::string, double>::const_iterator cit =
		services.begin(); cit != services.end(); ++cit)
	{
		acl::string buf(cit->first);
		char* key = strrchr(buf.c_str(), '/');
		if (key)
			*key++ = 0;
		else
			key = buf.c_str();
		if (*key == 0)
		{
			logger_error("invalid service=%s", cit->first.c_str());
			continue;
		}
		
		if (var_main_service_list.find(key)
			!= var_main_service_list.end())
		{
			add_main_service(key, ip);
		}
	}

	return true;
}

bool service_node::add_main_service(const char* service, const char* ip)
{
//	printf("add service=%s, ip=%s\r\n", service, ip);

	std::map<acl::string, double> members;
	members[ip] = when_;
	acl::redis cmd(&redis_);

	acl::string key;
	key.format("%s|%s", SERVICE, service);
	if (cmd.zadd(key, members) == -1)
	{
		logger_error("zadd error=%s, key=%s, ip=%s",
			cmd.result_error(), service, ip);
		return false;
	}

	return true;
}
