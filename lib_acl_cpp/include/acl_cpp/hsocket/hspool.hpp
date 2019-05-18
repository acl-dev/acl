#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

#ifndef ACL_CLIENT_ONLY

struct ACL_HTABLE;

namespace acl {

class hsclient;
class locker;

class ACL_CPP_API hspool : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param addr_rw {const char*} handlersocket 插件在 Mysql 上的监听地址，
	 * 格式为：ip:port，注：该地址是 handlersocket 的读写地址
	 * @param addr_rd {const char*} handlersocket 插件在 Mysql 上的监听地址，
	 * 格式为：ip:port，注：该地址是 handlersocket 的只读地址
	 * @param cache_enable {bool} 是否内部自动启用行对象缓存机制
	 * @param retry_enable {bool} 当因为网络原因而出错时是否需要进行重试
	 */
	hspool(const char* addr_rw, const char* addr_rd = NULL,
		bool cache_enable = true, bool retry_enable = true);

	~hspool();

	/**
	 * 从连接池中获得连接对象
	 * @param dbn {const char*} 数据库名称
	 * @param tbl {const char*} 数据库表名
	 * @param idx {const char*} 索引字段名
	 * @param flds {const char*} 要打开的数据字段名集合，格式为
	 *  由分隔符 ",; \t" 分隔的字段名称，如：user_id,user_name,user_mail
	 * @param readonly {bool} 是否仅以只读方式打开
	 */
	hsclient* peek(const char* dbn, const char* tbl,
		const char* idx, const char* flds, bool readonly = false);

	/**
	 * 归还连接对象
	 * @param client {hsclient*}
	 */
	void put(hsclient* client);
private:
	char* addr_rw_;
	char* addr_rd_;
	bool cache_enable_;
	bool retry_enable_;
	std::list<hsclient*> pool_;
	locker* locker_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
