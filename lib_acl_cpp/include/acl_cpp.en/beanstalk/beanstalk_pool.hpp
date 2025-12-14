#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

namespace acl {

class beanstalk;
class locker;

/**
 * Beanstalk client connection pool, can connect to different beanstalkd servers simultaneously.
 * Each beanstalkd has multiple connections, internally automatically locked; but does not control connection limit,
 * users should control the maximum connection limit of the connection pool themselves
 */
class ACL_CPP_API beanstalk_pool : public noncopyable
{
public:
	beanstalk_pool();
	~beanstalk_pool();

	/**
	 * Get a beanstalkd client connection from the connection pool
	 * @param addr {const char*} Beanstalkd server address (domain:port)
	 * @param clean_watch {bool} Whether to automatically cancel all
	 *  watched queues after getting the connection object
	 * @param conn_timeout {int} Timeout for connecting to beanstalkd
	 * @return {beanstalk*} Returns non-NULL on success, otherwise indicates error
	 */
	beanstalk* peek(const char* addr, bool clean_watch = true,
		int conn_timeout = 60);

	/**
	 * Put unused beanstalkd connection back into the connection pool
	 * @param client {beanstalk*} Beanstalkd client connection
	 * @param clean_watch {bool} Whether to cancel already watched queues
	 * @param keep {bool} If true, put client back into connection pool,
	 *  otherwise release the connection
	 */
	void put(beanstalk* client, bool clean_watch = true,
		bool keep = true);
private:
	locker* lock_;
	typedef std::multimap<string, beanstalk*> bspool;
	typedef bspool::const_iterator pool_cit;
	typedef bspool::iterator pool_it;
	typedef std::pair<pool_it, pool_it> pool_range;

	bspool pool_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

