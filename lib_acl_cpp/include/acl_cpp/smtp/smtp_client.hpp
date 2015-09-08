#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include <vector>

struct SMTP_CLIENT;

namespace acl {

class istream;
class polarssl_conf;
class mail_message;

class ACL_CPP_API smtp_client
{
public:
	smtp_client(const char* addr, int conn_timeout, int rw_timeout);
	~smtp_client();

	/**
	 * 调用本函数发送邮件数据至邮件服务端
	 * @param message {const mail_messsage&} 邮件相关信息，必须提前构建好
	 * @param email {const char*} 非空时，优先使用此文件做为邮件体数据发送
	 * @
	 */
	bool send(const mail_message& message, const char* email = NULL);

	/**
	 * 设置 SSL 数据传输模式
	 * @param ssl_conf {polarssl_conf*} 非空时，指定采用 SSL 传输模式
	 * @return {smtp_client&}
	 */
	smtp_client& set_ssl(polarssl_conf* ssl_conf);

	/**
	 * 获得上次 SMTP 交互过程服务端返回的状态码
	 * @return {int}
	 */
	int get_code() const;

	/**
	 * 获得上次 SMTP 交互过程服务端返回的状态信息
	 * @return {const char*}
	 */
	const char* get_status() const;

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param data {const char*} 邮件内容
	 * @param len {size_t} data 邮件数据长度
	 * @return {bool} 命令操作是否成功
	 */
	bool write(const char* data, size_t len);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @return {bool} 命令操作是否成功
	 */
	bool format(const char* fmt, ...);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @param ap {va_list}
	 * @return {bool} 命令操作是否成功
	 */
	bool vformat(const char* fmt, va_list ap);

	/////////////////////////////////////////////////////////////////////

	// 以下是打开连接和发送信封的分步步骤
	bool open();
	bool get_banner();
	bool greet();
	bool auth_login(const char* user, const char* pass);
	bool mail_from(const char* from);
	bool rcpt_to(const char* to);

	/**
	 * 发送一封完整的邮件，需要给出邮件存储于磁盘上的路径
	 * @param filepath {const char*} 邮件文件路径
	 * @return {bool} 命令操作是否成功
	 */
	bool send_email(const char* filepath);

	/**
	 * 开始发送邮件数据命令:DATA
	 * @return {bool} 命令操作是否成功
	 */
	bool data_begin();
	/**
	 * 邮件发送完毕后，最后必须调用本函数告诉邮件服务器发送数据结束
	 * @return {bool} 命令操作是否成功
	 */
	bool data_end();

	/**
	 * 断开与邮件服务器的连接
	 * @return {bool} 命令操作是否成功
	 */
	bool quit();

	/**
	 * NOOP 命令
	 * @return {bool} 命令操作是否成功
	 */
	bool noop();

	/**
	 * 重置与邮件服务器的连接状态
	 * @return {bool} 命令操作是否成功
	 */
	bool reset();

private:
	polarssl_conf* ssl_conf_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	SMTP_CLIENT* client_;
	socket_stream stream_;
	bool  ehlo_;

	bool to_recipients(const std::vector<rfc822_addr*>& recipients);
};

} // namespace acl
