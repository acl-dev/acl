#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

connect_manager::connect_manager()
: default_pool_(NULL)
, service_idx_(0)
, stat_inter_(1)
{
}

connect_manager::~connect_manager()
{
	std::vector<connect_pool*>::iterator it = pools_.begin();

	// default_pool_ 已经包含在 pools_ 里了
	for (; it != pools_.end(); ++it)
		delete *it;
}

// 分析一个服务器地址，格式：IP:PORT[:MAX_CONN]
// 返回值 < 0 表示非法的地址
static int check_addr(const char* addr, string& buf, int default_count)
{
	// 数据格式：IP:PORT[:CONNECT_COUNT]
	ACL_ARGV* tokens = acl_argv_split(addr, ":|");
	if (tokens->argc < 2)
	{
		logger_error("invalid addr: %s", addr);
		acl_argv_free(tokens);
		return -1;
	}

	int port = atoi(tokens->argv[1]);
	if (port <= 0 || port >= 65535)
	{
		logger_error("invalid addr: %s, port: %d", addr, port);
		acl_argv_free(tokens);
		return -1;
	}
	buf.format("%s:%d", tokens->argv[0], port);
	int conn_max;
	if (tokens->argc >= 3)
		conn_max = atoi(tokens->argv[2]);
	else
		conn_max = default_count;
	if (conn_max <= 0)
		conn_max = default_count;
	acl_argv_free(tokens);
	return conn_max;
}

void connect_manager::init(const char* default_addr,
	const char* addr_list, int count)
{
	if (addr_list != NULL && *addr_list != 0)
		set_service_list(addr_list, count);

	// 创建缺省 redis 服务连接池对象，该对象一同放入总的连接池集群中
	if (default_addr != NULL && *default_addr != 0)
	{
		logger("default_pool: %s", default_addr);
		int max = check_addr(default_addr, default_addr_, count);
		default_pool_ = &set(default_addr_.c_str(), max);
	}
	else
		logger("no default redis set");

	// 必须保证至少有一个 redis 服务可用
	if (pools_.empty())
		logger_fatal("no redis service available!");
}

void connect_manager::set_service_list(const char* addr_list, int count)
{
	if (addr_list == NULL || *addr_list == 0)
	{
		logger("addr_list null");
		return;
	}

	// 创建 redis 连接池服务集群

	ACL_ARGV* tokens = acl_argv_split(addr_list, ";, \t");
	ACL_ITER iter;
	acl::string addr;
	acl_foreach(iter, tokens)
	{
		const char* ptr = (const char*) iter.data;
		int max = check_addr(ptr, addr, count);
		if (max <= 0)
		{
			logger_error("invalid redis addr: %s", addr.c_str());
			continue;
		}
		set(addr.c_str(), max);
		logger("add one service: %s, max connect: %d",
			addr.c_str(), max);
	}
	acl_argv_free(tokens);
}

connect_pool& connect_manager::set(const char* addr, int count)
{
	char key[64];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
			return **it;
	}

	connect_pool* pool = create_pool(key, count);
	pools_.push_back(pool);
	service_size_ = pools_.size();

	logger("Add one service, addr: %s, count: %d", addr, count);

	return *pool;
}

connect_pool* connect_manager::get(const char* addr)
{
	char key[64];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
			return *it;
	}

	logger_error("no connect pool for addr %s", addr);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

connect_pool* connect_manager::peek()
{
	connect_pool* pool;
	lock_.lock();
	size_t n = service_idx_ % service_size_;
	service_idx_++;
	lock_.unlock();
	pool = pools_[n];
	return pool;
}

void connect_manager::statistics_record(int, void* ctx)
{
	connect_manager* manager = (connect_manager*) ctx;

	// 记录当前 redis 访问情况
	manager->statistics();

	// 重新设置定时器
	manager->statistics_timer();
}

void connect_manager::statistics_settimer(int inter /* = 1 */)
{
	if (inter <= 0 || inter >= 3600 * 24 * 32)
	{
		logger_error("invalid inter: %d", inter);
		return;
	}

	stat_inter_ = inter;
#ifndef WIN32
	acl_ioctl_server_request_timer(statistics_record, this, inter);
#endif
	logger("set statistics_timer ok! inter: %d", inter);
}

void connect_manager::statistics_timer()
{
	statistics_settimer(stat_inter_);
}

void connect_manager::statistics()
{
	std::vector<connect_pool*>::const_iterator cit = pools_.begin();
	for (; cit != pools_.end(); ++cit)
	{
		logger("redis: %s, total: %llu, curr: %llu",
			(*cit)->get_addr(), (*cit)->get_total_used(),
			(*cit)->get_current_used());
		(*cit)->reset_statistics(stat_inter_);
	}
}

} // namespace acl
