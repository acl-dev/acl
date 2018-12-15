#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"
#endif

namespace acl
{

connect_manager::connect_manager()
: thread_binding_(false)
, default_pool_(NULL)
, service_idx_(0)
, stat_inter_(1)
, retry_inter_(1)
, idle_ttl_(-1)
, check_inter_(-1)
, check_pos_(0)
, monitor_(NULL)
{
}

connect_manager::~connect_manager()
{
	lock_.lock();

	manager_it mit = manager_.begin();
	for (; mit != manager_.end(); ++mit) {
		for (pools_t::iterator it = mit->second->begin();
			it != mit->second->end(); ++it) {

			delete *it;
		}
		delete mit->second;
	}

	lock_.unlock();
}

void connect_manager::bind_thread(bool yes)
{
	thread_binding_ = yes;
}

// 分析一个服务器地址，格式：IP:PORT[:MAX_CONN]
// 返回值 < 0 表示非法的地址
static int check_addr(const char* addr, string& buf, size_t default_count)
{
	// 数据格式：IP:PORT[:CONNECT_COUNT]
	ACL_ARGV* tokens = acl_argv_split(addr, ":|");
	if (tokens->argc < 2) {
		logger_error("invalid addr: %s", addr);
		acl_argv_free(tokens);
		return -1;
	}

	int port = atoi(tokens->argv[1]);
	if (port <= 0 || port >= 65535) {
		logger_error("invalid addr: %s, port: %d", addr, port);
		acl_argv_free(tokens);
		return -1;
	}
	buf.format("%s:%d", tokens->argv[0], port);
	int conn_max;
	if (tokens->argc >= 3) {
		conn_max = atoi(tokens->argv[2]);
	} else {
		conn_max = (int) default_count;
	}
	if (conn_max < 0) {
		conn_max = (int) default_count;
	}
	acl_argv_free(tokens);
	return conn_max;
}

void connect_manager::set_retry_inter(int n)
{
	if (n == retry_inter_) {
		return;
	}

	lock_guard guard(lock_);

	retry_inter_ = n;

	manager_it mit = manager_.begin();
	for (; mit != manager_.end(); ++mit) {
		for (pools_t::iterator it = mit->second->begin();
			it != mit->second->end(); ++it) {

			(*it)->set_retry_inter(retry_inter_);
		}
	}
}

void connect_manager::set_check_inter(int n)
{
	check_inter_ = n;
}

void connect_manager::set_idle_ttl(time_t ttl)
{
	idle_ttl_ = ttl;
}

void connect_manager::init(const char* default_addr, const char* addr_list,
	size_t count, int conn_timeout /* = 30 */, int rw_timeout /* = 30 */)
{
	if (addr_list != NULL && *addr_list != 0) {
		set_service_list(addr_list, (int) count,
			conn_timeout, rw_timeout);
	}

	// 创建缺省服务连接池对象，该对象一同放入总的连接池集群中
	if (default_addr != NULL && *default_addr != 0) {
		logger("default_pool: %s", default_addr);
		int max = check_addr(default_addr, default_addr_, count);
		if (max < 0) {
			logger("no default connection set");
		} else {
			set(default_addr_.c_str(), max, conn_timeout, rw_timeout);
			default_pool_ = get(default_addr_);
		}
	} else {
		logger("no default connection set");
	}
}

void connect_manager::set_service_list(const char* addr_list, int count,
	int conn_timeout, int rw_timeout)
{
	if (addr_list == NULL || *addr_list == 0) {
		logger("addr_list null");
		return;
	}

	// 创建连接池服务集群
	char* buf = acl_mystrdup(addr_list);
	char* addrs = acl_mystr_trim(buf);
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,");
	ACL_ITER iter;
	acl::string addr;
	acl_foreach(iter, tokens) {
		const char* ptr = (const char*) iter.data;
		int max = check_addr(ptr, addr, count);
		if (max < 0) {
			logger_error("invalid server addr: %s", addr.c_str());
			continue;
		}
		set(addr.c_str(), max, conn_timeout, rw_timeout);
		logger("add one service: %s, max connect: %d",
			addr.c_str(), max);
	}
	acl_argv_free(tokens);
	acl_myfree(buf);
}

std::vector<connect_pool*>& connect_manager::get_pools(void)
{
	unsigned long id = get_id();
	pools_t* pools = get_pools_by_id(id);
	return *pools;
}

size_t connect_manager::size(void) const
{
	size_t n = 0;
	for (manager_cit cit = manager_.begin(); cit != manager_.end(); ++cit) {
		n += cit->second->size();
	}
	return n;
}

void connect_manager::set(const char* addr, size_t count,
	int conn_timeout /* = 30 */, int rw_timeout /* = 30 */)
{
	string buf(addr);
	buf.lower();

	lock_guard guard(lock_);
	std::map<string, connect_config>::const_iterator cit = addrs_.find(buf);
	if (cit == addrs_.end()) {
		connect_config config;
		config.addr         = addr;
		config.count        = count;
		config.conn_timeout = conn_timeout;
		config.rw_timeout   = rw_timeout;
		addrs_[buf]         = config;
	}
}

#define DEFAULT_ID	0

unsigned long connect_manager::get_id(void) const
{
	if (thread_binding_) {
		return thread::self();
	} else {
		return DEFAULT_ID;
	}
}

std::vector<connect_pool*>* connect_manager::get_pools_by_id(unsigned long id)
{
	manager_it mit = manager_.find(id);
	if (mit == manager_.end()) {
		pools_t* pools = new pools_t;
		manager_[id] = pools;
		return pools;
	} else {
		return mit->second;
	}
}

void connect_manager::remove(pools_t& pools, const char* addr)
{
	string buf;
	for (pools_t::iterator it = pools.begin(); it != pools.end(); ++it) {
		get_addr((*it)->get_key(), buf);
		if (buf == addr) {
			(*it)->set_delay_destroy();
			pools.erase(it);
			break;
		}
	}
}

void connect_manager::remove(const char* addr)
{
	string buf(addr);
	buf.lower();

	lock_guard guard(lock_);

	for (manager_it it = manager_.begin(); it != manager_.end(); ++it) {
		remove(*it->second, buf);
	}
}

connect_pool* connect_manager::add_pool(const connect_config& cf)
{
	string key;
	get_key(cf.addr, key);

	unsigned long id = get_id();
	pools_t* pools = get_pools_by_id(id);

	connect_pool* pool = create_pool(cf.addr, cf.count, pools->size());
	pool->set_key(key);
	pool->set_retry_inter(retry_inter_);
	pool->set_timeout(cf.conn_timeout, cf.rw_timeout);
	if (idle_ttl_ >= 0) {
		pool->set_idle_ttl(idle_ttl_);
	}
	if (check_inter_ > 0) {
		pool->set_check_inter(check_inter_);
	}
	pools->push_back(pool);

	logger("Add one service, addr: %s, count: %d",
		cf.addr.c_str(), (int) cf.count);
	return pool;
}

connect_pool* connect_manager::get(const char* addr,
	bool exclusive /* = true */, bool restore /* = false */)
{
	string key, buf(addr);
	get_key(addr, key);
	buf.lower();
	unsigned long id = get_id();

	if (exclusive) {
		lock_.lock();
	}

	pools_t* pools = get_pools_by_id(id);

	for (pools_t::iterator it = pools->begin(); it != pools->end(); ++it) {
		if (key == (*it)->get_key()) {
			if (restore && (*it)->aliving() == false) {
				(*it)->set_alive(true);
			}
			if (exclusive) {
				lock_.unlock();
			}
			return *it;
		}
	}

	std::map<string, connect_config>::const_iterator cit = addrs_.find(buf);
	if (cit == addrs_.end()) {
		if (exclusive) {
			lock_.unlock();
		}
		logger("no connect pool for addr %s", addr);
		return NULL;
	}

	connect_pool* pool = add_pool(cit->second);

	if (exclusive) {
		lock_.unlock();
	}

	return pool;
}

//////////////////////////////////////////////////////////////////////////

size_t connect_manager::check_idle(size_t step, size_t* left /* = NULL */)
{
	if (step == 0) {
		step = 1;
	}

	size_t nleft = 0, nfreed = 0, pools_size, check_max;
	unsigned long id = get_id();

	lock();

	pools_t* pools = get_pools_by_id(id);
	pools_size = pools->size();
	check_max = check_pos_ + step;

	while (check_pos_ < pools_size && check_pos_ < check_max) {
		connect_pool* pool = (*pools)[check_pos_++];
		int ret = pool->check_idle(idle_ttl_);
		if (ret > 0) {
			nfreed += ret;
		}
		nleft += pool->get_count();
	}

	unlock();

	if (left) {
		*left = nleft;
	}
	return nfreed;
}

connect_pool* connect_manager::peek(void)
{
	connect_pool* pool;
	size_t service_size, n;

	unsigned long id = get_id();
	lock_.lock();

	pools_t* pools = get_pools_by_id(id);
	service_size = pools->size();
	if (service_size == 0) {
		lock_.unlock();
		logger_warn("pools's size is 0!");
		return NULL;
	}

	// 遍历所有的连接池，找出一个可用的连接池
	for(size_t i = 0; i < service_size; i++) {
		n = service_idx_ % service_size;
		service_idx_++;
		pool = (*pools)[n];
		if (pool->aliving()) {
			lock_.unlock();
			return pool;
		}
	}

	lock_.unlock();

	logger_error("all pool(size=%d) is dead!", (int) service_size);
	return NULL;
}

connect_pool* connect_manager::peek(const char* addr,
	bool exclusive /* = true */)
{
	if (addr == NULL || *addr == 0 || thread_binding_) {
		return peek();
	}

	unsigned long id = get_id();
	connect_pool* pool;

	size_t service_size;
	unsigned n = acl_hash_crc32(addr, strlen(addr));

	if (exclusive) {
		lock_.lock();
	}

	pools_t* pools = get_pools_by_id(id);

	service_size = pools->size();
	if (service_size == 0) {
		if (exclusive) {
			lock_.unlock();
		}
		logger_warn("pools's size is 0!");
		return NULL;
	}

	n = n % service_size;
	pool = (*pools)[n];

	if (exclusive) {
		lock_.unlock();
	}

	return pool;
}

void connect_manager::lock(void)
{
	lock_.lock();
}

void connect_manager::unlock(void)
{
	lock_.unlock();
}

void connect_manager::statistics(void)
{
	unsigned long id = get_id();
	pools_t* pools = get_pools_by_id(id);

	std::vector<connect_pool*>::const_iterator cit = pools->begin();
	for (; cit != pools->end(); ++cit) {
		logger("server: %s, total: %llu, curr: %llu",
			(*cit)->get_key(), (*cit)->get_total_used(),
			(*cit)->get_current_used());
		(*cit)->reset_statistics(stat_inter_);
	}
}

bool connect_manager::start_monitor(connect_monitor* monitor)
{
	if (monitor_ != NULL) {
		logger_warn("one connect_monitor running!");
		return false;
	}

	monitor_ = monitor;

	// 设置检测线程为非分离模式，以便于主线程可以等待检测线程退出
	monitor_->set_detachable(false);
	// 启动检测线程
	monitor_->start();

	return true;
}

connect_monitor* connect_manager::stop_monitor(bool graceful /* = true */)
{
	connect_monitor* monitor = monitor_;

	if (monitor) {
		// 先将连接检测对象置 NULL
		monitor_ = NULL;

		// 先通知检测线程停止检测过程
		monitor->stop(graceful);

		// 再等待检测线程退出
		monitor->wait();
	}

	return monitor;
}

void connect_manager::set_pools_status(const char* addr, bool alive)
{
	if (addr == NULL || *addr == 0) {
		logger_warn("addr null");
		return;
	}

	std::vector<connect_pool*>::iterator it;

	lock_guard guard(lock_);

	for (manager_it mit = manager_.begin(); mit != manager_.end(); ++mit) {
		set_status(*mit->second, addr, alive);
	}
}

void connect_manager::set_status(pools_t& pools, const char* addr, bool alive)
{
	string buf(addr);
	buf.lower();

	for (pools_t::iterator it = pools.begin(); it != pools.end(); ++it) {
		if (buf == (*it)->get_key()) {
			(*it)->set_alive(alive);
			break;
		}
	}
}

void connect_manager::get_key(const char* addr, string& key)
{
	if (thread_binding_) {
		key.format("%lu|%s", thread::self(), addr);
	} else {
		key = addr;
	}
	key.lower();
}

void connect_manager::get_addr(const char* key, string& addr)
{
	if (thread_binding_) {
		const char* sep = strchr(key, '|');
		if (sep == NULL || *++sep == 0) {
			logger_fatal("invalid key=%s", key);
		}
		addr = sep;
	} else {
		addr = key;
	}
}

} // namespace acl
