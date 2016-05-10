#include "stdafx.h"
#include "redis_util.h"
#include "redis_monitor.h"

redis_monitor::redis_monitor(const char* addr, int conn_timeout,
	int rw_timeout, const char* passwd, bool prefer_master)
{
	addr_ = addr;
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	if (passwd && *passwd)
		passwd_ = passwd;
	prefer_master_ = prefer_master;
}

redis_monitor::~redis_monitor(void)
{
}

void redis_monitor::status(void)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	acl::redis redis(&client);

	std::vector<acl::redis_node*> nodes;
	redis_util::get_nodes(redis, prefer_master_, nodes);

	if (nodes.empty())
	{
		logger_error("no redis nodes available");
		return;
	}

	std::vector<acl::redis_client*> conns;
	for (std::vector<acl::redis_node*>::const_iterator
		cit = nodes.begin(); cit != nodes.end(); ++cit)
	{
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_warn("addr NULL, skip it");
			continue;
		}

		acl::redis_client* conn = new acl::redis_client(
			addr, conn_timeout_, rw_timeout_);
		conns.push_back(conn);
	}

	while (true)
	{
		show_status(conns);
		sleep(1);
	}

	for (std::vector<acl::redis_client*>::iterator it = conns.begin();
		it != conns.end(); ++it)
	{
		delete *it;
	}
}

int redis_monitor::check(const std::map<acl::string, acl::string>& info,
	const char* name, std::vector<int>& res)
{
	std::map<acl::string, acl::string>::const_iterator cit
		= info.find(name);
	if (cit == info.end())
	{
		logger_error("no %s", name);
		res.push_back(0);
		return 0;
	}
	else
	{
		int n = atoi(cit->second.c_str());
		res.push_back(n);
		return n;
	}
}

long long redis_monitor::check(const std::map<acl::string, acl::string>& info,
	const char* name, std::vector<long long>& res)
{
	std::map<acl::string, acl::string>::const_iterator cit
		= info.find(name);
	if (cit == info.end())
	{
		logger_error("no %s", name);
		res.push_back(0);
		return 0;
	}
	else
	{
		long long n = acl_atoll(cit->second.c_str());
		res.push_back(n);
		return n;
	}
}

double redis_monitor::check(const std::map<acl::string, acl::string>& info,
	const char* name, std::vector<double>& res)
{
	std::map<acl::string, acl::string>::const_iterator cit
		= info.find(name);
	if (cit == info.end())
	{
		logger_error("no %s", name);
		res.push_back(0.0);
		return 0.0;
	}
	else
	{
		double n = atof(cit->second.c_str());
		res.push_back(n);
		return n;
	}
}

int redis_monitor::check_keys(const char* name, const char* value)
{
	if (strncmp(name, "db", 2) != 0 || *value == 0)
		return 0;

	name += 2;
	if (*name == 0 || !acl_alldig(name))
		return 0;

	acl::string buf(value);
	buf.trim_space();

	int keys = 0;
	std::vector<acl::string>& tokens = buf.split2(";,");
	for (std::vector<acl::string>::iterator it = tokens.begin();
		it != tokens.end(); ++it)
	{
		std::vector<acl::string>& tokens2 = (*it).split2("=");
		if (tokens2.size() != 2)
			continue;
		if (tokens2[0] == "keys" && acl_alldig(tokens2[1]))
			keys = atoi(tokens2[1].c_str());
	}

	return keys;
}

int redis_monitor::check_keys(const std::map<acl::string, acl::string>& info,
	std::vector<int>& keys)
{
	int count = 0;

	for (std::map<acl::string, acl::string>::const_iterator cit
		= info.begin(); cit != info.end(); ++cit)
	{
		int n = check_keys(cit->first, cit->second);
		if (n > 0)
			count += n;
	}

	keys.push_back(count);
	return count;
}

static double to_human(long long n, acl::string& units)
{
	double msize;

#define KB	1024
#define MB	(1024 * 1024)
#define	GB	(1024 * 1024 * 1024)
	long long TB = (long long) GB * 1024;

	if ((msize = (double) (n / TB)) >= 1)
		units = "T";
	else if ((msize = ((double) n) / GB) >= 1)
		units = "G";
	else if ((msize = ((double) n) / MB) >= 1)
		units = "M";
	else if ((msize = ((double) n) / KB) >= 1)
		units = "K";
	else
	{
		msize = (double) n;
		units = "B";
	}

	return msize;
}

void redis_monitor::show_status(std::vector<acl::redis_client*>& conns)
{
	std::vector<acl::string> addrs;
	std::vector<int> tpses;
	std::vector<int> clients;
	std::vector<int> keys;
	std::vector<long long> memory;
	std::vector<long long> memory_rss;
	std::vector<long long> memory_peak;
	std::vector<double>    memory_frag;

	for (std::vector<acl::redis_client*>::iterator it = conns.begin();
		it != conns.end(); ++it)
	{
		acl::redis cmd(*it);
		std::map<acl::string, acl::string> info;
		if (cmd.info(info) < 0)
		{
			logger_error("cmd info error: %s, addr: %s",
				cmd.result_error(), (*it)->get_addr());
			continue;
		}

		addrs.push_back((*it)->get_addr());
		check(info, "instantaneous_ops_per_sec", tpses);
		check(info, "connected_clients", clients);
		check_keys(info, keys);
		check(info, "used_memory", memory);
		check(info, "used_memory_rss", memory_rss);
		check(info, "used_memory_peak", memory_peak);
		check(info, "mem_fragmentation_ratio", memory_frag);
	}

	int  all_tps = 0, all_client = 0, all_keys = 0;
	size_t size = addrs.size();


	acl::string units, rss_units, peak_units;
	double msize, mrss, mpeak;
	long long total_msize = 0, total_rss = 0, total_peak = 0;

	for (size_t i = 0; i < size; i++)
	{
		msize = to_human(memory[i], units);
		mrss  = to_human(memory_rss[i], rss_units);
		mpeak = to_human(memory_peak[i], peak_units);
#ifdef ACL_UNIX
		printf("\033[1;32;40mredis:\033[0m"
			" \033[1;36;40m%20s\033[0m,"
			" \033[1;33;40mtps:\033[0m"
			" \033[1;36;40m%8d\033[0m,"
			" \033[1;33;40mclients:\033[0m"
			" \033[1;36;40m%6d\033[0m,"
			" \033[1;33;40mkeys:\033[0m"
			" \033[1;36;40m%10d\033[0m,"
			" \033[1;33;40mmem:\033[0m"
			" \033[1;36;40m%8.2f %s\033[0m,"
			" \033[1;33;40mmem rss:\033[0m"
			" \033[1;36;40m%8.2f %s\033[0m,"
			" \033[1;33;40mmem peak:\033[0m"
			" \033[1;36;40m%8.2f %s\033[0m,"
			" \033[1;33;40mfrag ratio:\033[0m"
			" \033[1;36;40m%4.2f\033[0m\r\n",
#else
		printf("redis: %15s, tps: %8d, clients: %6d, keys: %10d,"
			" mem: %8.2f %s, mem rss: %8.2f %s,"
			" mem peak: %8.2f %s, frag ratio: %4.2f\r\n",
#endif
			addrs[i].c_str(), tpses[i], clients[i], keys[i],
			msize, units.c_str(), mrss, rss_units.c_str(),
			mpeak, peak_units.c_str(), memory_frag[i]);

		if (tpses[i] > 0)
			all_tps += tpses[i];
		if (clients[i] > 0)
			all_client += clients[i];
		if (keys[i] > 0)
			all_keys += keys[i];
		if (memory[i] > 0)
			total_msize += memory[i];
		if (memory_rss[i] > 0)
			total_rss += memory_rss[i];
		if (memory_peak[i] > 0)
			total_peak += memory_peak[i];
	}

	msize = to_human(total_msize, units);
	mrss  = to_human(total_rss, rss_units);
	mpeak = to_human(total_peak, peak_units);

#ifdef ACL_UNIX
	printf("\033[1;34;40mtotal masters:\033[0m"
		" \033[1;36;40m%12d\033[0m,"
		" \033[1;34;40mtps:\033[0m"
		" \033[1;36;40m%8d\033[0m,"
		" \033[1;34;40mclients:\033[0m"
		" \033[1;36;40m%6d\033[0m,"
		" \033[1;34;40mkeys:\033[0m"
		" \033[1;36;40m%10d\033[0m,"
		" \033[1;34;40mmem:\033[0m"
		" \033[1;36;40m%8.2f %s\033[0m,"
		" \033[1;34;40mmem rss:\033[0m"
		" \033[1;36;40m%8.2f %s\033[0m,"
		" \033[1;34;40mmem peak:\033[0m"
		" \033[1;36;40m%8.2f %s\033[0m\r\n",
#else
	printf("total masters: %12d, tps: %8d, clients: %6d,"
		" keys: %10d, mem: %8.2f %s, mem rss: %8.2f %s,"
		" mem peak: %8.2f %s\r\n",
#endif
		(int) size, all_tps, all_client, all_keys,
		msize, units.c_str(), mrss, rss_units.c_str(),
		mpeak, peak_units.c_str());
}
