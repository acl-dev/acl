#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/connpool/connect_client.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"
#endif

namespace acl {

class check_job : public thread_job {
public:
	check_job(tbox<bool>& box, connect_client& conn, bool opened)
	: box_(box), conn_(conn), opened_(opened), aliving_(true) {}

	~check_job() {}

	// Get the checking result for the given connection.
	bool aliving() const {
		return aliving_;
	}

	// Get the connection.
	connect_client& get_conn() const {
		return conn_;
	}

protected:
	// @overide
	void* run() {
		if (opened_) {
			aliving_ = conn_.alive();
		} else {
			aliving_ = conn_.open();
		}

		box_.push(NULL);
		return NULL;
	}

private:
	tbox<bool>& box_;
	connect_client& conn_;
	bool opened_;
	bool aliving_;
};

connect_pool::connect_pool(const char* addr, size_t max, size_t idx /* = 0 */)
: alive_(true)
, refers_(0)
, delay_destroy_(false)
, last_dead_(0)
, conn_timeout_(30)
, rw_timeout_(30)
, idx_(idx)
, max_(max)
, min_(0)
, count_(0)
, idle_ttl_(-1)
, last_check_(0)
, check_inter_(30)
, total_used_(0)
, current_used_(0)
, last_(0)
{
	retry_inter_ = 1;
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
	ACL_SAFE_STRNCPY(key_, addr_, sizeof(key_));
	acl_lowercase(key_);
}

connect_pool::~connect_pool()
{
	std::list<connect_client*>::iterator it = pool_.begin();
	for (; it != pool_.end(); ++it) {
		delete *it;
	}
}

void connect_pool::count_inc(bool exclusive) {
	if (exclusive) {
		lock_.lock();
	}
	++count_;
	++refers_;
	if (exclusive) {
		lock_.unlock();
	}
}

void connect_pool::count_dec(bool exclusive) {
	if (exclusive) {
		lock_.lock();
	}
	--count_;
	--refers_;
	if (exclusive) {
		lock_.unlock();
	}
}

void connect_pool::refer()
{
	lock_.lock();
	++refers_;
	lock_.unlock();
}

void connect_pool::unrefer()
{
	lock_.lock();
	if (--refers_ <= 0 && delay_destroy_) {
		lock_.unlock();
		delete this;
	} else {
		lock_.unlock();
	}
}

void connect_pool::set_key(const char* key)
{
	ACL_SAFE_STRNCPY(key_, key, sizeof(key_));
	acl_lowercase(key_);
}

connect_pool& connect_pool::set_timeout(int conn_timeout, int rw_timeout,
	bool sockopt_timeo /* false */)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_   = rw_timeout;
	sockopt_timo_ = sockopt_timeo;
	return *this;
}

connect_pool& connect_pool::set_conns_min(size_t min) {
	min_ = min;
	return *this;
}

connect_pool& connect_pool::set_idle_ttl(time_t ttl)
{
	idle_ttl_ = ttl;
	return *this;
}

connect_pool& connect_pool::set_retry_inter(int retry_inter)
{
	lock_.lock();
	retry_inter_ = retry_inter;
	lock_.unlock();

	return *this;
}

connect_pool& connect_pool::set_check_inter(int n)
{
	check_inter_ = n;
	return *this;
}

void connect_pool::reset_statistics(int inter)
{
	time_t now = time(NULL);
	lock_.lock();
	if (now - last_ >= inter) {
		last_ = now;
		current_used_ = 0;
	}
	lock_.unlock();
}

bool connect_pool::aliving()
{
	// XXX，虽然此处未加锁，但也应该不会有问题，因为下面的 peek() 过程会再次
	// 对 alive_ 加锁，以防止多线程操作时的冲突
	if (alive_) {
		return true;
	}

	time_t now = time(NULL);

	lock_.lock();
	if (retry_inter_ > 0 && now - last_dead_ >= retry_inter_) {
		alive_ = true;
		lock_.unlock();

		// 重置服务端连接状态，以便重试
		logger("reset server: %s", get_addr());
		return true;
	}

	lock_.unlock();
	return false;
}

connect_client* connect_pool::peek(bool on, double* tc, bool* old)
{
	struct timeval begin;

#define	SET_TIME_COST do {                                                    \
	if (tc) {                                                             \
		struct timeval end;                                           \
		gettimeofday(&end, NULL);                                     \
		*tc = stamp_sub(end, begin);                                  \
	}                                                                     \
} while (0)

	if (tc) {
		*tc = 0.0;
		gettimeofday(&begin, NULL);
	}

	if (old) {
		*old = false;
	}

	lock_.lock();
	if (alive_ == false) {
		time_t now = time(NULL);
		if (retry_inter_ <= 0 || now - last_dead_ < retry_inter_) {
			lock_.unlock();
			return NULL;
		}
		alive_ = true;

		// 重置服务端连接状态，以便重试
		logger("reset server: %s", get_addr());
	}

	connect_client* conn;

	std::list<connect_client*>::iterator it = pool_.begin();
	if (it != pool_.end()) {
		conn = *it;
		pool_.erase(it);
		total_used_++;
		current_used_++;

		lock_.unlock();

		SET_TIME_COST;

		if (old) {
			*old = true;
		}

		return conn;
	} else if (max_ > 0 && count_ >= max_) {
		logger_error("too many connections, max: %zd, curr: %zd,"
			" server: %s", max_, count_, addr_);
		lock_.unlock();

		SET_TIME_COST;
		return NULL;
	}

	if (!on) {
		lock_.unlock();

		SET_TIME_COST;
		return NULL;
	}

	// 将以下三个值预 +1
	count_inc(false);
	total_used_++;
	current_used_++;

	lock_.unlock();

	// 调用虚函数的子类实现方法，创建新连接对象，并打开连接
	conn = create_connect();
	if (conn == NULL) {
		lock_.lock();
		count_dec(false);
		total_used_--;
		current_used_--;
#ifdef AUTO_SET_ALIVE
		alive_ = false;
		(void) time(&last_dead_);
#endif
		lock_.unlock();

		SET_TIME_COST;
		return NULL;
	}

	// 在调用 open 之前先设置超时时间
	conn->set_timeout(conn_timeout_, rw_timeout_);

	// 调用子类方法打开连接
	if (!conn->open()) {
		lock_.lock();
		// 因为打开连接失败，所以还需将上面预 +1 的三个成员再 -1
		count_dec(false);
		total_used_--;
		current_used_--;
#ifdef AUTO_SET_ALIVE
		alive_ = false;
		(void) time(&last_dead_);
#endif
		lock_.unlock();
		delete conn;

		SET_TIME_COST;
		return NULL;
	}

	conn->set_pool(this);

	SET_TIME_COST;
	return conn;
}

void connect_pool::bind_one(connect_client* conn)
{
	if (conn->get_pool() != this) {
		conn->set_pool(this);
		count_inc(true);
	}
}

void connect_pool::put(connect_client* conn, bool keep /* = true */,
       cpool_put_oper_t oper /* cpool_put_check_idle */)
{
	time_t now = time(NULL);

	lock_.lock();

	// 检查是否设置了自销毁标志位
	if (delay_destroy_) {
		if (conn->get_pool() == this) {
			count_dec(false);
		}
		delete conn;

		if (refers_ <= 0) {
			// 如果引用计数为 0 则自销毁
			lock_.unlock();
			delete this;
		} else {
			lock_.unlock();
		}
		return;
	}

	if (keep && alive_) {
		conn->set_when(now);

		// 将归还的连接放在链表首部，这样在调用释放过期连接
		// 时比较方便，有利于尽快将不忙的数据库连接关闭
		pool_.push_front(conn);
	} else {
		acl_assert(count_ > 0);
		if (conn->get_pool() == this) {
			count_dec(false);
		}
		delete conn;
	}

	if (check_inter_ >= 0 && now - last_check_ >= check_inter_) {
		lock_.unlock();
		if (oper & cpool_put_check_idle) {
			(void) check_idle(idle_ttl_, true);
		}
		if (oper & cpool_put_check_dead) {
			(void) check_dead();
		}
		if (oper & cpool_put_keep_conns) {
			keep_conns();
		}
	} else {
		lock_.unlock();
	}
}

void connect_pool::set_delay_destroy()
{
	lock_.lock();
	delay_destroy_ = true;
	lock_.unlock();
}

void connect_pool::set_alive(bool yes /* true | false */)
{
	lock_.lock();
	alive_ = yes;
	if (yes == false) {
		time(&last_dead_);
	}
	lock_.unlock();
}

size_t connect_pool::check_idle(bool exclusive /* true */)
{
	return check_idle(idle_ttl_, exclusive);
}

size_t connect_pool::check_idle(time_t ttl, bool exclusive /* true */)
{
	if (exclusive) {
		lock_.lock();
	}

	(void) time(&last_check_);

	if (pool_.empty() && min_ == 0) {
		if (exclusive) {
			lock_.unlock();
		}
		return 0;
	}

	size_t n = 0;

	if (ttl == 0) {
		std::list<connect_client*>::iterator it = pool_.begin();
		for (; it != pool_.end(); ++it) {
			delete *it;
			count_dec(false);
			n++;
		}

		pool_.clear();

		if (exclusive) {
			lock_.unlock();
		}
		return n;
	}

	if (ttl > 0) {
		n += kick_idle_conns(ttl);
	}

	if (exclusive) {
		lock_.unlock();
	}

	return n;
}

size_t connect_pool::kick_idle_conns(time_t ttl)
{
	time_t now = time(NULL), when;

	std::list<connect_client*>::iterator it, next;
	std::list<connect_client*>::reverse_iterator rit = pool_.rbegin();

	size_t n = 0;

	for (; rit != pool_.rend();) {
		it = --rit.base();
		when = (*it)->get_when();
		if (when <= 0) {
			++rit;
			continue;
		}

		if (now - when < ttl) {
			break;
		}

		// If min > 0, try to keep the minimal count of connections.
		if (min_ > 0 && count_ <= min_) {
			break;
		}

		// Decrease connections count only if the connection is mine.
		if ((*it)->get_pool() == this) {
			count_dec(false);
		}

		delete *it;
		next = pool_.erase(it);
		rit = std::list<connect_client*>::reverse_iterator(next);
		n++;
	}

	return n;
}

size_t connect_pool::check_dead(thread_pool* threads /* NULL */)
{
	lock_.lock();
	size_t count = count_;
	lock_.unlock();

	if (count == 0) {
		return 0;
	}

	if (threads == NULL) {
		return check_dead(count);
	}
	return check_dead(count, *threads);
}

size_t connect_pool::check_dead(size_t count)
{
	size_t n = 0;
	for (size_t i = 0; i < count; i++) {
		connect_client* conn = peek_back();
		if (conn == NULL) {
			break;
		}

		if (conn->alive()) {
			put_front(conn);
			continue;
		}

		if (conn->get_pool() == this) {
			count_dec(true);
		}
		delete conn;
		n++;
	}

	return n;
}

size_t connect_pool::check_dead(size_t count, thread_pool& threads)
{
	// Check all connections in threads pool.
	size_t n = 0;
	tbox<bool> box;
	std::vector<check_job*> jobs;
	for (size_t i = 0; i < count; i++) {
		connect_client* conn = peek_back();
		if (conn == NULL) {
			break;
		}

		check_job* job = NEW check_job(box, *conn, true);
		jobs.push_back(job);
		threads.execute(job);
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	bool found;
	for (size_t i = 0; i < jobs.size(); i++) {
		(void) box.pop(-1, &found);
		if (!found) {
			logger_fatal("Pop message from thread job failed!");
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double tc = stamp_sub(end, begin);
	logger("Addr: %s; threads: limit=%zd, count=%d; jobs count=%zd, %zd, time cost=%.2f ms",
	       addr_, threads.get_limit(), threads.threads_count(), jobs.size(), count, tc);

	for (std::vector<check_job*>::iterator it = jobs.begin();
	     it != jobs.end(); ++it) {
		connect_client* conn = &(*it)->get_conn();

		if ((*it)->aliving()) {
			delete *it;
			put_front(conn);
			continue;
		}

		delete *it;

		if (conn->get_pool() == this) {
			count_dec(true);
		}
		delete conn;
		n++;
	}

	return n;
}

connect_client* connect_pool::peek_back()
{
	lock_.lock();
#if 1
	std::list<connect_client*>::reverse_iterator rit = pool_.rbegin();
	if (rit == pool_.rend()) {
		lock_.unlock();
		return NULL;
	}

	std::list<connect_client*>::iterator it = --rit.base();
	connect_client* conn = *it;
	pool_.erase(it);
#else
	std::list<connect_client*>::reverse_iterator rit = pool_.rbegin();
	if (rit == pool_.rend()) {
		lock_.unlock();
		return NULL;
	}
	connect_client* conn = *rit;
	pool_.erase(--rit.base());
#endif
	lock_.unlock();
	return conn;
}


void connect_pool::put_front(connect_client* conn)
{
	time_t now = time(NULL);

	lock_.lock();

	// 检查是否设置了自销毁标志位
	if (delay_destroy_) {
		if (conn->get_pool() == this) {
			count_dec(false);
		}
		delete conn;

		if (refers_ <= 0) {
			// 如果引用计数为 0 则自销毁
			lock_.unlock();
			delete this;
		} else {
			lock_.unlock();
		}
		return;
	}

	alive_ = true;  // 该连接充当服务检测成功功能，所以可在此处设置服务可用

	conn->set_when(now);

	// 将归还的连接放在链表首部，这样在调用释放过期连接
	// 时比较方便，有利于尽快将不忙的数据库连接关闭
	pool_.push_front(conn);

	lock_.unlock();
}

void connect_pool::keep_conns(thread_pool* threads /* NULL */)
{
	lock_.lock();
	size_t min;
	if (min_ > 0 && min_ > count_) {
		min = min_ - count_;
	} else {
		min = 0;
	}
	lock_.unlock();

	if (min == 0) {
		return;
	}

	if (threads) {
		keep_conns(min, *threads);
	} else {
		keep_conns(min);
	}
}

void connect_pool::keep_conns(size_t min)
{
	for (size_t i = 0; i < min; i++) {
		lock_.lock();
		if (min_ > 0 && count_ >= min_) {
			lock_.unlock();
			break;
		}
		lock_.unlock();

		connect_client* conn = this->create_connect();
		if (conn == NULL) {
			logger_error("Create connection error");
			break;
		}

		if (!conn->open()) {
			logger_error("Open error: %s", last_serror());
			delete conn;
			break;
		}

		conn->set_pool(this);
		put(conn, true);

		lock_.lock();
		alive_ = true;
		count_inc(false);
		if (max_ > 0 && count_ >= max_) {
			lock_.unlock();
			break;
		}
		lock_.unlock();
	}
}

void connect_pool::keep_conns(size_t min, thread_pool& threads)
{
	tbox<bool> box;
	std::vector<check_job*> jobs;
	for (size_t i = 0; i < min; i++) {
		lock_.lock();
		if (min_ > 0 && count_ >= min_) {
			lock_.unlock();
			break;
		}
		lock_.unlock();

		connect_client* conn = this->create_connect();
		check_job* job = NEW check_job(box, *conn, false);
		jobs.push_back(job);
		threads.execute(job);
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	// Waiting all jobs finished.
	bool found;
	for (size_t i = 0; i < jobs.size(); i++) {
		(void) box.pop(-1, &found);
		if (!found) {
			logger_fatal("Pop message from thread job failed!");
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double tc = stamp_sub(end, begin);
	logger("Addr=%s; threads: limit=%zd, count=%d; jobs count=%zd, time cost=%.2f ms",
		addr_, threads.get_limit(), threads.threads_count(), jobs.size(), tc);

	for (std::vector<check_job*>::iterator it = jobs.begin();
	     it != jobs.end(); ++it) {
		connect_client* conn = &(*it)->get_conn();
		if (!(*it)->aliving()) {
			delete conn;
			delete *it;
			continue;
		}

		delete *it;

		lock_.lock();
		alive_ = true;
		if (max_ > 0 && count_ >= max_) {
			lock_.unlock();
			delete *it;
			continue;
		}
		lock_.unlock();

		conn->set_pool(this);
		put(conn, true);

		count_inc(true);
	}
}

} // namespace acl
