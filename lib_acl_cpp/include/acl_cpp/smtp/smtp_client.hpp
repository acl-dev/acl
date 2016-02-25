#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include <vector>

struct SMTP_CLIENT;

namespace acl {

class istream;
class polarssl_conf;
class mail_message;

/**
 * SMTP 邮件发送客户端类，可以使用此类对象发送邮件，支持身份认证等功能
 */
class ACL_CPP_API smtp_client
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} SMTP 邮件服务器地址，格式：IP:PORT
	 *  或 domain:port
	 * @param conn_timeout {int} 连接服务器的超时时间(秒）
	 * @param rw_timeout {int} 网络 IO 超时时间(秒)
	 */
	smtp_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 60);
	~smtp_client();

	/**
	 * 调用本函数发送邮件数据至邮件服务端，该函数会首先调用 send_envelop
	 * 发送信封，当 email 或 message.get_email() 非空时，则会调用发送邮件
	 * 过程；否则（即 email 和 message.get_email() 均为 NULL）则只发送
	 * 信封
	 * @param message {const mail_messsage&} 邮件相关信息，必须提前构建好
	 * @param email {const char*} 非空时，优先使用此文件做为邮件体数据发送
	 * @return {bool} 发送是否成功
	 *  注：如果 email 为 NULL 同时 messsage.get_email() 也为 NULL，则本
	 *     函数仅发送 SMTP 信封部分，用户还需要调用：
	 *     data_begin-->write|format|vformat|send_file-->data_end
	 *     过程来发送邮件数据体
	 */
	bool send(const mail_message& message, const char* email = NULL);

	/**
	 * 在 SMTP 会话阶段仅发送邮件信封部分数据，应用调用此函数成功后，
	 * 还需要调用:
	 * 1、data_begin：开始发送邮件体指令
	 * 2、write/format/vformat/send_file：发送邮件数据
	 * 3、data_end：表示发送邮件体数据完毕
	 * @param message {const mail_message&} 发送邮件所构建的邮件消息对象
	 * @return {bool} 是否成功
	 *  注：本函数是 open/auth_login/mail_from/rcpt_to 发送信封过程的组合
	 */
	bool send_envelope(const mail_message& message);

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

	/////////////////////////////////////////////////////////////////////

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param data {const char*} 邮件内容
	 * @param len {size_t} data 邮件数据长度
	 * @return {bool} 命令操作是否成功
	 *  注：在第一次调用本函数前，必须保证 SMTP 信封已经成功发送
	 */
	bool write(const char* data, size_t len);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @return {bool} 命令操作是否成功
	 *  注：在第一次调用本函数前，必须保证 SMTP 信封已经成功发送
	 */
	bool format(const char* fmt, ...);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @param ap {va_list}
	 * @return {bool} 命令操作是否成功
	 *  注：在第一次调用本函数前，必须保证 SMTP 信封已经成功发送
	 */
	bool vformat(const char* fmt, va_list ap);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 连接远程 SMTP 服务器
	 * @return {bool} 连接是否成功，若想使用 SSL 方式，则需要在类对象
	 *  初始化后调用 set_ssl 设置 SSL 通信方式
	 */
	bool open();

	/**
	 * 主动关闭与 SMTP 服务端的连接
	 */
	void close();

	/**
	 * 第一次连接成功后需要调用本函数获得 SMTP 服务端的欢迎信息
	 * @return {bool} 是否成功
	 */
	bool get_banner();

	/**
	 * 调用 get_banner 成功后调用本函数向 SMTP 服务端发送 HELO/HELO 命令
	 * @return {bool} 是否成功
	 */
	bool greet();

	/**
	 * 调用 gree 成功后调用本函数向 SMTP 服务端发送身份认证命令
	 * @param user {const char*} 用户账号，非空字符串
	 * @param pass {const char*} 用户账号密码，非空字符串
	 * @return {bool} 是否成功
	 */
	bool auth_login(const char* user, const char* pass);

	/**
	 * 调用 auth_login 成功后（如果无身份验证，则可以在 greet 成功后）
	 * 调用本函数向 SMTP 服务器发送 MAIL FROM 命令
	 * @param from {const char*} 发件人的邮箱地址
	 * @return {bool} 是否成功
	 */
	bool mail_from(const char* from);

	/**
	 * 调用 mail_from 成功后调用本函数向 SMTP 服务端发送 RCPT TO 命令，
	 * 指明一个收件人，可以多次本函数将邮件发送至多个收件人
	 * @param to {const char*} 收件人邮箱地址
	 * @return {bool} 是否成功
	 */
	bool rcpt_to(const char* to);

	/**
	 * 调用 rcpt_to 或 send_envelope 成功调用本函数向 SMTP 服务端
	 * DATA 命令，表明开始发送邮件数据
	 * @return {bool} 命令操作是否成功
	 *  注：在调用本函数前，必须保证 SMTP 信封已经成功发送
	 */
	bool data_begin();

	/**
	 * 调用 data_begin 成功调用本函数向 SMTP 服务端发送一封完整的邮件，
	 * 需要给出邮件存储于磁盘上的路径
	 * @param filepath {const char*} 邮件文件路径
	 * @return {bool} 命令操作是否成功
	 *  注：在调用本函数前，必须保证 SMTP 信封已经成功发送
	 */
	bool send_email(const char* filepath);

	/**
	 * 邮件发送完毕（如调用：send_email）后，最后必须调用本函数告诉 SMTP
	 * 邮件服务器发送数据结束
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

	/**
	 * 获得与 SMTP 服务器之间的连接流对象，该函数只有当 open 成功后才可调用
	 * @return {socket_stream&}
	 */
	socket_stream& get_stream(void)
	{
		return stream_;
	}

private:
	polarssl_conf* ssl_conf_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	SMTP_CLIENT* client_;
	socket_stream stream_;
	bool  ehlo_;
	bool  reuse_;

	bool to_recipients(const std::vector<rfc822_addr*>& recipients);
};

} // namespace acl
