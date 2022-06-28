#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/hsocket/hsclient.hpp"
#include "acl_cpp/hsocket/hspool.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

hspool::hspool(const char* addr_rw, const char* addr_rd,
	bool cache_enable /* = true */, bool retry_enable /* = true */)
: cache_enable_(cache_enable)
, retry_enable_(retry_enable)
{
	acl_assert(addr_rw);
	addr_rw_ = acl_mystrdup(addr_rw);
	if (addr_rd != NULL) {
		addr_rd_ = acl_mystrdup(addr_rd);
	} else {
		addr_rd_ = addr_rw_;
	}
	locker_ = NEW locker(true);
}

hspool::~hspool(void)
{
	if (addr_rd_ != addr_rw_) {
		acl_myfree(addr_rd_);
	}
	acl_myfree(addr_rw_);

	std::list<hsclient*>::iterator it = pool_.begin();
	for (; it != pool_.end(); ++it) {
		delete *it;
	}
	delete locker_;
}

hsclient* hspool::peek(const char* dbn, const char* tbl,
	const char* idx, const char* flds, bool readonly /* = false */)
{
	hsclient* client;
	const char* addr;

	if (readonly) {
		addr = addr_rd_;
	} else {
		addr = addr_rw_;
	}

	locker_->lock();

	// 先顺序查询符合表字段条件的连接对象
	std::list<hsclient*>::iterator it = pool_.begin();
	for (; it != pool_.end(); ++it) {
		// 如果地址不匹配查跳过，地址必须匹配
		if (strcmp((*it)->get_addr(), addr) != 0) {
			continue;
		}

		// 打开已经打开的表，查询表字段是否符合
		if ((*it)->open_tbl(dbn, tbl, idx, flds, false)) {
			client = *it;
			pool_.erase(it);
			locker_->unlock();
			return client;
		}
	}

	// 查询地址匹配的连接对象，如果存在一个匹配的连接对象，则
	// 打开新的表
	for (it = pool_.begin(); it != pool_.end();) {
		// 如果地址不匹配查跳过，地址必须匹配
		if (strcmp((*it)->get_addr(), addr) != 0) {
			++it;
			continue;
		}

		client = *it;
		it = pool_.erase(it); // 从连接池中删除

		// 打开新的表
		if (client->open_tbl(dbn, tbl, idx, flds, true)) {
			locker_->unlock();
			return client;
		}

		// 打开失败，则需要删除该连接对象
		delete client;
	}

	locker_->unlock();

	client = NEW hsclient(addr, cache_enable_, retry_enable_);

	if (!client->open_tbl(dbn, tbl, idx, flds)) {
		delete client;
		return NULL;
	}

	return client;
}

void hspool::put(hsclient* client)
{
	acl_assert(client);
	locker_->lock();
	pool_.push_back(client);
	locker_->unlock();
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
