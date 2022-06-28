#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/sslbase_conf.hpp"
#include "acl_cpp/stream/sslbase_io.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/smtp/mail_message.hpp"
#include "acl_cpp/smtp/smtp_client.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

smtp_client::smtp_client(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 60 */)
{
	addr_ = acl_mystrdup(addr);
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	client_ = NULL;

	ehlo_ = true;
	reuse_ = false;
	ssl_conf_ = NULL;
}

smtp_client::~smtp_client(void)
{
	acl_myfree(addr_);
	close();
}

smtp_client& smtp_client::set_ssl(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

int smtp_client::get_code(void) const
{
	return client_ == NULL ? -1 : client_->smtp_code;
}

const char* smtp_client::get_status(void) const
{
	return client_ == NULL ? "unknown" : client_->buf;
}

bool smtp_client::send(const mail_message& message,
	const char* email /* = NULL */)
{
	// 发送 SMTP 邮件信封过程
	if (!send_envelope(message)) {
		return false;
	}

	// 优先使用参数给出的邮件文件，然后才是 message 中自动生成的邮件文件
	if (email == NULL) {
		email = message.get_email();
	}

	// 如果没有可发送的邮件文件，则认为调用者想通过 write 等接口直接发送数据
	if (email == NULL) {
		return true;
	}

	// 发送 DATA 命令
	if (!data_begin()) {
		return false;
	}

	// 发送邮件
	if (!send_email(email)) {
		return false;
	}

	// 发送 DATA 结束符
	return data_end();
}

bool smtp_client::send_envelope(const mail_message& message)
{
	if (!open()) {
		return false;
	}
	if (!get_banner()) {
		return false;
	}
	if (!greet()) {
		return false;
	}

	const char* user = message.get_auth_user();
	const char* pass = message.get_auth_pass();
	if (user && pass && !auth_login(user, pass)) {
		return false;
	}

	const rfc822_addr* from = message.get_from();
	if (from == NULL) {
		logger_error("from null");
		return false;
	}
	if (!mail_from(from->addr)) {
		return false;
	}
	return to_recipients(message.get_recipients());
}

bool smtp_client::open(void)
{
	if (stream_.opened()) {
		acl_assert(client_ != NULL);
		acl_assert(client_->conn == stream_.get_vstream());
		reuse_ = true;
		return true;
	}

	reuse_ = false;

	client_ = smtp_open(addr_, conn_timeout_, rw_timeout_, 1024);
	if (client_ == NULL) {
		logger_error("connect %s error: %s", addr_, last_serror());
		return false;
	}

	// 打开流对象，只所以使用 stream_ 主要为了使用 SSL 通信
	stream_.open(client_->conn);

	// 如果设置了 SSL 通信方式，则需要打开 SSL 通信接口
	if (ssl_conf_) {
		sslbase_io* ssl = ssl_conf_->create(false);
		if (stream_.setup_hook(ssl) == ssl) {
			logger_error("open ssl client error!");
			ssl->destroy();
			return false;
		}
	}

	return true;
}

void smtp_client::close(void)
{
	if (client_) {
		// 将 SMTP_CLIENT 对象的流置空，以避免内部再次释放，
		// 因为该流对象会在下面 stream_.close() 时被释放
		client_->conn = NULL;
		smtp_close(client_);
		client_ = NULL;
	}

	// 当 socket 流对象打开着，则关闭之，同时将依附于其的 SSL 对象释放
	if (stream_.opened()) {
		stream_.close();
	}

	// 重置连接是否被重用的状态
	reuse_ = false;
}

bool smtp_client::get_banner(void)
{
	// 如果是同一个连接被使用，则不必再获得服务端的欢迎信息
	if (reuse_) {
		return true;
	}
	return smtp_get_banner(client_) == 0 ? true : false;
}

bool smtp_client::greet(void)
{
	return smtp_greet(client_, "localhost", ehlo_ ? 1 : 0)
		== 0 ? true : false;
}

bool smtp_client::auth_login(const char* user, const char* pass)
{
	if (user == NULL || *user == 0) {
		logger_error("user null");
		return false;
	}
	if (pass == NULL || *pass == 0) {
		logger_error("pass null");
		return false;
	}
	return smtp_auth(client_, user, pass) == 0 ? true : false;
}

bool smtp_client::mail_from(const char* from)
{
	if (from == NULL || *from == 0) {
		logger_error("from null");
		return false;
	}
	return smtp_mail(client_, from) == 0 ? true : false;
}

bool smtp_client::rcpt_to(const char* to)
{
	if (to == NULL || *to == 0) {
		logger_error("to null");
		return false;
	}
	return smtp_rcpt(client_, to) == 0 ? true : false;
}

bool smtp_client::to_recipients(const std::vector<rfc822_addr*>& recipients)
{
	std::vector<rfc822_addr*>::const_iterator cit;
	for (cit = recipients.begin(); cit != recipients.end(); ++cit) {
		if ((*cit)->addr && !rcpt_to((*cit)->addr)) {
			return false;
		}
	}
	return true;
}

bool smtp_client::data_begin(void)
{
	return smtp_data(client_) == 0 ? true : false;
}

bool smtp_client::data_end(void)
{
	return smtp_data_end(client_) == 0 ? true : false;
}

bool smtp_client::quit(void)
{
	return smtp_quit(client_) == 0 ? true : false;
}

bool smtp_client::noop(void)
{
	return smtp_noop(client_) == 0 ? true : false;
}

bool smtp_client::reset(void)
{
	return smtp_rset(client_) == 0 ? true : false;
}

bool smtp_client::write(const char* data, size_t len)
{
	return smtp_send(client_, data, len) == 0 ? true : false;
}

bool smtp_client::format(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bool ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

bool smtp_client::vformat(const char* fmt, va_list ap)
{
	string buf;
	buf.vformat(fmt, ap);
	return write(buf.c_str(), buf.size());
}

bool smtp_client::send_email(const char* filepath)
{
	return smtp_send_file(client_, filepath) == 0 ? true : false;
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
