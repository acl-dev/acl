#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <stdarg.h>
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/beanstalk/beanstalk.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

namespace acl
{

#ifdef ACL_WINDOWS
#define atoll _atoi64
#endif


beanstalk::beanstalk(const char* addr, int conn_timeout,
	bool retry /* = true */)
: timeout_(conn_timeout)
, retry_(retry)
{
	addr_ = acl_mystrdup(addr);
	acl_lowercase(addr_);
	errbuf_[0] = 0;
	tube_used_ = NULL;
	// 放置缺省队列
	tubes_watched_.push_back(acl_mystrdup("default"));
}

beanstalk::~beanstalk(void)
{
	acl_myfree(addr_);
	if (tube_used_) {
		acl_myfree(tube_used_);
	}
	std::vector<char*>::iterator it = tubes_watched_.begin();
	for (; it != tubes_watched_.end(); ++it) {
		acl_myfree(*it);
	}
}

bool beanstalk::beanstalk_open(void)
{
	if (conn_.opened()) {
		return true;
	}
	if (!conn_.open(addr_, timeout_, 0)) {
		logger_error("connect server: %s error: %s",
			addr_, last_serror());
		errbuf_.format("connect");
		return false;
	}
	if (!tube_used_ && beanstalk_use()) {
		logger_error("use %s error: %s", tube_used_, last_serror());
		conn_.close();
		return false;
	}

	if (tubes_watched_.empty()) {
		return true;
	}

	std::vector<char*>::iterator it = tubes_watched_.begin();
	for (; it != tubes_watched_.end(); ++it) {
		if (!beanstalk_watch(*it)) {
			logger_error("watch %s failed", *it);
			conn_.close();
			return false;
		}
	}

	return true;
}

bool beanstalk::beanstalk_use(void)
{
	if (tube_used_ == NULL) {
		return true;
	}

	if (conn_.format("use %s\r\n", tube_used_) == -1) {
		logger_error("use %s error: %s", tube_used_, last_serror());
		conn_.close();
		errbuf_.format("use");
		return false;
	}
	string line(128);
	if (conn_.gets(line) == false || line.empty()) {
		conn_.close();
		logger_error("gets from beanstalkd(%s) error: %s",
			addr_, last_serror());
		errbuf_.format("gets");
		return false;
	}

	ACL_ARGV* tokens = acl_argv_split(line.c_str(), "\t ");
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "USING")
		|| strcasecmp(tokens->argv[1], tube_used_)) {

		logger_error("'use %s' error %s", tube_used_, tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

unsigned beanstalk::beanstalk_watch(const char* tube)
{
	if (conn_.format("watch %s\r\n", tube) == -1) {
		logger_error("'watch %s' failed: %s", tube, last_serror());
		errbuf_ = "watch";
		return 0;
	}
	string line(128);
	if (!conn_.gets(line)) {
		logger_error("'watch %s' error(%s): reply ailed",
			last_serror(), tube);
		errbuf_ = "watch";
		return 0;
	}

	ACL_ARGV* tokens = acl_argv_split(line.c_str(), "\t ");
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "WATCHING")) {
		logger_error("'watch %s' error: %s", tube, line.c_str());
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return 0;
	}

	unsigned n = (unsigned) atoi(tokens->argv[1]);
	acl_argv_free(tokens);

	// 如果服务器返回所关注的队列数为 0，肯定是出错了，因为至少还有一个
	// 缺省队列：default，所以此时需要关闭连接，以尽量消除与本连接相关
	// 的错误，下一个命令会自动进行重连操作以恢复操作过程
	if (n == 0) {
		logger_error("'watch %s' error(%s), tube watched is 0",
			line.c_str(), tube);
		errbuf_ = line.c_str();
		close();
	}
	return n;
}

ACL_ARGV* beanstalk::beanstalk_request(const string& cmdline,
	const void* data /*= NULL */, size_t len /* = 0 */)
{
	string line(128);
	bool retried = false;

	while (true) {
		if (!beanstalk_open()) {
			return NULL;
		}

		// 先写入数据头
		if (conn_.write(cmdline) == -1) {
			conn_.close();
			if (retry_ && !retried) {
				retried = true;
				continue;
			}
			logger_error("write to beanstalkd(%s) error: %s",
				addr_, last_serror());
			errbuf_ = "write";
			return NULL;
		}

		// 如果有数据体，则写入数据体
		if (data && len > 0 && (conn_.write(data, len) == -1
			|| conn_.write("\r\n", 2) == -1)) {

			conn_.close();
			if (retry_ && !retried) {
				retried = true;
				continue;
			}
			logger_error("write to beanstalkd(%s) error: %s",
				addr_, last_serror());
			errbuf_ = "write";
			return NULL;
		}

		line.clear();
		if (!conn_.gets(line) || line.empty()) {
			conn_.close();
			if (retry_ && !retried) {
				retried = true;
				continue;
			}
			logger_error("gets from beanstalkd(%s) error: %s",
				addr_, last_serror());
			errbuf_ = "gets";
			return NULL;
		}
		break;
	}

	ACL_ARGV* tokens = acl_argv_split(line.c_str(), "\t ");
	return tokens;
}

bool beanstalk::open(void)
{
	if (conn_.opened()) {
		return true;
	}

	if (!conn_.open(addr_, timeout_, 0)) {
		logger_error("connect beanstalkd %s error: %s",
			addr_, last_serror());
		errbuf_= "connect";
		return false;
	}
	return true;
}

void beanstalk::close(void)
{
	if (conn_.opened()) {
		conn_.close();
	}
	if (tube_used_) {
		acl_myfree(tube_used_);
		tube_used_ = NULL;
	}
	std::vector<char*>::iterator it = tubes_watched_.begin();
	for (; it != tubes_watched_.end(); ++it) {
		acl_myfree(*it);
	}
	tubes_watched_.clear();
}

bool beanstalk::use(const char* tube)
{
	if (tube_used_) {
		// 在 beanstalk_request 前释放掉该值，可以避免在
		// beanstalk_request 触发 beanstalk_use 过程，虽然
		// 触发该过程并没有害处，但却多了一次通讯过程
		acl_myfree(tube_used_);
		tube_used_ = NULL;
	}

	string cmdline(128);
	cmdline.format("use %s\r\n", tube);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "USING")
		|| strcasecmp(tokens->argv[1], tube)) {

		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return false;
	}

	acl_argv_free(tokens);

	// 放在后面，在第一次使用时可以避免多一次通讯交互
	tube_used_ = acl_mystrdup(tube);
	return true;
}

unsigned long long beanstalk::put(const void* data, size_t n,
	unsigned pri /* = 1024 */, unsigned delay /* = 0 */,
	unsigned int ttr /* = 60 */)
{
	errbuf_.clear();
	string cmdline(128);
	cmdline.format("put %u %u %u %u\r\n", pri, delay, ttr, (unsigned int) n);
	ACL_ARGV* tokens = beanstalk_request(cmdline, data, n);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return 0;
	}

	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "INSERTED")) {
		logger_error("'%s' error", cmdline.c_str());
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return 0;
	}

#ifdef MINGW
	unsigned long long id = (unsigned long long) atol(tokens->argv[1]);
#else
	unsigned long long id = (unsigned long long) atoll(tokens->argv[1]);
#endif
	acl_argv_free(tokens);
	return id;
}

unsigned long long beanstalk::format_put(unsigned pri, unsigned delay,
	unsigned int ttr, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	unsigned long long ret = vformat_put(fmt, ap, pri, delay, ttr);
	va_end(ap);
	return ret;
}

unsigned long long beanstalk::vformat_put(const char* fmt, va_list ap,
	unsigned pri /* = 1024 */, unsigned delay /* = 0 */,
	unsigned ttr /* = 60 */)
{
	string buf;
	buf.vformat(fmt, ap);
	return put(buf.c_str(), buf.length(), pri, delay, ttr);
}

unsigned long long beanstalk::format_put(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	unsigned long long ret = vformat_put(fmt, ap);
	va_end(ap);
	return ret;
}

unsigned beanstalk::watch(const char* tube)
{
	// 先检查是否已经监控相同队列
	std::vector<char*>::iterator it = tubes_watched_.begin();
	for (; it != tubes_watched_.end(); ++it) {
		if (strcmp(*it, tube) == 0) {
			break;
		}
	}

	string cmdline(128);
	cmdline.format("watch %s\r\n", tube);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return 0;
	}
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "WATCHING")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return 0;
	}

	unsigned n = (unsigned) atoi(tokens->argv[1]);
	acl_argv_free(tokens);

	// 如果服务器返回所关注的队列数为 0，肯定是出错了，因为至少还有一个
	// 缺省队列：default，所以此时需要关闭连接，以尽量消除与本连接相关
	// 的错误，下一个命令会自动进行重连操作以恢复操作过程
	if (n == 0) {
		logger_error("'%s' error, tube watched is 0", cmdline.c_str());
		errbuf_ = "watching";
		close();
	}

	// 添加进监控集合中
	else if (it == tubes_watched_.end()) {
		tubes_watched_.push_back(acl_mystrdup(tube));
	}

	return n;
}

unsigned beanstalk::ignore(const char* tube)
{
	if (strcasecmp(tube, "default") == 0) {
		logger_error("tube(%s) is default, can't be ignore", tube);
		errbuf_ = "default";
		return 0;
	}

	bool found = false;
	std::vector<char*>::iterator it = tubes_watched_.begin();
	for (; it != tubes_watched_.end(); ++it) {
		if (strcmp(tube, *it) == 0) {
			acl_myfree(*it);
			tubes_watched_.erase(it);
			found = true;
			break;
		}
	}

	if (!found) {
		logger_error("tube(%s) not found", tube);
		errbuf_ = "tube_not_found";
		return 0;
	}
	return ignore_one(tube);
}

unsigned beanstalk::ignore_all(void)
{
	if (tubes_watched_.size() <= 1) {
		// first tube watched must be "default"
		if (strcmp(tubes_watched_[0], "default") != 0) {
			logger_fatal("first tube(%s) is not default",
				tubes_watched_[0]);
		}
		return 1;
	}

	unsigned ret = 1;  // at least one default tube is watched
	std::vector<char*>::iterator it = tubes_watched_.begin();
	++it;  // skip first default tube
	for (; it != tubes_watched_.end();) {
		ret = ignore_one(*it);
		if (ret == 0) {
			logger_error("ignore tube %s failed", *it);
			errbuf_ = "ignore";
			acl_myfree(*it);
			return 0;
		}
		acl_myfree(*it);
		it = tubes_watched_.erase(it);
		ret++;
	}

	return ret;
}

unsigned beanstalk::ignore_one(const char* tube)
{
	string cmdline(128);
	cmdline.format("ignore %s\r\n", tube);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return 0;
	}
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "WATCHING")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return 0;
	}

	unsigned n = (unsigned) atoi(tokens->argv[1]);
	acl_argv_free(tokens);

	// 如果服务器返回所关注的队列数为 0，肯定是出错了，因为至少还有一个
	// 缺省队列：default，所以此时需要关闭连接，以尽量消除与本连接相关
	// 的错误，下一个命令会自动进行重连操作以恢复操作过程
	if (n == 0) {
		logger_error("'%s' error, tube watched is 0", cmdline.c_str());
		errbuf_ = "no_watching";
		close();
	}
	return n;
}

unsigned long long beanstalk::reserve(string& buf, int timeout /* = -1 */)
{
	buf.clear(); // 虽然读操作过程也会清空缓存
	string cmdline(128);
	if (timeout >= 0) {
		cmdline.format("reserve-with-timeout %d\r\n", timeout);
	} else {
		cmdline.format("reserve\r\n");
	}
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return 0;
	}
	if (strcasecmp(tokens->argv[0], "TIMED_OUT") == 0
		|| strcasecmp(tokens->argv[0], "DEADLINE_SOON") == 0) {

		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return 0;
	}
	if (tokens->argc < 3 || strcasecmp(tokens->argv[0], "RESERVED")) {
		logger_error("reserve failed: %s", tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return 0;
	}

#ifdef MINGW
	unsigned long long id = (unsigned long long) atol(tokens->argv[1]);
#else
	unsigned long long id = (unsigned long long) atoll(tokens->argv[1]);
#endif
	unsigned short n = (unsigned short) atoi(tokens->argv[2]);
	acl_argv_free(tokens);

	if (n == 0) {
		logger_error("reserve data's length 0");
		errbuf_ = "reserve";
		close();
		return 0;
	}

	// 读取规定的字节数
	if (!conn_.read(buf, n, true)) {
		logger_error("reserve read body failed");
		errbuf_ = "read";
		close();
		return 0;
	} else if (!conn_.gets(cmdline)) {
		logger_error("reserve: gets last line failed");
		errbuf_ = "gets";
		close();
		return 0;
	}
	return id;
}

bool beanstalk::delete_id(unsigned long long id)
{
	string cmdline(128);
	cmdline.format("delete %llu\r\n", id);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (strcasecmp(tokens->argv[0], "DELETED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

bool beanstalk::release(unsigned long long id, unsigned pri /* = 1024 */,
	unsigned delay /* = 0*/)
{
	string cmdline(128);
	cmdline.format("release %llu %u %u\r\n", id, pri, delay);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (strcasecmp(tokens->argv[0], "RELEASED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

bool beanstalk::bury(unsigned long long id, unsigned int pri /* = 1024 */)
{
	string cmdline(128);
	cmdline.format("bury %llu %u\r\n", id, pri);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (strcasecmp(tokens->argv[0], "BURIED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

bool beanstalk::touch(unsigned long long id)
{
	string cmdline(128);
	cmdline.format("touch %llu\r\n", id);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (strcasecmp(tokens->argv[0], "TOUCHED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

unsigned long long beanstalk::peek_fmt(string& buf, const char* fmt, ...)
{
	buf.clear(); // 虽然读操作过程也会清空缓存

	string cmdline(128);
	va_list ap;
	va_start(ap, fmt);
	cmdline.vformat(fmt, ap);
	va_end(ap);

	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return 0;
	}
	if (tokens->argc < 3 || strcasecmp(tokens->argv[0], "FOUND")) {
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return 0;
	}

#ifdef MINGW
	unsigned long long id = (unsigned long long) atol(tokens->argv[1]);
#else
	unsigned long long id = (unsigned long long) atoll(tokens->argv[1]);
#endif
	unsigned short n = (unsigned short) atoi(tokens->argv[2]);
	acl_argv_free(tokens);

	if (n == 0) {
		logger_error("peek data's length 0");
		errbuf_ = "peek";
		close();
		return 0;
	}

	// 读取规定的字节数
	if (!conn_.read(buf, n, true)) {
		logger_error("peek read body failed");
		errbuf_ = "read";
		close();
		return 0;
	} else if (!conn_.gets(cmdline)) {
		logger_error("peek: gets last line falied");
		errbuf_ = "gets";
		close();
		return 0;
	}
	return id;
}

unsigned long long beanstalk::peek(string& buf, unsigned long long id)
{
	return peek_fmt(buf, "peek %llu\r\n", id);
}

unsigned long long beanstalk::peek_ready(string& buf)
{
	return peek_fmt(buf, "peek-ready\r\n");
}

unsigned long long beanstalk::peek_delayed(string& buf)
{
	return peek_fmt(buf, "peek-delayed\r\n");
}

unsigned long long beanstalk::peek_buried(string& buf)
{
	return peek_fmt(buf, "peek-buried\r\n");
}

int beanstalk::kick(unsigned n)
{
	string cmdline(128);
	cmdline.format("kick %u\r\n", n);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return -1;
	}
	if (strcasecmp(tokens->argv[0], "KICKED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return -1;
	}

	int  ret;
	if (tokens->argc >= 2) {
		ret = atoi(tokens->argv[1]);
	} else {
		ret = 0;
	}
	acl_argv_free(tokens);
	return ret;
}

bool beanstalk::list_tube_used(string& buf)
{
	buf.clear(); // 虽然读操作过程也会清空缓存

	string cmdline(128);
	cmdline.format("list-tube-used\r\n");
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "USING")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return false;
	}

	buf = tokens->argv[1];
	acl_argv_free(tokens);
	return true;
}

bool beanstalk::list_tubes_fmt(string& buf, const char* fmt, ...)
{
	buf.clear(); // 虽然读操作过程也会清空缓存

	string cmdline(128);
	va_list ap;
	va_start(ap, fmt);
	cmdline.vformat(fmt, ap);
	va_end(ap);

	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (tokens->argc < 2 || strcasecmp(tokens->argv[0], "OK")) {
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		close();
		return false;
	}
	unsigned short n = (unsigned short) atoi(tokens->argv[1]);
	acl_argv_free(tokens);

	if (n == 0) {
		logger_error("list data's length 0");
		errbuf_ = "length";
		close();
		return false;
	}

	// 读取规定的字节数
	if (!conn_.read(buf, n, true)) {
		logger_error("peek read body failed");
		errbuf_ = "read";
		close();
		return false;
	}
	return true;
}

bool beanstalk::list_tubes(string& buf)
{
	return list_tubes_fmt(buf, "list-tubes\r\n");
}

bool beanstalk::list_tubes_watched(string& buf)
{
	return list_tubes_fmt(buf, "list-tubes-watched\r\n");
}

bool beanstalk::pause_tube(const char* tube, unsigned delay)
{
	string cmdline(128);
	cmdline.format("pause-tube %s %u\r\n", tube, delay);
	ACL_ARGV* tokens = beanstalk_request(cmdline);
	if (tokens == NULL) {
		logger_error("'%s' error", cmdline.c_str());
		return false;
	}
	if (strcasecmp(tokens->argv[0], "PAUSED")) {
		logger_error("'%s' error %s", cmdline.c_str(), tokens->argv[0]);
		errbuf_ = tokens->argv[0];
		acl_argv_free(tokens);
		return false;
	}

	acl_argv_free(tokens);
	return true;
}

void beanstalk::quit(void)
{
	if (conn_.opened()) {
		conn_.format("quit\r\n");
		conn_.close();
	}
}

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)
