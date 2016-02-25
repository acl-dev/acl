#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/beanstalk/beanstalk.hpp"
#include "acl_cpp/beanstalk/beanstalk_pool.hpp"
#endif

namespace acl {

beanstalk_pool::beanstalk_pool()
{
	lock_ = NEW locker();
}

beanstalk_pool::~beanstalk_pool()
{
	pool_it it = pool_.begin();
	for (; it != pool_.end(); ++it)
		delete it->second;
	delete lock_;
}

beanstalk* beanstalk_pool::peek(const char* addr, bool clean_watch /* = true */,
	int conn_timeout /* = 60 */)
{
	char* key = acl_mystrdup(addr);
	acl_lowercase(key);

	lock_->lock();
	pool_range r = pool_.equal_range(key);
	for (pool_it it = r.first; it != r.second; ++it)
	{
		beanstalk* client = it->second;
		pool_.erase(it);
		lock_->unlock();

		acl_myfree(key);
		if (clean_watch)
			client->ignore_all();
		return client;
	}

	lock_->unlock();
	acl_myfree(key);
	return NEW beanstalk(addr, conn_timeout);
}

void beanstalk_pool::put(beanstalk* client, bool clean_watch /* = true */,
	bool keep /* = true */)
{
	if (keep == false)
	{
		delete client;
		return;
	}
	if (clean_watch)
		client->ignore_all();

	lock_->lock();
	pool_.insert(std::make_pair(client->get_addr(), client));
	lock_->unlock();
}

} // namespace acl
