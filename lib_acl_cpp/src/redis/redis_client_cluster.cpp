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
#include "redis_request.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_client_cluster::redis_client_cluster(int max_slot /* = 16384 */)
: max_slot_(max_slot)
, redirect_max_(15)
, redirect_sleep_(100)
, ssl_conf_(NULL)
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

	if (ssl_conf_)
		pool->set_ssl_conf(ssl_conf_);

	string key(addr);
	key.lower();
	std::map<string, string>::const_iterator cit;
	if ((cit = passwds_.find(key)) != passwds_.end()
		|| (cit = passwds_.find("default")) != passwds_.end()) {

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

	if (slot_addrs_[slot] == NULL) {
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
	if (slot >= 0 && slot < max_slot_) {
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
	for (; cit != addrs_.end(); ++cit) {
		if (strcmp((*cit), addr) == 0)
			break;
	}

	// 将 slot 与地址进行关联映射
	if (cit != addrs_.end())
		slot_addrs_[slot] = *cit;
	else {
		// 只所以采用动态分配方式，是因为在往数组中添加对象时，无论
		// 数组如何做动态调整，该添加的动态内存地址都是固定的，所以
		// slot_addrs_ 的下标地址也是相对不变的
		char* buf = acl_mystrdup(addr);
		addrs_.push_back(buf);
		slot_addrs_[slot] = buf;
	}

	unlock();
}

void redis_client_cluster::set_all_slot(const char* addr, size_t max_conns,
	int conn_timeout, int rw_timeout)
{
	redis_client client(addr, 30, 60, false);

	string key(addr);
	key.lower();
	std::map<string, string>::const_iterator cit0;
	if ((cit0 = passwds_.find(key)) != passwds_.end()
		|| (cit0 = passwds_.find("default")) != passwds_.end()) {

		client.set_password(cit0->second.c_str());
	}

	redis_cluster cluster(&client);

	const std::vector<redis_slot*>* slots = cluster.cluster_slots();
	if (slots == NULL)
		return;

	std::vector<redis_slot*>::const_iterator cit;
	for (cit = slots->begin(); cit != slots->end(); ++cit) {
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
			set(buf, max_conns, conn_timeout, rw_timeout);

		for (size_t i = slot_min; i <= slot_max; i++)
			set_slot((int) i, buf);
	}
}

redis_client_cluster& redis_client_cluster::set_ssl_conf(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

redis_client_cluster& redis_client_cluster::set_password(
	const char* addr, const char* pass)
{
	// 允许 pass 为空字符串且非空指针，这样就可以当 default 值被设置时，
	// 允许部分 redis 节点无需连接密码
	if (addr == NULL || *addr == 0 || pass == NULL || *pass == 0)
		return *this;

	lock_guard guard(lock_);

	string key(addr);
	key.lower();
	passwds_[key] = pass;

	unsigned long id = get_id();
	conns_pools& pools = get_pools_by_id(id);
	for (std::vector<connect_pool*>::iterator it = pools.pools.begin();
		it != pools.pools.end(); ++it) {

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

const char* redis_client_cluster::get_password(const char* addr) const
{
	if (addr == NULL || *addr == 0)
		return NULL;

	lock_guard guard((const_cast<redis_client_cluster*>(this))->lock_);

	std::map<string, string>::const_iterator cit = passwds_.find(addr);
	if (cit != passwds_.end())
		return cit->second.c_str();

	cit = passwds_.find("default");
	if (cit != passwds_.end())
		return cit->second.c_str();
	return NULL;
}

// 根据输入的目标地址进行重定向：打开与该地址的连接，如果连接失败，则随机
// 选取一个服务器地址进行连接
redis_client* redis_client_cluster::redirect(const char* addr, size_t max_conns)
{
	redis_client_pool* conns;

	// 如果服务器地址不存在，则根据服务器地址动态创建连接池对象
	if ((conns = (redis_client_pool*) this->get(addr)) == NULL) {
		this->set(addr, max_conns);
		conns = (redis_client_pool*) this->get(addr);
	}

	if (conns == NULL) {
		return NULL;
	}

	redis_client* conn;

	int i = 0;

	while (i++ < 5) {
		conn = (redis_client*) conns->peek();
		if (conn != NULL) {
			return conn;
		}

#ifdef AUTO_SET_ALIVE
		conns->set_alive(false);
#endif
		conns = (redis_client_pool*) this->peek();
		if (conns == NULL) {
			logger_error("no connections availabble, "
				"i: %d, addr: %s", i, addr);
			return NULL;
		}
	}

	logger_warn("too many retry: %d, addr: %s", i, addr);
	return NULL;
}

redis_client* redis_client_cluster::peek_conn(int slot)
{
	// 如果已经计算了哈希槽值，则优先从本地缓存中查找对应的连接池
	// 如果未找到，则从所有集群结点中随便找一个可用的连接池对象

	redis_client_pool* conns;
	redis_client* conn;
	int i = 0;

	while (i++ < 5) {
		if (slot < 0)
			conns = (redis_client_pool*) this->peek();
		else if ((conns = peek_slot(slot)) == NULL)
			conns = (redis_client_pool*) this->peek();

		if (conns == NULL) {
			slot = -1;
			continue;
		}

		conn = (redis_client*) conns->peek();
		if (conn != NULL)
			return conn;

		// 取消哈希槽的地址映射关系
		clear_slot(slot);

#ifdef AUTO_SET_ALIVE
		// 将连接池对象置为不可用状态
		conns->set_alive(false);
#endif
	}

	logger_warn("too many retry: %d, slot: %d", i, slot);
	return NULL;
}

redis_client* redis_client_cluster::reopen(redis_command& cmd,
	redis_client* conn)
{
	connect_pool* pool = conn->get_pool();
	int slot = cmd.get_slot();

	// 删除哈希槽中的地址映射关系以便下次操作时重新获取
	clear_slot(slot);

	// 将连接对象归还给连接池对象
	pool->put(conn, false);

	redis_request* obj = cmd.get_request_obj();
	string* buf = cmd.get_request_buf();
	// 如果连接断开且请求数据为空时，则无须重试
	if ((obj == NULL || !obj->get_size()) && buf->empty()) {
		logger_error("not retry when no request!");
		return NULL;
	}

#ifdef AUTO_SET_ALIVE
	// 将连接池对象置为不可用状态
	pool->set_alive(false);
#endif
	// 从连接池集群中顺序取得一个连接对象
	conn = peek_conn(slot);
	if (conn == NULL) {
		logger_error("peek_conn NULL");
	} else {
		cmd.clear(true);
		cmd.set_client_addr(*conn);
	}
	return conn;
}

redis_client* redis_client_cluster::move(redis_command& cmd,
	redis_client* conn, const char* ptr, int ntried)
{
	// 将旧连接对象归还给连接池对象
	conn->get_pool()->put(conn, true);

	const char* addr = cmd.get_addr(ptr);
	if (addr == NULL) {
		logger_warn("MOVED invalid, ptr: %s", ptr);
		return NULL;
	}

	// 从连接池集合中提取目标redis节点的配置项，如果不存在，则默认使用
	// 所设置的第一个redis节点的配置项
	const conn_config* conf = this->get_config(addr, true);
	if (conf == NULL) {
		logger_error("no conn_config for addr=%s", addr);
		return NULL;
	}

	conn = redirect(addr, conf->count);
	if (conn == NULL) {
		logger_error("redirect NULL, addr: %s", addr);
		return NULL;
	}

	ptr = conn->get_pool()->get_addr();
	cmd.set_client_addr(ptr);

	// 需要保存哈希槽值
	cmd.clear(true);

	if (ntried >= 2 && redirect_sleep_ > 0 && strcmp(ptr, addr) != 0) {
		logger("redirect %d, curr %s, waiting %s ...",
			ntried, ptr, addr);
		acl_doze(redirect_sleep_);
	}

	return conn;
}

redis_client* redis_client_cluster::ask(redis_command& cmd,
	redis_client* conn, const char* ptr, int ntried)
{
	// 将旧连接对象归还给连接池对象
	conn->get_pool()->put(conn, true);

	const char* addr = cmd.get_addr(ptr);
	if (addr == NULL) {
		logger_warn("ASK invalid, ptr: %s", ptr);
		return NULL;
	}

	// 从连接池集合中提取目标redis节点的配置项，如果不存在，则默认使用
	// 所设置的第一个redis节点的配置项
	const conn_config* conf = this->get_config(addr, true);
	if (conf == NULL) {
		logger_error("no conn_config for addr=%s", addr);
		return NULL;
	}

	conn = redirect(addr, conf->count);
	if (conn == NULL) {
		logger_error("redirect NULL, addr: %s", addr);
		return NULL;
	}

	ptr = conn->get_pool()->get_addr();
	cmd.set_client_addr(ptr);

	if (ntried >= 2 && redirect_sleep_ > 0 && strcmp(ptr, addr) != 0) {
		logger("redirect %d, curr %s, waiting %s ...",
			ntried, ptr, addr);
		acl_doze(redirect_sleep_);
	}

	dbuf_pool* dbuf = cmd.get_dbuf();
	const redis_result* result = conn->run(dbuf, "ASKING\r\n", 0);
	if (result == NULL) {
		logger_error("ASKING's reply null");
		conn->get_pool()->put(conn, !conn->eof());
		return NULL;
	}

	const char* status = result->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0) {
		logger_error("ASKING's reply error: %s",
			status ? status : "null");
		conn->get_pool()->put(conn, !conn->eof());
		return NULL;
	}

	cmd.clear(true);
	return conn;
}

redis_client* redis_client_cluster::cluster_down(redis_command& cmd,
	redis_client* conn, const char* ptr, int ntried)
{
	clear_slot(cmd.get_slot());

	if (redirect_sleep_ > 0) {
		logger("%s: redirect %d, slot %d, waiting %s ...",
			conn->get_pool()->get_addr(), ntried,
			cmd.get_slot(), ptr);
		acl_doze(redirect_sleep_);
	}

	// 将旧连接对象归还给连接池对象，并设置该连接池为不可用状态
	connect_pool* conns = conn->get_pool();
	conns->put(conn, false);
	conns->set_alive(false);

	conn = peek_conn(-1);
	if (conn == NULL) {
		logger_error("peek_conn NULL");
		return NULL;
	}

	cmd.clear(true);
	cmd.set_client_addr(*conn);
	return conn;
}

const redis_result* redis_client_cluster::run(redis_command& cmd,
	size_t nchild, int* timeout /* = NULL */)
{
	redis_client* conn = peek_conn(cmd.get_slot());

	// 如果没有找到可用的连接对象，则直接返回 NULL 表示出错
	if (conn == NULL) {
		logger_error("peek_conn NULL, slot_: %d", cmd.get_slot());
		return NULL;
	}

	cmd.set_client_addr(*conn);
	conn->set_check_addr(cmd.is_check_addr());

	redis_result_t type;
	bool  last_moved = false;
	int   n = 0;

	dbuf_pool* dbuf    = cmd.get_dbuf();
	redis_request* obj = cmd.get_request_obj();
	string* buf        = cmd.get_request_buf();

	const redis_result* result;

	while (n++ < redirect_max_) {
		// 根据请求过程是否采用内存分片方式调用不同的请求过程
		if (cmd.is_slice_req()) {
			result = conn->run(dbuf, *obj, nchild, timeout);
		} else {
			result = conn->run(dbuf, *buf, nchild, timeout);
		}

		// 如果连接异常断开，则需要进行重试
		if (conn->eof()) {
			conn = reopen(cmd, conn);
			if (conn == NULL) {
				return result;
			}
			last_moved = true;
			continue;
		}

		if (result == NULL) {
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("result NULL");
			return NULL;
		}

		// 取得服务器的响应结果的类型，并进行分别处理
		type = result->get_type();

		if (type == REDIS_RESULT_UNKOWN) {
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("unknown result type: %d", type);
			return NULL;
		}

		if (type != REDIS_RESULT_ERROR) {
			// 如果发生重定向过程，则设置哈希槽对应 redis 服务地址
			if (cmd.get_slot() < 0 || !last_moved) {
				// 将连接对象归还给连接池对象
				conn->get_pool()->put(conn, true);
				return result;
			}

			// XXX: 因为此处还要引用一次 conn 对象，所以将 conn
			// 归还给连接池的过程须放在此段代码之后
			const char* addr = conn->get_pool()->get_addr();
			set_slot(cmd.get_slot(), addr);

			// 将连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			return result;
		}

#define	EQ(x, y) !strncasecmp((x), (y), sizeof(y) -1)

		// 对于结果类型为错误类型，则需要进一步判断是否是重定向指令
		const char* ptr = result->get_error();
		if (ptr == NULL || *ptr == 0) {
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("result error: null");
			return result;
		}

		// 如果出错信息为重定向指令，则执行重定向过程
		if (EQ(ptr, "MOVED")) {
			conn = move(cmd, conn, ptr, n);
			if (conn == NULL) {
				return result;
			}
			last_moved = true;
		} else if (EQ(ptr, "ASK")) {
			conn = ask(cmd, conn, ptr, n);
			if (conn == NULL) {
				return result;
			}
			last_moved = false;
		}

		// 处理一个主结点失效的情形
		else if (EQ(ptr, "CLUSTERDOWN")) {
			conn = cluster_down(cmd, conn, ptr, n);
			if (conn == NULL) {
				return result;
			}
		}

		// 对于其它错误类型，则直接返回本次得到的响应结果对象
		else {
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("server error: %s", ptr);
			if (!cmd.is_slice_req()) {
				logger_error("request: %s", buf->c_str());
			}
			return result;
		}
	}

	if (conn != NULL) {
		conn->get_pool()->put(conn, true);
	}

	logger_warn("too many redirect: %d, max: %d", n, redirect_max_);
	return NULL;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
