#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

connect_manager::connect_manager()
: default_pool_(NULL)
, service_idx_(0)
, stat_inter_(1)
, retry_inter_(1)
, monitor_(NULL)
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

void connect_manager::set_retry_inter(int n)
{
	if (n == retry_inter_)
		return;

	lock_.lock();

	retry_inter_ = n;

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
		(*it)->set_retry_inter(retry_inter_);

	lock_.unlock();
}

void connect_manager::init(const char* default_addr,
	const char* addr_list, int count)
{
	if (addr_list != NULL && *addr_list != 0)
		set_service_list(addr_list, count);

	// 创建缺省服务连接池对象，该对象一同放入总的连接池集群中
	if (default_addr != NULL && *default_addr != 0)
	{
		logger("default_pool: %s", default_addr);
		int max = check_addr(default_addr, default_addr_, count);
		default_pool_ = &set(default_addr_.c_str(), max);
	}
	else
		logger("no default connection set");

	// 必须保证至少有一个服务可用
	if (pools_.empty())
		logger_fatal("no connection available!");
}

void connect_manager::set_service_list(const char* addr_list, int count)
{
	if (addr_list == NULL || *addr_list == 0)
	{
		logger("addr_list null");
		return;
	}

	// 创建连接池服务集群
	char* buf = acl_mystrdup(addr_list);
	char* addrs = acl_mystr_trim(buf);
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,");
	ACL_ITER iter;
	acl::string addr;
	acl_foreach(iter, tokens)
	{
		const char* ptr = (const char*) iter.data;
		int max = check_addr(ptr, addr, count);
		if (max <= 0)
		{
			logger_error("invalid server addr: %s", addr.c_str());
			continue;
		}
		set(addr.c_str(), max);
		logger("add one service: %s, max connect: %d",
			addr.c_str(), max);
	}
	acl_argv_free(tokens);
	acl_myfree(buf);
}

connect_pool& connect_manager::set(const char* addr, int count)
{
	char key[256];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	lock_.lock();

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
		{
			lock_.unlock();
			return **it;
		}
	}

	connect_pool* pool = create_pool(key, count, pools_.size() - 1);
	if (retry_inter_ > 0)
		pool->set_retry_inter(retry_inter_);
	pools_.push_back(pool);

	lock_.unlock();

	logger("Add one service, addr: %s, count: %d", addr, count);

	return *pool;
}

void connect_manager::remove(const char* addr)
{
	char key[256];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	lock_.lock();

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
		{
			(*it)->set_delay_destroy();
			pools_.erase(it);
			break;
		}
	}
	if (it == pools_.end())
		logger_warn("addr(%s) not found!", addr);

	lock_.unlock();
}


connect_pool* connect_manager::get(const char* addr,
	bool exclusive /* = true */)
{
	char key[256];
	ACL_SAFE_STRNCPY(key, addr, sizeof(key));
	acl_lowercase(key);

	if (exclusive)
		lock_.lock();

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
		{
			if (exclusive)
				lock_.unlock();
			return *it;
		}
	}

	if (exclusive)
		lock_.unlock();

	logger_error("no connect pool for addr %s", addr);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

connect_pool* connect_manager::peek()
{
	connect_pool* pool;
	size_t service_size, n;

	lock_.lock();
	service_size = pools_.size();
	if (service_size == 0)
	{
		lock_.unlock();
		logger_warn("pools's size is 0!");
		return NULL;
	}

	// 遍历所有的连接池，找出一个可用的连接池
	for(size_t i = 0; i < service_size; i++)
	{
		n = service_idx_ % service_size;
		service_idx_++;
		pool = pools_[n];
		if (pool->aliving())
		{
			lock_.unlock();
			return pool;
		}
	}

	lock_.unlock();

	logger_error("all pool(size=%d) is dead!", (int) service_size);
	return NULL;
}

connect_pool* connect_manager::peek(const char* key,
	bool exclusive /* = true */)
{
	if (key == NULL || *key == 0)
		return peek();

	size_t service_size;
	connect_pool* pool;
	unsigned n = acl_hash_crc32(key, strlen(key));

	if (exclusive)
		lock_.lock();
	service_size = pools_.size();
	if (service_size == 0)
	{
		if (exclusive)
			lock_.unlock();
		logger_warn("pools's size is 0!");
		return NULL;
	}
	pool = pools_[n % service_size];
	if (exclusive)
		lock_.unlock();

	return pool;
}

void connect_manager::lock()
{
	lock_.lock();
}

void connect_manager::unlock()
{
	lock_.unlock();
}

void connect_manager::statistics()
{
	std::vector<connect_pool*>::const_iterator cit = pools_.begin();
	for (; cit != pools_.end(); ++cit)
	{
		logger("server: %s, total: %llu, curr: %llu",
			(*cit)->get_addr(), (*cit)->get_total_used(),
			(*cit)->get_current_used());
		(*cit)->reset_statistics(stat_inter_);
	}
}

void connect_manager::start_monitor(int check_inter /* = 1 */,
	int conn_timeout /* = 10 */)
{
	if (monitor_ != NULL)
		return;

	monitor_ = NEW connect_monitor(*this, check_inter, conn_timeout);

	// 设置检测线程为非分离模式，以便于主线程可以等待检测线程退出
	monitor_->set_detachable(false);
	// 启动检测线程
	monitor_->start();
}

void connect_manager::stop_monitor(bool graceful /* = true */)
{
	if (monitor_)
	{
		// 先通知检测线程停止检测过程
		monitor_->stop(graceful);
		// 再等待检测线程退出
		monitor_->wait();
		// 删除检测线程对象
		delete monitor_;
		monitor_ = NULL;
	}
}

void connect_manager::set_pools_status(const char* addr, bool alive)
{
	std::vector<connect_pool*>::iterator it;
	connect_pool* pool;
	const char* ptr;

	if (addr == NULL || *addr == 0)
	{
		logger_warn("addr null");
		return;
	}

	lock();
	it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		pool = *it;
		ptr = pool->get_addr();
		if (ptr && strcasecmp(ptr, addr) == 0)
		{
			pool->set_alive(alive);
			break;
		}
	}
	unlock();
}

} // namespace acl
