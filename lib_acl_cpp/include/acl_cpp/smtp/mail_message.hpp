#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class dbuf_pool;
struct rfc822_addr;
class mail_attach;
class mail_body;
class ofstream;

/**
 * 邮件数据构造类，此类可以生成一封完整的邮件，同时还用于构建 SMTP 发送过程
 * 的邮件信封信息
 */
class ACL_CPP_API mail_message : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param charset {const char*} 字符集
	 */
	mail_message(const char* charset = "utf-8");
	~mail_message();

	/**
	 * 设置 SMTP 发送过程的身份验证信息
	 * @param user {const char*} 邮箱账号
	 * @param pass {const char*} 邮箱密码
	 * @return {mail_message&}
	 */
	mail_message& set_auth(const char* user, const char* pass);

	/**
	 * 设置邮件的发送都邮箱，此字段可用于 SMTP 发送过程的 MAIL FROM 命令，
	 * 同时又可作为邮件头中的 From 字段值
	 * @param from {const char*} 发件人邮件地址
	 * @param name {const char*} 发件人名称
	 * @return {mail_message&}
	 */
	mail_message& set_from(const char* from, const char* name = NULL);

	mail_message& set_sender(const char* sender, const char* name = NULL);

	/**
	 * 设置邮件头中的 Reply-To 字段值
	 * @param reply_to {const char*} Reply-To 邮箱字段值
	 * @param name {const char*} Reply-To 对应的人员名称
	 * @return {mail_message&}
	 */
	mail_message& set_reply_to(const char* reply_to, const char* name = NULL);

	/**
	 * 设置邮件头中的 Return-Path 字段值
	 * @param return_path {const char*} Return-Path 邮箱字段值
	 * @return {mail_message&}
	 */
	mail_message& set_return_path(const char* return_path);

	/**
	 * 设置邮件头中的 Delivered-To 字段值
	 * @param delivered_to {const char*} Delivered-To 邮箱字段值
	 * @return {mail_message&}
	 */
	mail_message& set_delivered_to(const char* delivered_to);

	/**
	 * 添加收件人地址，该地址仅出现在信封中，不出现在邮件头中
	 * @param recipients {const char*} 收件人集合，遵守 RFC822 格式
	 * @return {mail_message&}
	 */
	mail_message& add_recipients(const char* recipients);

	/**
	 * 设置邮件头中的 To 字段值，同时该收件人地址集合被用于信封中作为收件人
	 * @param to {const char*} 收件人邮箱地址集合，遵守 RFC822 格式
	 * @return {mail_message&}
	 */
	mail_message& add_to(const char* to);

	/**
	 * 设置邮件头中的 Cc 字段值，同时该收件人地址集合被用于信封中作为收件人
	 * @param cc {const char*} 收件人邮箱地址集合，遵守 RFC822 格式
	 * @return {mail_message&}
	 */
	mail_message& add_cc(const char* cc);

	/**
	 * 设置邮件发送的暗送地址集合，该地址集合不会出现在邮件头中
	 * @param bcc {const char*} 暗送邮箱地址集合，遵守 RFC822 格式
	 * @return {mail_message&}
	 */
	mail_message& add_bcc(const char* bcc);

	/**
	 * 设置邮件头中的主题，该主题将采用 rfc2047 编码且采用类构造函数
	 * 设置的字符集
	 * @param subject {const char*} 邮件头主题字段值
	 * @return {mail_message&}
	 */
	mail_message& set_subject(const char* subject);

	/**
	 * 用户可以调用此函数添加邮件头中的头部扩展字段值
	 * @param name {const char*} 字段名
	 * @param value {const char*} 字段值
	 * @return {mail_message&}
	 */
	mail_message& add_header(const char* name, const char* value);

	/**
	 * 设置邮件的正文对象
	 * @param body {const mail_body&} 邮件正文对象
	 * @return {mail_message&}
	 */
	mail_message& set_body(const mail_body& body);

	/**
	 * 给一封邮件添加一个附件
	 * @param filepath {const char*} 附件全路径（非空）
	 * @param content_type {const char*} 附件类型（非空）
	 * @return {mail_message&}
	 */
	mail_message& add_attachment(const char* filepath,
		const char* content_type);

	/**
	 * 构造一封完整的邮件，并将邮件内容存储于给定磁盘文件中，如果该文件
	 * 存在则首先会清空，否则会创建新的文件
	 * @param filepath {const char*} 目标文件
	 * @return {bool} 操作是否成功
	 */
	bool save_to(const char* filepath);

	/**
	 * 可以单独调用本函数用来生成邮件头数据
	 * @param out {string&} 创建的邮件头数据将追加于该缓冲区中
	 * @return {bool} 操作是否成功
	 */
	bool build_header(string& out);

	/**
	 * 获得所创建的邮件在磁盘上的全路径，该函数必须在调用 save_to 成功后调用
	 * @return {const char*}
	 */
	const char* get_email() const
	{
		return filepath_;
	}

	/**
	 * 获得用于 SMTP 身份验证时的邮箱账号
	 * @return {const char*}
	 */
	const char* get_auth_user() const
	{
		return auth_user_;
	}

	/**
	 * 获得用于 SMTP 身份验证时的邮箱账号密码
	 * @return {const char*}
	 */
	const char* get_auth_pass() const
	{
		return auth_pass_;
	}

	/**
	 * 获得由 set_from 设置的邮箱地址对象
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_from() const
	{
		return from_;
	}

	/**
	 * 获得由 set_sender 设置的邮箱地址对象
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_sender() const
	{
		return sender_;
	}

	/**
	 * 获得由 set_reply_to 设置的邮箱地址对象
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_reply_to() const
	{
		return reply_to_;
	}

	/**
	 * 获得由 set_return_path 设置的邮箱地址对象
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_return_path() const
	{
		return return_path_;
	}

	/**
	 * 获得由 set_delivered_to 设置的邮箱地址对象
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_delivered_to() const
	{
		return delivered_to_;
	}

	const std::vector<rfc822_addr*>& get_to() const
	{
		return to_list_;
	}

	/**
	 * 获得由 set_cc 设置的邮箱地址对象集合
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_cc() const
	{
		return cc_list_;
	}

	/**
	 * 获得由 set_bcc 设置的邮箱地址对象集合
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_bcc() const
	{
		return bcc_list_;
	}

	/**
	 * 获得所有邮件接收者的地址集合
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_recipients() const
	{
		return recipients_;
	}

	/**
	 * 获得用户设置的邮件头扩展字段值
	 * @param name {const char*} 字段名
	 * @return {const char*}
	 */
	const char* get_header_value(const char* name) const;

	/**
	 * 为 MIME 数据创建唯一的分隔符
	 * @param id {const char*} 调用者填写的 ID 标识
	 * @param out {string&} 存储结果
	 */
	static void create_boundary(const char* id, string& out);

private:
	dbuf_pool* dbuf_;
	char* charset_;
	char* transfer_encoding_;

	char* auth_user_;
	char* auth_pass_;
	rfc822_addr* from_;
	rfc822_addr* sender_;
	rfc822_addr* reply_to_;
	rfc822_addr* return_path_;
	rfc822_addr* delivered_to_;
	std::vector<rfc822_addr*> to_list_;
	std::vector<rfc822_addr*> cc_list_;
	std::vector<rfc822_addr*> bcc_list_;
	std::vector<rfc822_addr*> recipients_;
	char* subject_;
	std::vector<std::pair<char*, char*> > headers_;
	std::vector<mail_attach*> attachments_;
	const mail_body* body_;
	size_t body_len_;
	char* filepath_;

	void add_addrs(const char* in, std::vector<rfc822_addr*>& out);
	bool append_addr(const rfc822_addr& addr, string& out);
	bool append_addr(const char* name, const rfc822_addr& addr,
		string& out);
	bool append_addrs(const char* name,
		const std::vector<rfc822_addr*>& addrs, string& out);
	bool append_message_id(string& out);
	bool append_subject(const char* subject, string& out);
	bool append_date(string& out);
	bool append_header(ofstream& fp);
	bool append_multipart(ofstream& fp);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
