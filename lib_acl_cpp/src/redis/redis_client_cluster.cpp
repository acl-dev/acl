#include "acl_stdafx.hpp"
#include <vector>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"
#include "acl_cpp/redis/redis_slot.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"

namespace acl
{

redis_client_cluster::redis_client_cluster(int conn_timeout /* = 30 */,
	int rw_timeout /* = 30 */, int max_slot /* = 16384 */)
: conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, max_slot_(max_slot)
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
	int count, size_t idx)
{
	redis_client_pool* pool = NEW redis_client_pool(addr, count, idx);
	pool->set_timeout(conn_timeout_, rw_timeout_);

	return pool;
}

redis_client_pool* redis_client_cluster::peek_slot(int slot)
{
	if (slot < 0 || slot >= max_slot_)
		return NULL;

	// ��Ҫ��������
	lock();

	if (slot_addrs_[slot] == NULL)
	{
		unlock();
		return NULL;
	}

	// ��Ϊ�Ѿ������˼��������������ڵ��� get ����ʱ�ĵڶ���������������Ϊ false
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

	// ������������е�ַ�����õ�ַ��������ֱ����ӣ�Ȼ��ʹ֮�� slot ���й���

	// �öδ�����Ҫ��������
	lock();

	std::vector<char*>::const_iterator cit = addrs_.begin();
	for (; cit != addrs_.end(); ++cit)
	{
		if (strcmp((*cit), addr) == 0)
			break;
	}

	// �� slot ���ַ���й���ӳ��
	if (cit != addrs_.end())
		slot_addrs_[slot] = *cit;
	else
	{
		// ֻ���Բ��ö�̬���䷽ʽ������Ϊ������������Ӷ���ʱ������
		// �����������̬����������ӵĶ�̬�ڴ��ַ���ǹ̶��ģ�����
		// slot_addrs_ ���±��ַҲ����Բ����
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

} // namespace acl
