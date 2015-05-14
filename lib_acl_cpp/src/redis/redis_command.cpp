#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_command.hpp"
#include "redis_request.hpp"

namespace acl
{

#define INT_LEN		11
#define	LONG_LEN	21

redis_command::redis_command()
: conn_(NULL)
, cluster_(NULL)
, max_conns_(0)
, used_(0)
, slot_(-1)
, redirect_max_(15)
, redirect_sleep_(100)
, slice_req_(false)
, request_buf_(NULL)
, request_obj_(NULL)
, argv_size_(0)
, argv_(NULL)
, argv_lens_(NULL)
, slice_res_(false)
, result_(NULL)
{
	pool_ = NEW dbuf_pool(128000);
	addr_[0] = 0;
}


redis_command::redis_command(redis_client* conn)
: conn_(conn)
, cluster_(NULL)
, max_conns_(0)
, used_(0)
, slot_(-1)
, redirect_max_(15)
, redirect_sleep_(1)
, slice_req_(false)
, request_buf_(NULL)
, request_obj_(NULL)
, argv_size_(0)
, argv_(NULL)
, argv_lens_(NULL)
, slice_res_(false)
, result_(NULL)
{
	pool_ = NEW dbuf_pool(128000);
	if (conn != NULL)
		set_client_addr(*conn);
	else
		addr_[0] = 0;
}

redis_command::redis_command(redis_client_cluster* cluster, size_t max_conns)
: conn_(NULL)
, cluster_(cluster)
, max_conns_(max_conns)
, used_(0)
, slot_(-1)
, slice_req_(false)
, request_buf_(NULL)
, request_obj_(NULL)
, argv_size_(0)
, argv_(NULL)
, argv_lens_(NULL)
, slice_res_(false)
, result_(NULL)
{
	pool_ = NEW dbuf_pool(128000);
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

redis_command::~redis_command()
{
	if (argv_ != NULL)
		acl_myfree(argv_);
	if (argv_lens_ != NULL)
		acl_myfree(argv_lens_);
	delete request_buf_;
	delete request_obj_;
	delete pool_;
}

void redis_command::reset(bool save_slot /* = false */)
{
	return clear(save_slot);
}

void redis_command::clear(bool save_slot /* = false */)
{
	if (used_ > 0)
	{
		delete pool_;
		pool_ = NEW dbuf_pool();
		result_ = NULL;
	}
	if (!save_slot)
		slot_ = -1;
}

void redis_command::set_slice_request(bool on)
{
	slice_req_ = on;
}

void redis_command::set_slice_respond(bool on)
{
	slice_res_ = on;
}

void redis_command::set_client(redis_client* conn)
{
	conn_ = conn;
	set_client_addr(*conn);
}

void redis_command::set_client_addr(redis_client& conn)
{
	socket_stream* stream = conn.get_stream();
	if (stream == NULL)
		addr_[0] = 0;
	else
		ACL_SAFE_STRNCPY(addr_, stream->get_peer(true), sizeof(addr_));
}

void redis_command::set_client_addr(const char* addr)
{
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
}

void redis_command::set_cluster(redis_client_cluster* cluster, size_t max_conns)
{
	cluster_ = cluster;
	max_conns_ = max_conns;
	if (max_conns_ == 0)
		max_conns_ = 100;
	redirect_max_ = cluster->get_redirect_max();
	if (redirect_max_ <= 0)
		redirect_max_ = 15;
	redirect_sleep_ = cluster->get_redirect_sleep();
}

bool redis_command::eof() const
{
	return conn_ == NULL ? false : conn_->eof();
}

void redis_command::argv_space(size_t n)
{
	if (argv_size_ >= n)
		return;
	argv_size_ = n;
	if (argv_ == NULL)
	{
		argv_ = (const char**) acl_mymalloc(n * sizeof(char*));
		argv_lens_ = (size_t*) acl_mymalloc(n * sizeof(size_t));
	}
	else
	{
		argv_ = (const char**) acl_myrealloc(argv_, n * sizeof(char*));
		argv_lens_ = (size_t*) acl_myrealloc(argv_lens_,
			n * sizeof(size_t));
	}
}

void redis_command::hash_slot(const char* key)
{
	hash_slot(key, strlen(key));
}

void redis_command::hash_slot(const char* key, size_t len)
{
	// ֻ�м�Ⱥģʽ����Ҫ�����ϣ��ֵ
	if (cluster_ == NULL)
		return;

	int max_slot = cluster_->get_max_slot();
	if (max_slot <= 0)
		return;

	// ��������˹�ϣ��ֵ���򲻱����¼���
	if (slot_ >= 0 && slot_ < max_slot)
		return;

	unsigned short n = acl_hash_crc16(key, len);
	slot_ = (int) (n % max_slot);
}

const char* redis_command::get_client_addr() const
{
	return addr_;
}

/////////////////////////////////////////////////////////////////////////////

size_t redis_command::result_size() const
{
	return result_ ? result_->get_size() : 0;
}

redis_result_t redis_command::result_type() const
{
	return result_ ? result_->get_type() : REDIS_RESULT_UNKOWN;
}

int redis_command::result_number(bool* success /* = NULL */) const
{
	return result_ ? result_->get_integer(success) : 0;
}

long long int redis_command::result_number64(bool* success /* = NULL */) const
{
	return result_ ? result_->get_integer64(success) : 0;
}

const char* redis_command::get_result(size_t i, size_t* len /* = NULL */) const
{
	return result_ ? result_->get(i, len) : NULL;
}

const char* redis_command::result_status() const
{
	return result_ ? result_->get_status() : "";
}

const char* redis_command::result_error() const
{
	return result_ ? result_->get_error() : "";
}

const redis_result* redis_command::result_child(size_t i) const
{
	return result_ ? result_->get_child(i) : NULL;
}

const char* redis_command::result_value(size_t i, size_t* len /* = NULL */) const
{
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	const redis_result* child = result_->get_child(i);
	if (child == NULL)
		return NULL;
	size_t size = child->get_size();
	if (size == 0)
		return NULL;
	if (size == 1)
		return child->get(0, len);

	// ���ڴ��п��ܱ���Ƭ�ɶ����������С�ڴ�
	size = child->get_length();
	size++;
	char* buf = (char*) pool_->dbuf_alloc(size);
	size = child->argv_to_string(buf, size);
	if (len)
		*len = size;
	return buf;
}

const redis_result* redis_command::get_result() const
{
	return result_;
}

// �����ض�����Ϣ������ض���ķ�������ַ
const char* redis_command::get_addr(const char* info)
{
	char* cmd = pool_->dbuf_strdup(info);
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

// ���������Ŀ���ַ�����ض��򣺴���õ�ַ�����ӣ��������ʧ�ܣ������
// ѡȡһ����������ַ��������
redis_client* redis_command::redirect(redis_client_cluster* cluster,
	const char* addr)
{
	redis_client_pool* conns;

	// �����������ַ�����ڣ�����ݷ�������ַ��̬�������ӳض���
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
	}

	logger_warn("too many retry: %d, addr: %s", i, addr);
	return NULL;
}

redis_client* redis_command::peek_conn(redis_client_cluster* cluster, int slot)
{
	// ����Ѿ������˹�ϣ��ֵ�������ȴӱ��ػ����в��Ҷ�Ӧ�����ӳ�
	// ���δ�ҵ���������м�Ⱥ����������һ�����õ����ӳض���

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

		// ȡ����ϣ�۵ĵ�ַӳ���ϵ
		cluster->clear_slot(slot);

		// �����ӳض�����Ϊ������״̬
		conns->set_alive(false);
	}

	logger_warn("too many retry: %d, slot: %d", i, slot);
	return NULL;
}

const redis_result* redis_command::run(redis_client_cluster* cluster,
	size_t nchild)
{
	redis_client* conn = peek_conn(cluster, slot_);

	// ���û���ҵ����õ����Ӷ�����ֱ�ӷ��� NULL ��ʾ����
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
		// ������������Ƿ�����ڴ��Ƭ��ʽ���ò�ͬ���������
		if (slice_req_)
			result_ = conn->run(pool_, *request_obj_, nchild);
		else
			result_ = conn->run(pool_, *request_buf_, nchild);

		// ��������쳣�Ͽ�������Ҫ��������
		if (conn->eof())
		{
			// ɾ����ϣ���еĵ�ַӳ���ϵ�Ա��´β���ʱ���»�ȡ
			cluster->clear_slot(slot_);

			// �����ӳض�����Ϊ������״̬
			conn->get_pool()->set_alive(false);

			// �����Ӷ���黹�����ӳض���
			conn->get_pool()->put(conn, false);

			// �����ӳؼ�Ⱥ��˳��ȡ��һ�����Ӷ���
			conn = peek_conn(cluster, slot_);
			if (conn != NULL)
			{
				last_moved = true;
				clear(true);
				set_client_addr(*conn);
				continue;
			}

			last_moved = false;
		}

		// �����Ӷ���黹�����ӳض���
		else
			conn->get_pool()->put(conn, true);

		if (result_ == NULL)
		{
			logger_error("result NULL");
			return NULL;
		}

		// ȡ�÷���������Ӧ��������ͣ������зֱ���
		type = result_->get_type();

		if (type == REDIS_RESULT_UNKOWN)
		{
			logger_error("unknown result type: %d", type);
			return NULL;
		}

		if (type != REDIS_RESULT_ERROR)
		{
			// ��������ض�����̣������ù�ϣ�۶�Ӧ redis �����ַ
			if (slot_ < 0 || !last_moved)
				return result_;

			const char* addr = conn->get_pool()->get_addr();
			cluster->set_slot(slot_, addr);

			return result_;
		}

#define	EQ(x, y) !strncasecmp((x), (y), sizeof(y) -1)

		// ���ڽ������Ϊ�������ͣ�����Ҫ��һ���ж��Ƿ����ض���ָ��
		const char* ptr = result_->get_error();
		if (ptr == NULL || *ptr == 0)
		{
			logger_error("result error: null");
			return result_;
		}

		// ���������ϢΪ�ض���ָ���ִ���ض������
		if (EQ(ptr, "MOVED"))
		{
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

			// ��Ҫ�����ϣ��ֵ
			clear(true);
		}
		else if (EQ(ptr, "ASK"))
		{
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

			result_ = conn->run(pool_, "ASKING\r\n", 0);
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

		// ����һ�������ʧЧ������
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

			conn = peek_conn(cluster, -1);
			if (conn == NULL)
			{
				logger_error("peek_conn NULL");
				return result_;
			}
			clear(true);

			set_client_addr(*conn);
		}

		// ���������������ͣ���ֱ�ӷ��ر��εõ�����Ӧ�������
		else
		{
			logger_error("server error: %s", ptr);
			return result_;
		}
	}

	logger_warn("too many redirect: %d, max: %d", n, redirect_max_);
	return NULL;
}

const redis_result* redis_command::run(size_t nchild /* = 0 */)
{
	used_++;

	if (cluster_ != NULL)
		return run(cluster_, nchild);
	else if (conn_ != NULL)
	{
		if (slice_req_)
			result_ = conn_->run(pool_, *request_obj_, nchild);
		else
			result_ = conn_->run(pool_, *request_buf_, nchild);
		return result_;
	}
	else
		return NULL;
}

/////////////////////////////////////////////////////////////////////////////

int redis_command::get_number(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER)
	{
		if (success)
			*success = false;
		return -1;
	}
	if (success)
		*success = true;
	return result->get_integer();
}

long long int redis_command::get_number64(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER)
	{
		if (success)
			*success = false;
		return -1;
	}
	if (success)
		*success = true;
	return result->get_integer64();
}

int redis_command::get_number(std::vector<int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer());
	}

	return size;
}

int redis_command::get_number64(std::vector<long long int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer64());
	}

	return size;
}

bool redis_command::check_status(const char* success /* = "OK" */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STATUS)
		return false;

	const char* status = result->get_status();
	if (status == NULL || *status == '\0')
		return false;
	else if (success == NULL || strcasecmp(status, success) == 0)
		return true;
	else
		return false;
}

int redis_command::get_status(std::vector<bool>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		out.push_back(rr->get_integer() > 0 ? true : false);
	}

	return (int) size;
}

const char* redis_command::get_status()
{
	const redis_result* result = run();
	return result == NULL ? "" : result->get_status();
}

int redis_command::get_string(string& buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	return result->argv_to_string(buf);
}

int redis_command::get_string(string* buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	if (buf == NULL)
		return (int) result->get_length();
	return result->argv_to_string(*buf);
}

int redis_command::get_string(char* buf, size_t size)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING)
		return -1;
	return result->argv_to_string(buf, size);
}

int redis_command::get_strings(std::vector<string>& out)
{
	return get_strings(&out);
}

int redis_command::get_strings(std::vector<string>* out)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (out == NULL)
		return result->get_size();

	out->clear();

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return 0;

	if (size > 0)
		out->reserve(size);

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
			out->push_back("");
		else if (rr->get_size() == 0)
			out->push_back("");
		else 
		{
			rr->argv_to_string(buf);
			out->push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}


int redis_command::get_strings(std::list<string>& out)
{
	return get_strings(&out);
}

int redis_command::get_strings(std::list<string>* out)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (out == NULL)
		return result->get_size();

	out->clear();

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return 0;

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++)
	{
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
			out->push_back("");
		else if (rr->get_size() == 0)
			out->push_back("");
		else 
		{
			rr->argv_to_string(buf);
			out->push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}

int redis_command::get_strings(std::map<string, string>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	string name, value;

	const redis_result* rr;
	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}
		name.clear();
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i++;
			continue;
		}
		value.clear();
		rr->argv_to_string(value);
		i++;

		out[name] = value;
	}

	return (int) out.size();
}

int redis_command::get_strings(std::vector<string>& names,
	std::vector<string>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	string name, value;
	const redis_result* rr;

	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}
		name.clear();
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i++;
			continue;
		}
		value.clear();
		rr->argv_to_string(value);
		i++;

		names.push_back(name);
		values.push_back(value);
	}

	return (int) names.size();
}

int redis_command::get_strings(std::vector<const char*>& names,
	std::vector<const char*>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	if (result->get_size() == 0)
		return 0;

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL)
		return -1;
	if (size % 2 != 0)
		return -1;

	char* nbuf, *vbuf;
	size_t len;
	const redis_result* rr;

	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i += 2;
			continue;
		}
		len = rr->get_length() + 1;
		nbuf = (char*) pool_->dbuf_alloc(len);
		rr->argv_to_string(nbuf, len);
		i++;

		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING)
		{
			i++;
			continue;
		}
		len = rr->get_length() + 1;
		vbuf = (char*) pool_->dbuf_alloc(len);
		rr->argv_to_string(vbuf, len);
		i++;

		names.push_back(nbuf);
		values.push_back(vbuf);
	}

	return (int) names.size();
}

/////////////////////////////////////////////////////////////////////////////

const redis_result** redis_command::scan_keys(const char* cmd, const char* key,
	int& cursor, size_t& size, const char* pattern, const size_t* count)
{
	size = 0;
	if (cursor < 0)
		return NULL;

	const char* argv[7];
	size_t lens[7];
	size_t argc = 0;

	argv[argc] = cmd;
	lens[argc] = strlen(cmd);
	argc++;

	if (key && *key)
	{
		argv[argc] = key;
		lens[argc] = strlen(key);
		argc++;
	}

	char cursor_s[INT_LEN];
	safe_snprintf(cursor_s, sizeof(cursor_s), "%d", cursor);
	argv[argc] = cursor_s;
	lens[argc] = strlen(cursor_s);
	argc++;

	if (pattern && *pattern)
	{
		argv[argc] = "MATCH";
		lens[argc] = sizeof("MATCH") - 1;
		argc++;

		argv[argc] = pattern;
		lens[argc] = strlen(pattern);
		argc++;
	}

	if (count && *count > 0)
	{
		argv[argc] = "COUNT";
		lens[argc] = sizeof("COUNT") - 1;
		argc++;

		char count_s[LONG_LEN];
		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) (*count));
		argv[argc] = count_s;
		lens[argc] = strlen(count_s);
		argc++;
	}

	if (key && *key)
		hash_slot(key);
	build_request(argc, argv, lens);
	const redis_result* result = run();
	if (result == NULL)
	{
		cursor = -1;
		return NULL;
	}

	if (result->get_size() != 2)
	{
		cursor = -1;
		return NULL;
	}

	const redis_result* rr = result->get_child(0);
	if (rr == NULL)
	{
		cursor = -1;
		return NULL;
	}
	string tmp(128);
	if (rr->argv_to_string(tmp) < 0)
	{
		cursor = -1;
		return NULL;
	}
	cursor = atoi(tmp.c_str());
	if (cursor < 0)
	{
		cursor = -1;
		return NULL;
	}

	rr = result->get_child(1);
	if (rr == NULL)
	{
		cursor = -1;
		return NULL;
	}

	const redis_result** children = rr->get_children(&size);
	if (children == NULL)
	{
		cursor = 0;
		size = 0;
	}

	return children;
}

void redis_command::clear_request()
{
	if (request_buf_)
		request_buf_->clear();
	if (request_obj_)
		request_obj_->clear();
}

void redis_command::build_request(size_t argc, const char* argv[], size_t lens[])
{
	if (slice_req_)
		build_request2(argc, argv, lens);
	else
		build_request1(argc, argv, lens);
}

void redis_command::build_request1(size_t argc, const char* argv[], size_t lens[])
{
	if (request_buf_ == NULL)
		request_buf_ = NEW string(256);
	else
		request_buf_->clear();
	request_buf_->format("*%lu\r\n", (unsigned long) argc);
	for (size_t i = 0; i < argc; i++)
	{
		request_buf_->format_append("$%lu\r\n", (unsigned long) lens[i]);
		request_buf_->append(argv[i], lens[i]);
		request_buf_->append("\r\n");
	}
	//printf("%s: %s", __FUNCTION__, request_buf_->c_str());
}

void redis_command::build_request2(size_t argc, const char* argv[], size_t lens[])
{
	size_t size = 1 + argc * 3;
	if (request_obj_ == NULL)
		request_obj_ = NEW redis_request();
	else
		request_obj_->clear();
	request_obj_->reserve(size);

#define BLEN	32

	char* buf = (char*) pool_->dbuf_alloc(BLEN);
	int  len = safe_snprintf(buf, BLEN, "*%lu\r\n", (unsigned long) argc);
	request_obj_->put(buf, len);

	for (size_t i = 0; i < argc; i++)
	{
		buf = (char*) pool_->dbuf_alloc(BLEN);
		len = safe_snprintf(buf, BLEN, "$%lu\r\n",
			(unsigned long) lens[i]);
		request_obj_->put(buf, len);

		request_obj_->put(argv[i], lens[i]);

		buf = (char*) pool_->dbuf_strdup("\r\n");
		request_obj_->put(buf, 2);
	}
}

//////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::map<string, string>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<string, string>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::map<string, const char*>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<string, const char*>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = cit->second;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::vector<string>& names, const std::vector<string>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<const char*>& names,
	const std::vector<const char*>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const char* names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const int names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = names_len[j];
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = values_len[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::vector<string>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j].c_str();
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<const char*>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<int>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const char* names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const char* names[], const size_t lens[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = lens[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const int names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL)
		argc_++;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL)
	{
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++)
	{
		buf4int = (char*) pool_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl
