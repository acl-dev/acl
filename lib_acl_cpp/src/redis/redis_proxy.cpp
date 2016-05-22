#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_proxy.hpp"

namespace acl
{

#define INT_LEN		11
#define	LONG_LEN	21

redis_proxy::redis_proxy(redis_client_cluster* cluster, size_t max_conns)
: conn_(NULL)
, cluster_(cluster)
, max_conns_(max_conns)
, used_(0)
, slot_(-1)
, request_buf_(NULL)    
, result_(NULL)
{
	dbuf_ = new dbuf_pool();
	addr_[0] = 0;

	if (cluster != NULL)
	{
		redirect_max_ = cluster->get_redirect_max();
		if (redirect_max_ <= 0)
			redirect_max_ = 15;
		redirect_sleep_ = cluster->get_redirect_sleep();
	}
	else
	{
		redirect_max_ = 15;
		redirect_sleep_ = 1;
	}
}

redis_proxy::~redis_proxy()
{
	dbuf_->destroy();
}

void redis_proxy::reset(bool save_slot /* = false */)
{
	return clear(save_slot);
}

void redis_proxy::clear(bool save_slot /* = false */)
{
	if (used_ > 0)
	{
		dbuf_->dbuf_reset();
		result_ = NULL;
	}
	if (!save_slot)
		slot_ = -1;
}

void redis_proxy::set_client(redis_client* conn)
{
	conn_ = conn;
	set_client_addr(*conn);
}

void redis_proxy::set_client_addr(redis_client& conn)
{
	socket_stream* stream = conn.get_stream();
	if (stream == NULL)
		addr_[0] = 0;
	else
		ACL_SAFE_STRNCPY(addr_, stream->get_peer(true), sizeof(addr_));
}

void redis_proxy::set_client_addr(const char* addr)
{
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
}

void redis_proxy::set_cluster(redis_client_cluster* cluster, size_t max_conns)
{
	cluster_ = cluster;
	max_conns_ = max_conns;
	if (cluster == NULL)
		return;

	redirect_max_ = cluster->get_redirect_max();
	if (redirect_max_ <= 0)
		redirect_max_ = 15;
	redirect_sleep_ = cluster->get_redirect_sleep();
}

bool redis_proxy::eof() const
{
	return conn_ == NULL ? false : conn_->eof();
}

void redis_proxy::hash_slot(const char* key)
{
	hash_slot(key, strlen(key));
}

void redis_proxy::hash_slot(const char* key, size_t len)
{
	// 只有集群模式才需要计算哈希槽值
	if (cluster_ == NULL)
		return;

	int max_slot = cluster_->get_max_slot();
	if (max_slot <= 0)
		return;

	// 如果缓存了哈希槽值，则不必重新计算
	if (slot_ >= 0 && slot_ < max_slot)
		return;

	unsigned short n = acl_hash_crc16(key, len);
	slot_ = (int) (n % max_slot);
}

const char* redis_proxy::get_client_addr() const
{
	return addr_;
}

const redis_result* redis_proxy::get_result() const
{
	return result_;
}

// 分析重定向信息，获得重定向的服务器地址
const char* redis_proxy::get_addr(const char* info)
{
	char* cmd = dbuf_->dbuf_strdup(info);
	char* slot = strchr(cmd, ' ');
	if (slot == NULL)
		return NULL;
	*slot++ = 0;
	char* addr = strchr(slot, ' ');
	if (addr == NULL)
		return NULL;
	*addr++ = 0;
	if (*addr == 0)
		return NULL;

	return addr;
}

// 根据输入的目标地址进行重定向：打开与该地址的连接，如果连接失败，则随机
// 选取一个服务器地址进行连接
redis_client* redis_proxy::redirect(redis_client_cluster* cluster,
	const char* addr)
{
	redis_client_pool* conns;

	// 如果服务器地址不存在，则根据服务器地址动态创建连接池对象
	if ((conns = (redis_client_pool*) cluster->get(addr)) == NULL)
		conns = (redis_client_pool*) &cluster->set(addr, max_conns_);

	if (conns == NULL)
		return NULL;

	redis_client* conn;

	int i = 0;

	while (i++ < 5)
	{
		conn = (redis_client*) conns->peek();
		if (conn != NULL)
			return conn;

		conns->set_alive(false);
		conns = (redis_client_pool*) cluster->peek();
		if (conns == NULL)
		{
			logger_error("no connections availabble, "
				"i: %d, addr: %s", i, addr);
			return NULL;
		}
	}

	logger_warn("too many retry: %d, addr: %s", i, addr);
	return NULL;
}

redis_client* redis_proxy::peek_conn(redis_client_cluster* cluster, int slot)
{
	// 如果已经计算了哈希槽值，则优先从本地缓存中查找对应的连接池
	// 如果未找到，则从所有集群结点中随便找一个可用的连接池对象

	redis_client_pool* conns;
	redis_client* conn;
	int i = 0;

	while (i++ < 5)
	{
		if (slot < 0)
			conns = (redis_client_pool*) cluster->peek();
		else if ((conns = cluster->peek_slot(slot)) == NULL)
			conns = (redis_client_pool*) cluster->peek();

		if (conns == NULL)
		{
			slot = -1;
			continue;
		}

		conn = (redis_client*) conns->peek();
		if (conn != NULL)
			return conn;

		// 取消哈希槽的地址映射关系
		cluster->clear_slot(slot);

		// 将连接池对象置为不可用状态
		conns->set_alive(false);
	}

	logger_warn("too many retry: %d, slot: %d", i, slot);
	return NULL;
}

bool redis_proxy::build_request(const char* data, size_t len)
{
	if(!data||len<=0) return false;

	if (request_buf_ == NULL)
		request_buf_ = NEW string(len+1);
	else
		request_buf_->clear();
	request_buf_->append(data,len);
	return true;
}

const redis_result* redis_proxy::run(redis_client_cluster* cluster,
	size_t nchild)
{
	redis_client* conn = peek_conn(cluster, slot_);

	// 如果没有找到可用的连接对象，则直接返回 NULL 表示出错
	if (conn == NULL)
	{
		logger_error("peek_conn NULL, slot_: %d", slot_);
		return NULL;
	}

	set_client_addr(*conn);

	redis_result_t type;
	bool  last_moved = false;
	int   n = 0;

	while (n++ < redirect_max_)
	{
		result_ = conn->run(dbuf_, *request_buf_, nchild);

		// 如果连接异常断开，则需要进行重试
		if (conn->eof())
		{
			// 删除哈希槽中的地址映射关系以便下次操作时重新获取
			cluster->clear_slot(slot_);

			// 将连接池对象置为不可用状态
			conn->get_pool()->set_alive(false);

			// 将连接对象归还给连接池对象
			conn->get_pool()->put(conn, false);

			// 从连接池集群中顺序取得一个连接对象
			conn = peek_conn(cluster, slot_);
			if (conn == NULL)
			{
				logger_error("peek_conn NULL");
				return result_;
			}

			last_moved = true;
			clear(true);
			set_client_addr(*conn);
			continue;
		}

		if (result_ == NULL)
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("result NULL");

			return NULL;
		}

		// 取得服务器的响应结果的类型，并进行分别处理
		type = result_->get_type();

		if (type == REDIS_RESULT_UNKOWN)
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("unknown result type: %d", type);

			return NULL;
		}

		if (type != REDIS_RESULT_ERROR)
		{
			// 如果发生重定向过程，则设置哈希槽对应 redis 服务地址
			if (slot_ < 0 || !last_moved)
			{
				// 将连接对象归还给连接池对象
				conn->get_pool()->put(conn, true);
				return result_;
			}

			// XXX: 因为此处还要引用一次 conn 对象，所以将 conn
			// 归还给连接池的过程须放在此段代码之后
			const char* addr = conn->get_pool()->get_addr();
			cluster->set_slot(slot_, addr);

			// 将连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);

			return result_;
		}

#define	EQ(x, y) !strncasecmp((x), (y), sizeof(y) -1)

		// 对于结果类型为错误类型，则需要进一步判断是否是重定向指令
		const char* ptr = result_->get_error();
		if (ptr == NULL || *ptr == 0)
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("result error: null");

			return result_;
		}

		// 如果出错信息为重定向指令，则执行重定向过程
		if (EQ(ptr, "MOVED"))
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);

			const char* addr = get_addr(ptr);
			if (addr == NULL)
			{
				logger_warn("MOVED invalid, ptr: %s", ptr);
				return result_;
			}

			conn = redirect(cluster, addr);

			if (conn == NULL)
			{
				logger_error("redirect NULL, addr: %s", addr);
				return result_;
			}

			ptr = conn->get_pool()->get_addr();

			set_client_addr(ptr);

			if (n >= 2 && redirect_sleep_ > 0
				&& strcmp(ptr, addr) != 0)
			{
				logger("redirect %d, curr %s, waiting %s ...",
					n, ptr, addr);
				acl_doze(redirect_sleep_);
			}

			last_moved = true;

			// 需要保存哈希槽值
			clear(true);
		}
		else if (EQ(ptr, "ASK"))
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);

			const char* addr = get_addr(ptr);
			if (addr == NULL)
			{
				logger_warn("ASK invalid, ptr: %s", ptr);
				return result_;
			}

			conn = redirect(cluster, addr);
			if (conn == NULL)
			{
				logger_error("redirect NULL, addr: %s", addr);
				return result_;
			}

			ptr = conn->get_pool()->get_addr();

			set_client_addr(ptr);

			if (n >= 2 && redirect_sleep_ > 0
				&& strcmp(ptr, addr) != 0)
			{
				logger("redirect %d, curr %s, waiting %s ...",
					n, ptr, addr);
				acl_doze(redirect_sleep_);
			}

			result_ = conn->run(dbuf_, "ASKING\r\n", 0);
			if (result_ == NULL)
			{
				logger_error("ASKING's reply null");
				return NULL;
			}

			const char* status = result_->get_status();
			if (status == NULL || strcasecmp(status, "OK") != 0)
			{
				logger_error("ASKING's reply error: %s",
					status ? status : "null");
				return NULL;
			}

			last_moved = false;
			clear(true);
		}

		// 处理一个主结点失效的情形
		else if (EQ(ptr, "CLUSTERDOWN"))
		{
			cluster->clear_slot(slot_);

			if (redirect_sleep_ > 0)
			{
				logger("%s: redirect %d, slot %d, waiting %s ...",
					conn->get_pool()->get_addr(),
					n, slot_, ptr);
				acl_doze(redirect_sleep_);
			}

			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);

			conn = peek_conn(cluster, -1);
			if (conn == NULL)
			{
				logger_error("peek_conn NULL");
				return result_;
			}

			clear(true);
			set_client_addr(*conn);
		}

		// 对于其它错误类型，则直接返回本次得到的响应结果对象
		else
		{
			// 将旧连接对象归还给连接池对象
			conn->get_pool()->put(conn, true);
			logger_error("server error: %s", ptr);
			logger_error("request: %s",request_buf_->c_str());
			return result_;
		}
	}

	logger_warn("too many redirect: %d, max: %d", n, redirect_max_);
	return NULL;
}

const redis_result* redis_proxy::run(size_t nchild /* = 0 */)
{
	used_++;

	if (cluster_ != NULL)
		return run(cluster_, nchild);
	else if (conn_ != NULL)
	{
		result_ = conn_->run(dbuf_, *request_buf_, nchild);
		return result_;
	}
	else
	{
		logger_error("ERROR: cluster_ and conn_ are all NULL");
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////

void redis_proxy::logger_result(const redis_result* result)
{
	if (result == NULL)
	{
		logger_error("result NULL");
		return;
	}

	string res;
	result->to_string(res);

	logger_error("result type: %d， error: %s, res: [%s], req: [%s]",
		result->get_type(), result_->get_error(), res.c_str(),
		request_buf_ ? request_buf_->c_str() : "slice request");
}

void redis_proxy::clear_request()
{
	if (request_buf_)
		request_buf_->clear();
}

} // namespace acl
