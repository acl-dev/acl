#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <vector>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"
#include "acl_cpp/redis/redis_slot.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"
#endif

namespace acl
{

redis_client_cluster::redis_client_cluster(int max_slot /* = 16384 */)
: max_slot_(max_slot)
, redirect_max_(15)
, redirect_sleep_(100)
{
	slot_addrs_ = (const char**) acl_mycalloc(max_slot_, sizeof(char*));
}

redis_client_cluster::~redis_client_cluster()
{
	acl_myfree(slot_addrs_);

	std::vector<char*>::iterator it = addrs_.begin();
	for (; it != addrs_.end(); ++it)
		acl_myfree(*it);
}

void redis_client_cluster::set_redirect_max(int max)
{
	if (max > 0)
		redirect_max_ = max;
}

void redis_client_cluster::set_redirect_sleep(int n)
{
	redirect_sleep_ = n;
}

connect_pool* redis_client_cluster::create_pool(const char* addr,
	size_t count, size_t idx)
{
	redis_client_pool* pool = NEW redis_client_pool(addr, count, idx);

	string key(addr);
	key.lower();
	std::map<string, string>::const_iterator cit;
	if ((cit = passwds_.find(key)) != passwds_.end()
		|| (cit = passwds_.find("default")) != passwds_.end())
	{
		pool->set_password(cit->second.c_str());
	}

	return pool;
}

redis_client_pool* redis_client_cluster::peek_slot(int slot)
{
	if (slot < 0 || slot >= max_slot_)
		return NULL;

	// 需要加锁保护
	lock();

	if (slot_addrs_[slot] == NULL)
	{
		unlock();
		return NULL;
	}

	// 因为已经进行了加锁保护，所以在调用 get 方法时的第二个锁保护参数设为 false
	redis_client_pool* conns =
		(redis_client_pool*) get(slot_addrs_[slot], false);

	unlock();

	return conns;
}

void redis_client_cluster::clear_slot(int slot)
{
	if (slot >= 0 && slot < max_slot_)
	{
		lock();
		slot_addrs_[slot] = NULL;
		unlock();
	}
}

void redis_client_cluster::set_slot(int slot, const char* addr)
{
	if (slot < 0 || slot >= max_slot_ || addr == NULL || *addr == 0)
		return;

	// 遍历缓存的所有地址，若该地址不存在则直接添加，然后使之与 slot 进行关联

	// 该段代码需要加锁保护
	lock();

	std::vector<char*>::const_iterator cit = addrs_.begin();
	for (; cit != addrs_.end(); ++cit)
	{
		if (strcmp((*cit), addr) == 0)
			break;
	}

	// 将 slot 与地址进行关联映射
	if (cit != addrs_.end())
		slot_addrs_[slot] = *cit;
	else
	{
		// 只所以采用动态分配方式，是因为在往数组中添加对象时，无论
		// 数组如何做动态调整，该添加的动态内存地址都是固定的，所以
		// slot_addrs_ 的下标地址也是相对不变的
		char* buf = acl_mystrdup(addr);
		addrs_.push_back(buf);
		slot_addrs_[slot] = buf;
	}

	unlock();
}

void redis_client_cluster::set_all_slot(const char* addr, int max_conns)
{
	redis_client client(addr, 30, 60, false);
	redis_cluster cluster(&client);

	const std::vector<redis_slot*>* slots = cluster.cluster_slots();
	if (slots == NULL)
		return;

	std::vector<redis_slot*>::const_iterator cit;
	for (cit = slots->begin(); cit != slots->end(); ++cit)
	{
		const redis_slot* slot = *cit;
		const char* ip = slot->get_ip();
		if (*ip == 0)
			continue;
		int port = slot->get_port();
		if (port <= 0)
			continue;

		size_t slot_min = slot->get_slot_min();
		size_t slot_max = slot->get_slot_max();
		if ((int) slot_max >= max_slot_ || slot_max < slot_min)
			continue;
		
		char buf[128];
		safe_snprintf(buf, sizeof(buf), "%s:%d", ip, port);
		redis_client_pool* conns = (redis_client_pool*) get(buf);
		if (conns == NULL)
			set(buf, max_conns);

		for (size_t i = slot_min; i <= slot_max; i++)
			set_slot((int) i, buf);
	}
}

redis_client_cluster& redis_client_cluster::set_password(
	const char* addr, const char* pass)
{
	// 允许 pass 为空字符串且非空指针，这样就可以当 default 值被设置时，
	// 允许部分 redis 节点无需连接密码
	if (addr == NULL || *addr == 0 || pass == NULL || *pass == 0)
		return *this;

	string key(addr);
	key.lower();
	passwds_[key] = pass;

	for (std::vector<connect_pool*>::iterator it = pools_.begin();
		it != pools_.end(); ++it)
	{
		redis_client_pool* pool = (redis_client_pool*) (*it);
		key = pool->get_addr();
		key.lower();

		std::map<string, string>::const_iterator cit =
			passwds_.find(key);
		if (cit != passwds_.end() || !strcasecmp(addr, "default"))
			pool->set_password(pass);
	}

	return *this;
}

} // namespace acl
