#include <vector>
#include <string>
#include <assert.h>
#include "string_util.hpp"
#include "locker.hpp"
#include "connpool/connect_pool.hpp"
#include "connpool/connect_manager.hpp"

namespace acl_min
{

connect_manager::connect_manager()
: default_pool_(NULL)
, service_idx_(0)
, stat_inter_(1)
{
	lock_ = new locker();
}

connect_manager::~connect_manager()
{
	std::vector<connect_pool*>::iterator it = pools_.begin();

	// default_pool_ 已经包含在 pools_ 里了
	for (; it != pools_.end(); ++it)
		delete *it;

	delete lock_;
}

// 分析一个服务器地址，格式：IP:PORT[:MAX_CONN]
// 返回值 < 0 表示非法的地址
static int check_addr(const char* addr, std::string& buf, int default_count)
{
	buf.clear();

	// 数据格式：IP:PORT[:CONNECT_COUNT]
	std::vector<std::string> tokens;
	split3(addr, ":|", tokens);
	if (tokens.size() < 2)
		return -1;

	int port = atoi(tokens[1].c_str());
	if (port <= 0 || port >= 65535)
		return -1;

	buf = tokens[0].c_str();
	buf += ":";
	buf += port;

	int conn_max;
	if (tokens.size() >= 3)
		conn_max = atoi(tokens[2].c_str());
	else
		conn_max = default_count;
	if (conn_max <= 0)
		conn_max = default_count;
	return conn_max;
}

bool connect_manager::init(const char* default_addr,
	const char* addr_list, int count)
{
	if (addr_list != NULL && *addr_list != 0)
		set_service_list(addr_list, count);

	// 创建缺省服务连接池对象，该对象一同放入总的连接池集群中
	if (default_addr != NULL && *default_addr != 0)
	{
		int max = check_addr(default_addr, default_addr_, count);
		default_pool_ = &set(default_addr_.c_str(), max);
	}

	// 必须保证至少有一个服务可用
	return pools_.empty() ? false : true;
}

void connect_manager::set_service_list(const char* addr_list, int count)
{
	if (addr_list == NULL || *addr_list == 0)
		return;

	// 创建连接池服务集群

	std::vector<std::string> tokens;
	char* buf = strdup(addr_list);
	char* addrs = strtrim(buf);
	split3(addrs, ";,", tokens);
	std::string addr;

	std::vector<std::string>::const_iterator cit = tokens.begin();
	for (; cit != tokens.end(); ++cit)
	{
		const char* ptr = (*cit).c_str();
		int max = check_addr(ptr, addr, count);
		if (max > 0)
			set(addr.c_str(), max);
	}
	free(buf);
}

connect_pool& connect_manager::set(const char* addr, int count)
{
	char key[256];
	lowercase(addr, key, sizeof(key));

	lock_->lock();

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
		{
			lock_->unlock();
			return **it;
		}
	}

	connect_pool* pool = create_pool(key, count, pools_.size() - 1);
	pools_.push_back(pool);

	lock_->unlock();

	return *pool;
}

void connect_manager::remove(const char* addr)
{
	char key[256];
	lowercase(addr, key, sizeof(key));

	lock_->lock();

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

	lock_->unlock();
}

connect_pool* connect_manager::get(const char* addr,
	bool exclusive /* = true */)
{
	char  key[256];
	lowercase(addr, key, sizeof(key));

	if (exclusive)
		lock_->lock();

	std::vector<connect_pool*>::iterator it = pools_.begin();
	for (; it != pools_.end(); ++it)
	{
		if (strcasecmp(key, (*it)->get_addr()) == 0)
		{
			if (exclusive)
				lock_->unlock();
			return *it;
		}
	}

	if (exclusive)
		lock_->unlock();

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

connect_pool* connect_manager::peek()
{
	connect_pool* pool;
	size_t service_size, n;

	lock_->lock();
	service_size = pools_.size();
	if (service_size == 0)
	{
		lock_->unlock();
		return NULL;
	}
	n = service_idx_ % service_size;
	service_idx_++;
	lock_->unlock();
	pool = pools_[n];
	return pool;
}

static unsigned hash(const void *buffer, size_t len)
{
        unsigned long h = 0;
        unsigned long g;
	const unsigned char* s = (const unsigned char *) buffer;

        /*
         * From the "Dragon" book by Aho, Sethi and Ullman.
         */

        while (len-- > 0) {
                h = (h << 4) + *s++;
                if ((g = (h & 0xf0000000)) != 0) {
                        h ^= (g >> 24);
                        h ^= g;
                }
        }

        return h;
}

connect_pool* connect_manager::peek(const char* key,
	bool exclusive /* = true */)
{
	if (key == NULL || *key == 0)
		return peek();

	size_t service_size;
	connect_pool* pool;
	unsigned n = hash(key, strlen(key));

	if (exclusive)
		lock_->lock();
	service_size = pools_.size();
	if (service_size == 0)
	{
		if (exclusive)
			lock_->unlock();
		return NULL;
	}
	pool = pools_[n % service_size];
	if (exclusive)
		lock_->unlock();

	return pool;
}

void connect_manager::lock()
{
	lock_->lock();
}

void connect_manager::unlock()
{
	lock_->unlock();
}

void connect_manager::statistics()
{
	std::vector<connect_pool*>::const_iterator cit = pools_.begin();
	for (; cit != pools_.end(); ++cit)
		(*cit)->reset_statistics(stat_inter_);
}

} // namespace acl
