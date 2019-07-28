#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

namespace acl {

class beanstalk;
class locker;

/**
 * beanstalk 客户端连接池，可以同时连接不同的 beanstalkd 服务器，
 * 每个 beanstalkd 有多个连接，内部自动加锁；但不控制连接数限制，
 * 用户应自行控制连接池的最大连接上限
 */
class ACL_CPP_API beanstalk_pool : public noncopyable
{
public:
	beanstalk_pool();
	~beanstalk_pool();

	/**
	 * 从连接池中取得一个 beanstalkd 的客户端连接
	 * @param addr {const char*} beanstalkd 服务地址(domain:port)
	 * @param clean_watch {bool} 在取得连接对象后是否自动取消所有的
	 *  已关注队列
	 * @param conn_timeout {int} 连接 beanstalkd 的超时时间
	 * @return {beanstalk*} 返回非空表示正常，否则表示出错
	 */
	beanstalk* peek(const char* addr, bool clean_watch = true,
		int conn_timeout = 60);

	/**
	 * 将不用的 beanstalkd 连接放回到连接池中
	 * @param client {beanstalk*} beanstalkd 客户端连接
	 * @param clean_watch {bool} 是否取消已经关注的队列
	 * @param keep {bool} 如果为 true 则将 client 放回至连接池，
	 *  否则释放该连接
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
